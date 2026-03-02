/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    yaml-scalar-util.cpp

    Shared scalar parsing utilities for YAML module

    Qore Programming Language

    Copyright 2003 - 2026 Qore Technologies, s.r.o.

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

#include "yaml-scalar-util.h"
#include "yaml-module.h"

#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <math.h>

static const char* invalid_date_format = "invalid date format";
static const char* truncated_date = "invalid date format; input truncated";
static const char* invalid_chars_after_time = "invalid characters after time value";

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
    if (*str != '{')
        return 0;

    const char* p = str + 1;
    while (isdigit(*p))
        ++p;
    if (p == (str + 1) || *p != '}' || static_cast<size_t>(p - str + 1) != len)
        return 0;

    return (unsigned)atoi(str + 1);
}

double yaml_parse_float(const char* val, size_t len) {
    assert(len);
    bool sign = (*val == '-' || *val == '+');
    if ((len == static_cast<size_t>(5 + sign)) && (!strcasecmp(val + sign, "@nan@")
        || !strcasecmp(val + sign, "@inf@"))) {
        if (val[1 + sign] == 'n' || val[1 + sign] == 'N')
            return (double)NAN;
        double d = (double)INFINITY;
        if (*val == '-')
            d = -d;
        return d;
    }
    // Use locale-independent parsing
    return q_strtod(val);
}

QoreNumberNode* yaml_parse_number(const char* val, size_t len) {
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
        if (prec)
            return new QoreNumberNode(val, prec);
    }

    return new QoreNumberNode(val);
}

QoreValue yaml_try_parse_number(const char* val, size_t len, bool no_simple_numeric) {
    // issue #4893: parsing 'n' -> 0n instead of "n"
    if (len == 1 && *val == 'n') {
        return QoreValue();
    }

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

    if (od) {
        if ((len < 19
            || (len == 19 && !sign
                && ((strcmp(val, "9223372036854775807") <= 0)))
            || (len == 20 && sign
                && ((*val == '+' && strcmp(val, "+9223372036854775807") <= 0)
                    || (*val == '-' && strcmp(val, "-9223372036854775808") <= 0))))) {
            if (no_simple_numeric)
                return QoreValue();
            errno = 0;
            int64 iv = strtoll(val, 0, 10);
            assert(errno != ERANGE);
            return iv;
        }
        // if it is an integer requiring > 64bits, use "number" if possible
        return new QoreNumberNode(val);
    }

    // Use locale-independent parsing
    return no_simple_numeric ? QoreValue() : QoreValue(q_strtod(val));
}

// Checks if the value matches the pattern for an absolute date/time string.
// This performs format validation only (YYYY-MM-DD pattern with valid ranges for month 01-12
// and day 01-31); actual date validity (e.g., rejecting Feb 30) is handled by
// yaml_parse_absolute_date() when the date is parsed by Qore's date parsing functions.
bool yaml_check_absolute_date(size_t len, const char* val, bool quoted) {
    if (quoted) {
        // we expect a full date when single quoted
        return (len >= 19 && isdigit(val[0]) && isdigit(val[1]) && isdigit(val[2]) && isdigit(val[3])
            && val[4] == '-'
            && ((val[5] == '0' && isdigit(val[6])) || (val[5] == '1' && (val[6] >= '0' && val[6] <= '2')))
            && val[7] == '-'
            && (((val[8] >= '0' && val[8] <= '2') && isdigit(val[9]))
                || (val[8] == '3' && (val[9] == '0' || val[9] == '1')))
            && val[10] == ' '
            && (((val[11] == '0' || (val[11] == '1')) && isdigit(val[12]))
                || (val[11] == '2' && (val[12] >= '0' && val[12] <= '3')))
            && val[13] == ':'
            && ((val[14] >= '0' && val[14] <= '5') && isdigit(val[15]))
            && val[16] == ':'
            && ((val[17] >= '0' && val[17] <= '5') && isdigit(val[18])));
    } else {
        return (len >= 9 && isdigit(val[0]) && isdigit(val[1]) && isdigit(val[2]) && isdigit(val[3])
            && val[4] == '-'
            && ((val[5] == '0' && isdigit(val[6])) || (val[5] == '1' && (val[6] >= '0' && val[6] <= '2')))
            && val[7] == '-'
            && (((val[8] >= '0' && val[8] <= '2') && isdigit(val[9]))
                || (val[8] == '3' && (val[9] == '0' || val[9] == '1'))));
    }
}

bool yaml_check_duration(const char* val) {
    if (*val != 'P') {
        return false;
    }

    const char* tv = val + 1;
    bool time = false;
    if (!*tv) {
        return false;
    }
    while (*tv) {
        if (*tv == 'T') {
            if (time) {
                return false;
            }
            time = true;
            ++tv;
            continue;
        }
        // find first non-number after time component code
        if (!is_number(tv)) {
            return false;
        }
        if (time) {
            if (*tv != 'H' && *tv != 'M' && *tv != 'S' && *tv != 'u') {
                return false;
            }
        } else if (*tv != 'Y' && *tv != 'M' && *tv != 'D') {
            return false;
        }
        ++tv;
    }

    return true;
}

// always return the date/time value in the current timezone
static DateTimeNode* yaml_return_date(DateTimeNode* d) {
    d->setZone(currentTZ());
    return d;
}

DateTimeNode* yaml_parse_absolute_date(const char* val, size_t len, ExceptionSink* xsink) {
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
        int frac_len = 0;
        while (isdigit(*p)) {
            us *= 10;
            us += *p - '0';
            ++frac_len;
            ++p;
        }

        // adjust to microseconds
        while (frac_len < 6) {
            us *= 10;
            ++frac_len;
        }
        while (frac_len > 6) {
            us /= 10;
            --frac_len;
        }
    }

    if (!*p)
        return yaml_return_date(DateTimeNode::makeAbsolute(0, year, month, day, hour, minute, second, us));

    const AbstractQoreZoneInfo* zone = 0;

    // read timezone
    if (*p == ' ') {
        ++p;
        if (!*p)
            return dt_err(xsink, val, truncated_date);
    }
    if (*p == 'Z') {
        ++p;
    } else if (*p == '+' || *p == '-') {
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
    } else {
        return dt_err(xsink, val, invalid_chars_after_time);
    }

    if (*p) {
        return dt_err(xsink, val, invalid_chars_after_time);
    }

    return yaml_return_date(DateTimeNode::makeAbsolute(zone, year, month, day, hour, minute, second, us));
}

DateTimeNode* yaml_parse_duration(const char* val) {
    return new DateTimeNode(val);
}

QoreValue yaml_parse_tagged_scalar(const char* val, size_t len, const char* tag, ExceptionSink* xsink) {
    if (!strcmp(tag, YAML_TIMESTAMP_TAG))
        return yaml_parse_absolute_date(val, len, xsink);
    if (!strcmp(tag, YAML_BINARY_TAG))
        return parseBase64(val, len, xsink);
    if (!strcmp(tag, YAML_STR_TAG))
        return new QoreStringNode(val, len, QCS_UTF8);
    if (!strcmp(tag, YAML_NULL_TAG))
        return QoreValue();
    if (!strcmp(tag, YAML_BOOL_TAG)) {
        if (!strcmp(val, "true"))
            return true;
        if (!strcmp(val, "false"))
            return false;
        xsink->raiseException(QY_PARSE_ERR, "cannot parse boolean value '%s'", val);
        return QoreValue();
    }
    if (!strcmp(tag, YAML_INT_TAG))
        return q_atoll(val);
    if (!strcmp(tag, YAML_FLOAT_TAG))
        return yaml_parse_float(val, len);
    if (!strcmp(tag, QORE_YAML_DURATION_TAG))
        return new DateTimeNode(val);
    if (!strcmp(tag, QORE_YAML_NUMBER_TAG))
        return yaml_parse_number(val, len);
    if (!strcmp(tag, QORE_YAML_SQLNULL_TAG))
        return &Null;

    xsink->raiseException(QY_PARSE_ERR, "don't know how to parse scalar tag '%s'", tag);
    return QoreValue();
}

QoreValue yaml_parse_implicit_scalar(const char* val, size_t len, yaml_scalar_style_t style,
                                      bool favor_string, ExceptionSink* xsink) {
    // For double-quoted strings with favor_string, always return as string
    if (favor_string || (style == YAML_DOUBLE_QUOTED_SCALAR_STYLE)) {
        return new QoreStringNode(val, len, QCS_UTF8);
    }

    // For single-quoted strings, check for dates and numbers
    if (style == YAML_SINGLE_QUOTED_SCALAR_STYLE) {
        if (yaml_check_absolute_date(len, val, true)) {
            return yaml_parse_absolute_date(val, len, xsink);
        }

        QoreValue n = yaml_try_parse_number(val, len, true);
        return n ? n : new QoreStringNode(val, len, QCS_UTF8);
    }

    // Plain scalars - full type inference
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
    if (yaml_check_absolute_date(len, val)) {
        return yaml_parse_absolute_date(val, len, xsink);
    }

    // check for relative date/time values (durations)
    if (yaml_check_duration(val)) {
        return yaml_parse_duration(val);
    }

    QoreValue n = yaml_try_parse_number(val, len);
    if (n) {
        return n;
    }

    return new QoreStringNode(val, len, QCS_UTF8);
}
