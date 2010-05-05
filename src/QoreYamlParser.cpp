/* indent-tabs-mode: nil -*- */
/*
  yaml Qore module

  Copyright (C) 2010 David Nichols, all rights reserved

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

const char *QY_PARSE_ERR = "YAML-PARSER-ERROR";

static const char *invalid_date_format = "invalid date format";
static const char *truncated_date = "invalid date format; input truncated";
static const char *invalid_chars_after_time = "invalid characters after time value";

AbstractQoreNode *QoreYamlParser::parse() {
   ReferenceHolder<AbstractQoreNode> rv(xsink);

   if (getCheckEvent(YAML_STREAM_START_EVENT))
      return 0;

   if (getEvent())
      return 0;

   if (event.type == YAML_DOCUMENT_START_EVENT) {
      if (getEvent())
	 return 0;

      if (event.type != YAML_DOCUMENT_END_EVENT) {
	 rv = parseNode();
	 if (*xsink)
	    return 0;

	 if (getCheckEvent(YAML_DOCUMENT_END_EVENT))
	    return 0;
      }
   }

   if (getCheckEvent(YAML_STREAM_END_EVENT))
      return 0;

   return rv.release();
}

AbstractQoreNode *QoreYamlParser::parseNode() {
   switch (event.type) {
      case YAML_SCALAR_EVENT:
	 return parseScalar();

      case YAML_SEQUENCE_START_EVENT:
	 return parseSeq();

      case YAML_MAPPING_START_EVENT:
	 return parseMap();

      default:
	 xsink->raiseException(QY_PARSE_ERR, "unexpected event '%s' when parsing YAML document", get_event_name(event.type));
   }

   return 0;
}

QoreListNode *QoreYamlParser::parseSeq() {
   ReferenceHolder<QoreListNode> l(new QoreListNode, xsink);

   while (true) {
      if (getEvent())
	 return 0;

      if (event.type == YAML_SEQUENCE_END_EVENT) 
	 break;

      AbstractQoreNode *rv = parseNode();
      if (*xsink)
	 return 0;
      l->push(rv);
   }

   return l.release();
}

QoreHashNode *QoreYamlParser::parseMap() {
   ReferenceHolder<QoreHashNode> h(new QoreHashNode, xsink);

   return h.release();
}

static AbstractQoreNode *dt_err(ExceptionSink *xsink, const char *val, const char *msg) {
   xsink->raiseException(QY_PARSE_ERR, "cannot parse timestamp value '%s': %s", val, msg);
   return 0;
}

AbstractQoreNode *QoreYamlParser::parseScalar() {
   //ReferenceHolder<AbstractQoreNode> rv(xsink);

   printd(0, "QoreYamlParser::parseScalar() anchor=%s tag=%s value=%s len=%d plain_implicit=%d quoted_implicit=%d style=%d\n",
	  event.data.scalar.anchor ? event.data.scalar.anchor : (yaml_char_t*)"n/a", 
	  event.data.scalar.tag ? event.data.scalar.tag : (yaml_char_t*)"n/a", 
	  event.data.scalar.value, event.data.scalar.length, event.data.scalar.plain_implicit, 
	  event.data.scalar.quoted_implicit, event.data.scalar.style);

   const char *val = (const char *)event.data.scalar.value;
   size_t len = event.data.scalar.length;

   if (!event.data.scalar.tag) {
      if (event.data.scalar.quoted_implicit) {
	 // assume it's a string
	 return new QoreStringNode(val, len, QCS_UTF8);
      }

      if (!strcmp(val, "true"))
	 return &True;
      if (!strcmp(val, "false"))
	 return &False;
      if (!strcmp(val, "null"))
	 return 0;

      double f = atof(val);

      // assume it's a string if it can't be converted to a number
      if (!f && len)
	 return new QoreStringNode(val, len, QCS_UTF8);

      // if the integer value is equal to the fp value, then it's an integer
      if (((double)((int64)f)) == f) 
	 return new QoreBigIntNode((int64)f);

      return new QoreFloatNode(f);
   }

   const char *tag = (const char *)event.data.scalar.tag;
   if (!strcmp(tag, YAML_TIMESTAMP_TAG)) {
      if (len < 8)
	 return dt_err(xsink, val, invalid_date_format);
      
      int year = atol(val);
      const char *p = val + 4;

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

      printd(0, "date: %04d-%02d-%02d\n", year, month, day);

      // according to the YAML draft timestamp spec, if no time or time zone 
      // information is given, then the value is assumed to be in UTC
      // http://yaml.org/type/timestamp.html

      // if there is no time portion, return date in UTC
      if (!*p)
	 return DateTimeNode::makeAbsolute(0, year, month, day);

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
	 return DateTimeNode::makeAbsolute(0, year, month, day, hour, minute, second);
	 
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
	 return DateTimeNode::makeAbsolute(0, year, month, day, hour, minute, second, us);

      // get time zone offset
      const AbstractQoreZoneInfo *zone = 0;

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

      return DateTimeNode::makeAbsolute(zone, year, month, day, hour, minute, second, us);
   }

   if (!strcmp(tag, YAML_BINARY_TAG))
      return parseBase64(val, len, xsink);

   xsink->raiseException(QY_PARSE_ERR, "don't know how to parse scalar tag '%s'", tag);

   return 0;
}
