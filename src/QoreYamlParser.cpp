/* indent-tabs-mode: nil -*- */
/*
    yaml Qore module

    Copyright (C) 2010 - 2021 Qore Technologies, s.r.o.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "yaml-module.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <math.h>

const char* QY_PARSE_ERR = "YAML-PARSER-ERROR";

static const char* invalid_date_format = "invalid date format";
static const char* truncated_date = "invalid date format; input truncated";
static const char* invalid_chars_after_time = "invalid characters after time value";

QoreValue QoreYamlParser::parse() {
    ValueHolder rv(xsink);

    if (getCheckEvent(YAML_STREAM_START_EVENT))
        return QoreValue();

    if (getEvent())
        return QoreValue();

    if (event.type == YAML_DOCUMENT_START_EVENT) {
        if (getEvent())
            return QoreValue();

        if (event.type != YAML_DOCUMENT_END_EVENT) {
            rv = parseNode();
            if (*xsink)
                return QoreValue();

            if (getCheckEvent(YAML_DOCUMENT_END_EVENT))
                return QoreValue();

            if (getEvent())
                return QoreValue();
        }
    }

    if (checkEvent(YAML_STREAM_END_EVENT))
        return QoreValue();

    return rv.release();
}

QoreValue QoreYamlParser::parseNode(bool favor_string) {
    switch (event.type) {
        case YAML_SCALAR_EVENT:
            return parseScalar(favor_string);

        case YAML_SEQUENCE_START_EVENT:
            return parseSeq();

        case YAML_MAPPING_START_EVENT:
            return parseMap();

        default:
            xsink->raiseException(QY_PARSE_ERR, "unexpected event '%s' when parsing YAML document", get_event_name(event.type));
    }

    return QoreValue();
}

QoreListNode* QoreYamlParser::parseSeq() {
    ReferenceHolder<QoreListNode> l(new QoreListNode(autoTypeInfo), xsink);

    while (true) {
        if (getEvent())
            return nullptr;

        if (event.type == YAML_SEQUENCE_END_EVENT)
            break;

        QoreValue rv = parseNode();
        if (*xsink)
            return nullptr;
        l->push(rv, nullptr);
    }

    return l.release();
}

QoreHashNode* QoreYamlParser::parseMap() {
    ReferenceHolder<QoreHashNode> h(new QoreHashNode(autoTypeInfo), xsink);

    while (true) {
        if (getEvent())
            return nullptr;

        if (event.type == YAML_MAPPING_END_EVENT)
            break;

        // get key node and convert to string
        ValueHolder key(parseNode(true), xsink);
        if (*xsink)
            return nullptr;

        //printd(5, "key=%p type=%s\n", *key, key->getTypeName());

        // convert to string in default encoding
        QoreStringValueHelper str(*key, QCS_DEFAULT, xsink);
        if (*xsink)
            return nullptr;

        // get value
        if (getEvent())
            return nullptr;

        QoreValue value = parseNode();
        if (*xsink)
            return nullptr;

        h->setKeyValue(str->c_str(), value, xsink);
        if (*xsink)
            return nullptr;
    }

    return h.release();
}

static DateTimeNode* dt_err(ExceptionSink* xsink, const char* val, const char* msg) {
    xsink->raiseException(QY_PARSE_ERR, "cannot parse timestamp value '%s': %s", val, msg);
    return nullptr;
}

static bool is_number(const char* &tv) {
    if (*tv == '-')
        ++tv;
    if (!isdigit(*tv))
        return false;
    ++tv;
    while (isdigit(*tv))
        ++tv;
    return true;
}

static unsigned is_prec(const char* str, size_t len) {
    //printd(5, "is_prec str: '%s' len: %d strlen: %d\n", str, len, strlen(str));
    if (*str != '{')
        return 0;

    const char* p = str + 1;
    while (isdigit(*p))
        ++p;
    if (p == (str + 1) || *p != '}' || static_cast<size_t>(p - str + 1) != len)
        return 0;

    return (unsigned)atoi(str + 1);
}

static double parseFloat(const char* val, size_t len) {
    assert(len);
    bool sign = (*val == '-' || *val == '+');
    if ((len == static_cast<size_t>(5 + sign)) && (!strcasecmp(val + sign, "@nan@") || !strcasecmp(val + sign, "@inf@"))) {
        if (val[1 + sign] == 'n' || val[1 + sign] == 'N')
            return (double)NAN;
        double d = (double)INFINITY;
        if (*val == '-')
            d = -d;
        return d;
    }
    // issue #2832 do not use strtod() as it's locale-dependent
    return q_strtod(val);
}

static QoreNumberNode* parseNumber(const char* val, size_t len) {
    assert(val);
    bool sign = (*val == '-' || *val == '+');

    // check for @inf@ and @nan@
    if (!strncasecmp(val + sign, "@nan@", 5) || !strncasecmp(val + sign, "@inf@", 5)) {
        if (val[5 + sign] == 'n') {
            if (len == static_cast<size_t>(6 + sign))
                return new QoreNumberNode(val);
            else {
                unsigned prec = is_prec(val + sign + 6, len - sign - 6);
                if (prec)
                    return new QoreNumberNode(val, prec);
            }
        }
        return nullptr;
    }

    const char* p = strchr(val, '{');
    if (p) {
        unsigned prec = is_prec(p, len - (p - val));
        //printd(5, "%s prec %d\n", val, prec);
        if (prec)
            return new QoreNumberNode(val, prec);
    }

    return new QoreNumberNode(val);
}

// FIXME: this is still capable of false positives
static QoreValue try_parse_number(const char* val, size_t len, bool no_simple_numeric = false) {
    //printd(5, "try_parse_number() val: \"%s\" len: %d\n", val, (int)len);
    bool sign = (*val == '-' || *val == '+');

    // check for @inf@ and @nan@
    if (!strncasecmp(val + sign, "@nan@", 5) || !strncasecmp(val + sign, "@inf@", 5)) {
        if (len == static_cast<size_t>(5 + sign)) {
            if (val[1 + sign] == 'n' || val[1 + sign] == 'N')
                return (double)NAN;
            double d = (double)INFINITY;
            if (*val == '-')
                d = -d;
            return d;
        }
        if (val[5 + sign] == 'n') {
            if (len == static_cast<size_t>(6 + sign))
                return new QoreNumberNode(val);
            else {
                unsigned prec = is_prec(val + sign + 6, len - sign - 6);
                if (prec)
                    return new QoreNumberNode(val, prec);
            }
        }
        return QoreValue();
    }

    const char* str = val + (int)sign;
    // only digits flag
    bool od = true;
    // e char flag
    bool e = false;
    // decimal point flag
    bool dp = false;
    // plus or minus flag (for exponent)
    bool pm = false;
    while (*str) {
        if (isdigit(*str)) {
            ++str;
            continue;
        }
        if (*str == 'n') {
            if ((size_t)(str - val + 1) == len)
                return new QoreNumberNode(val);
            else {
                unsigned prec = is_prec(str + 1, len - (str - val) - 1);
                //printd(5, "%s prec %d\n", val, prec);
                if (prec)
                    return new QoreNumberNode(val, prec);
            }
        }
        if (od)
            od = false;
        if (*str == '.') {
            if (dp || e || pm)
                return QoreValue();
            dp = true;
        } else if ((*str == 'e' || *str == 'E') && (isdigit(*(str + 1)) || *(str + 1) == '+' || *(str + 1) == '-')) {
            if (e || pm)
                return QoreValue();
            e = true;
        } else if ((*str == '+' || *str == '-') && isdigit(*(str + 1))) {
            if (pm || !e)
                return QoreValue();
            pm = true;
        } else
            return QoreValue();
        ++str;
    }

    //printd(5, "try_parse_number() od: %d len: %d val: %s\n", od, len, val);

    if (od) {
        if ((len < 19
            || (len == 19 && !sign
                && ((strcmp(val, "9223372036854775807") <= 0)))
            || (len == 20 && sign
                && ((*val == '+' && strcmp(val, "+9223372036854775807") <= 0)
                    || (*val == '-' && strcmp(val, "-9223372036854775808") <= 0))))) {
            if (no_simple_numeric)
                return QoreValue();
            int64 iv = strtoll(val, 0, 10);
            errno = 0;
            assert(errno != ERANGE);
            //printd(5, "try_parse_number() returning INT\n");
            return iv;
        }
        //printd(5, "try_parse_number() returning NUMBER\n");
        // if it is an integer requiring > 64bits, use "number" if possible
        return new QoreNumberNode(val);
    }

    // issue #2832 do not use strtod() as it's locale-dependent
    return no_simple_numeric ? QoreValue() : QoreValue(q_strtod(val));
}

QoreValue QoreYamlParser::parseScalar(bool favor_string) {
    //ReferenceHolder<AbstractQoreNode> rv(xsink);

    const char* val = (const char*)event.data.scalar.value;
    size_t len = event.data.scalar.length;

    //printd(5, "QoreYamlParser::parseScalar() anchor=%s tag=%s value=%s len=%d plain_implicit=%d quoted_implicit=%d style=%d\n", event.data.scalar.anchor ? event.data.scalar.anchor : (yaml_char_t*)"n/a", event.data.scalar.tag ? event.data.scalar.tag : (yaml_char_t*)"n/a", val, len, event.data.scalar.plain_implicit, event.data.scalar.quoted_implicit, event.data.scalar.style);

    if (!event.data.scalar.tag) {
        if (favor_string || (event.data.scalar.quoted_implicit
                            && (event.data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE))) {
            // assume it's a string
            return new QoreStringNode(val, len, QCS_UTF8);
        }

        // issue #2343: could be a string, arbitrary-precision numeric or special floating-point value, or an ISO-8601 date/time value
        if (event.data.scalar.quoted_implicit && event.data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE) {
            if (len > 9 && isdigit(val[0]) && isdigit(val[1]) && isdigit(val[2]) && isdigit(val[3]) && val[4] == '-'
                && isdigit(val[5]) && isdigit(val[6]) && val[7] == '-'
                && isdigit(val[8]) && isdigit(val[9]))
                return parseAbsoluteDate();
            // try
            QoreValue n = try_parse_number(val, len, true);
            return n ? n : new QoreStringNode(val, len, QCS_UTF8);
        }

        // check for boolean values
        if (!strcmp(val, "true"))
            return true;
        if (!strcmp(val, "false"))
            return false;

        // check for null
        if (!strcmp(val, "null") || !strcmp(val, "~") || !len)
            return QoreValue();

        // check for sqlnull
        if (!strcmp(val, "sqlnull"))
            return &Null;

        // check for absolute date/time values
        if (len > 9 && isdigit(val[0]) && isdigit(val[1]) && isdigit(val[2]) && isdigit(val[3]) && val[4] == '-'
            && isdigit(val[5]) && isdigit(val[6]) && val[7] == '-'
            && isdigit(val[8]) && isdigit(val[9]))
            return parseAbsoluteDate();

        // check for relative date/time values (durations)
        if (*val == 'P') {
            const char* tv = val + 1;
            if (*tv == 'T') {
                ++tv;
                if (is_number(tv)) {
                if (*tv == 'H' || *tv == 'M' || *tv == 'S' || *tv == 'u')
                    return new DateTimeNode(val);
                }
            }
            else if (is_number(tv)) {
                if (*tv == 'Y' || *tv == 'M' || *tv == 'D')
                return new DateTimeNode(val);
            }
        }

        QoreValue n = try_parse_number(val, len);
        if (n)
            return n;

        return new QoreStringNode(val, len, QCS_UTF8);
    }

    const char* tag = (const char*)event.data.scalar.tag;
    // FIXME: use a map here for O(ln(n)) performance
    if (!strcmp(tag, YAML_TIMESTAMP_TAG))
        return parseAbsoluteDate();
    if (!strcmp(tag, YAML_BINARY_TAG))
        return parseBase64(val, len, xsink);
    if (!strcmp(tag, YAML_STR_TAG))
        return new QoreStringNode(val, len, QCS_UTF8);
    if (!strcmp(tag, YAML_NULL_TAG))
        return QoreValue();
    if (!strcmp(tag, YAML_BOOL_TAG))
        return parseBool();
    if (!strcmp(tag, YAML_INT_TAG))
        return q_atoll(val);
    if (!strcmp(tag, YAML_FLOAT_TAG))
        return parseFloat(val, len);
    if (!strcmp(tag, QORE_YAML_DURATION_TAG))
        return new DateTimeNode(val);
    if (!strcmp(tag, QORE_YAML_NUMBER_TAG))
        return parseNumber(val, len);
    if (!strcmp(tag, QORE_YAML_SQLNULL_TAG))
        return &Null;

    xsink->raiseException(QY_PARSE_ERR, "don't know how to parse scalar tag '%s'", tag);

    return QoreValue();
}

bool QoreYamlParser::parseBool() {
   const char* val = (const char*)event.data.scalar.value;

   if (!strcmp(val, "true"))
      return true;
   if (!strcmp(val, "false"))
      return false;

   xsink->raiseException(QY_PARSE_ERR, "cannot parse boolean value '%s'", val);
   return false;
}

// always return the date/time value in the current timezone
static DateTimeNode* yaml_return_date(DateTimeNode* d) {
   d->setZone(currentTZ());
   return d;
}

DateTimeNode* QoreYamlParser::parseAbsoluteDate() {
   const char* val = (const char*)event.data.scalar.value;
   size_t len = event.data.scalar.length;

   if (len < 8)
      return dt_err(xsink, val, invalid_date_format);

   int year = atol(val);
   const char* p = val + 4;

   if (*p != '-')
      return dt_err(xsink, val, invalid_date_format);

   ++p;
   int month = *p - '0';
   ++p;
   if (isdigit(*p)) {
      month = month * 10 + (*p - '0');
      ++p;
   }

   if (*p != '-')
      return dt_err(xsink, val, invalid_date_format);

   ++p;

   int day = *p - '0';
   ++p;
   if (isdigit(*p)) {
      day = day * 10 + (*p - '0');
      ++p;
   }

   //printd(5, "date: %04d-%02d-%02d\n", year, month, day);

   // according to the YAML draft timestamp spec, if no time zone
   // information is given, then the value is assumed to be in UTC
   // http://yaml.org/type/timestamp.html

   // if there is no time portion, return date in UTC
   if (!*p)
      return yaml_return_date(DateTimeNode::makeAbsolute(0, year, month, day));

   if (*p != ' ' && *p != 't' && *p != 'T')
      return dt_err(xsink, val, "invalid date/time separator character");

   ++p;
   if (!isdigit(*p))
      return dt_err(xsink, val, truncated_date);

   int hour = *p - '0';
   ++p;
   if (isdigit(*p)) {
      hour = hour * 10 + (*p - '0');
      ++p;
   }

   if (!*p)
      return dt_err(xsink, val, truncated_date);
   if (*p != ':')
      return dt_err(xsink, val, "invalid hours/minutes separator character");

   ++p;
   if (!isdigit(*p))
      return dt_err(xsink, val, truncated_date);

   int minute = *p - '0';
   ++p;
   if (isdigit(*p)) {
      minute = minute * 10 + (*p - '0');
      ++p;
   }

   if (!*p)
      return dt_err(xsink, val, truncated_date);
   if (*p != ':')
      return dt_err(xsink, val, "invalid minutes/seconds separator character");

   ++p;
   if (!isdigit(*p))
      return dt_err(xsink, val, truncated_date);

   int second = *p - '0';
   ++p;
   if (isdigit(*p)) {
      second = second * 10 + (*p - '0');
      ++p;
   }

   if (!*p)
      return yaml_return_date(DateTimeNode::makeAbsolute(0, year, month, day, hour, minute, second));

   int us = 0;
   if (*p == '.') {
      ++p;
      if (!isdigit(*p))
         return dt_err(xsink, val, truncated_date);

      // read all digits
      int len = 0;
      while (isdigit(*p)) {
         us *= 10;
         us += *p - '0';
         ++len;
         ++p;
      }

      // adjust to microseconds
      while (len < 6) {
         us *= 10;
         ++len;
      }
      while (len > 6) {
         us /= 10;
         --len;
      }
   }

   if (!*p)
      return yaml_return_date(DateTimeNode::makeAbsolute(0, year, month, day, hour, minute, second, us));

   const AbstractQoreZoneInfo* zone = 0;

   // read
   if (*p == ' ') {
      ++p;
      if (!*p)
         return dt_err(xsink, val, truncated_date);
   }
   if (*p == 'Z') {
      ++p;
   }
   else if (*p == '+' || *p == '-') {
      int mult = *p == '-' ? -1 : 1;

      ++p;
      if (!isdigit(*p))
         return dt_err(xsink, val, truncated_date);

      int utc_h = *p - '0';
      ++p;
      if (isdigit(*p)) {
         utc_h = utc_h * 10 + (*p - '0');
         ++p;
      }

      int offset = utc_h * 3600;

      if (*p) {
         if (*p != ':')
            return dt_err(xsink, val, "invalid time zone hours/minutes separator character");

         ++p;
         if (!isdigit(*p))
            return dt_err(xsink, val, truncated_date);

         int utc_m = *p - '0';
         ++p;
         if (isdigit(*p)) {
            utc_m = utc_m * 10 + (*p - '0');
            ++p;
         }

         offset += utc_m * 60;

         if (*p) {
            if (*p != ':')
               return dt_err(xsink, val, "invalid time zone hours/minutes separator character");

            ++p;
            if (!isdigit(*p))
               return dt_err(xsink, val, truncated_date);

            int utc_s = *p - '0';
            ++p;
            if (isdigit(*p)) {
               utc_s = utc_s * 10 + (*p - '0');
               ++p;
            }

            offset += utc_s;
         }
      }

      zone = findCreateOffsetZone(offset * mult);
   }
   else {
      return dt_err(xsink, val, invalid_chars_after_time);
   }

   if (*p)
      return dt_err(xsink, val, invalid_chars_after_time);

   return yaml_return_date(DateTimeNode::makeAbsolute(zone, year, month, day, hour, minute, second, us));
}
