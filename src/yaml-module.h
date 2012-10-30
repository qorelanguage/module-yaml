/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  yaml-module.h
  
  Qore Programming Language

  Copyright 2003 - 2010 David Nichols

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

#ifndef _QORE_YAML_MODULE_H
#define _QORE_YAML_MODULE_H

#include "../config.h"

#include <qore/Qore.h>

#include <yaml.h>

#include <stdarg.h>

#include <map>

#define QYE_NONE                0
#define QYE_CANONICAL           (1 << 0)
#define QYE_ESCAPE_UNICODE      (1 << 1)
#define QYE_EXPLICIT_START_DOC  (1 << 2)
#define QYE_EXPLICIT_END_DOC    (1 << 3)
#define QYE_BLOCK_STYLE         (1 << 4)
#define QYE_VER_1_0             (1 << 5)
#define QYE_VER_1_1             (1 << 6)
#define QYE_VER_1_2             (1 << 7)

#define QYE_DEFAULT (QYE_NONE)

#ifndef YAML_BINARY_TAG
#define YAML_BINARY_TAG "tag:yaml.org,2002:binary"
#endif

#ifndef YAML_OMAP_TAG
#define YAML_OMAP_TAG "tag:yaml.org,2002:omap"
#endif

DLLLOCAL extern const char* QORE_YAML_DURATION_TAG;
DLLLOCAL extern const char* QORE_YAML_NUMBER_TAG;

DLLLOCAL extern const char *QY_EMIT_ERR;

DLLLOCAL extern const char *QY_PARSE_ERR;

DLLLOCAL extern QoreString NullStr;

DLLLOCAL extern yaml_version_directive_t yaml_ver_1_0, yaml_ver_1_1, yaml_ver_1_2;

typedef std::map<int, const char *> event_map_t;
DLLLOCAL extern event_map_t event_map;

DLLLOCAL extern const char *get_event_name(yaml_event_type_t type);

class QoreYamlWriteHandler {
public:
   DLLLOCAL QoreYamlWriteHandler() {
   }
   DLLLOCAL virtual ~QoreYamlWriteHandler() {
   }
   DLLLOCAL virtual int write(unsigned char *buffer, size_t size) = 0;
};

class QoreYamlBase {
protected:
   yaml_event_t event;
   ExceptionSink *xsink;

   bool valid;

public:
   DLLLOCAL QoreYamlBase(ExceptionSink *n_xsink) : xsink(n_xsink), valid(false) {
   }
};

class QoreYamlEmitter : public QoreYamlBase {
protected:
   yaml_emitter_t emitter;
   QoreYamlWriteHandler &wh;

   bool block,
      implicit_start_doc, 
      implicit_end_doc;

   yaml_version_directive_t *yaml_ver;

   DLLLOCAL int err(const char *fmt, ...) {
      QoreStringNode *desc = new QoreStringNode;
      while (true) {
         va_list args;
         va_start(args, fmt);
         int rc = desc->vsprintf(fmt, args);
         va_end(args);
         if (!rc)
            break;
      }
      if (desc->strlen() > 255) {
         desc->terminate(255);
         desc->concat("...");
      }

      xsink->raiseException(QY_EMIT_ERR, desc);
      valid = false;
      return -1;
   }

   DLLLOCAL int emit(const char *event_str, const char *tag = 0) {
      if (!yaml_emitter_emit(&emitter, &event)) {
         if (tag)
            return err("error emitting yaml %s %s event", event_str, tag);
         return err("error emitting yaml %s event", event_str);
      }
            
      return 0;
   }

   DLLLOCAL int streamStart() {
      if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING))
	 return err("error initializing yaml stream start event");

      return emit("stream start");
   }

   DLLLOCAL int streamEnd() {
      if (!yaml_stream_end_event_initialize(&event))
	 return err("error initializing yaml stream end event");

      return emit("stream end");
   }

public:
   DLLLOCAL QoreYamlEmitter(QoreYamlWriteHandler &n_wh, int flags, int width, int indent, ExceptionSink *n_xsink);

   DLLLOCAL ~QoreYamlEmitter() {
      if (valid) {
	 docEnd();
	 streamEnd();
      }
      yaml_emitter_delete(&emitter);
   }

   DLLLOCAL int docStart(yaml_tag_directive_t *start = 0, unsigned elements = 0) {
      if (!yaml_document_start_event_initialize(&event, yaml_ver, start, start + elements, implicit_start_doc))
	 return err("unknown error initializing yaml document start event");

      return emit("doc start");
   }

   DLLLOCAL int docEnd() {
      //printd(5, "QoreYamlEmitter::docEnd() ied=%d\n", implicit_end_doc);
      if (!yaml_document_end_event_initialize(&event, implicit_end_doc))
	 return err("unknown error initializing yaml document end event");

      return emit("doc end");
   }

   DLLLOCAL int seqStart(yaml_sequence_style_t style = YAML_ANY_SEQUENCE_STYLE,
			 const char *tag = YAML_SEQ_TAG, const char *anchor = 0, bool implicit = true) {
      if (!yaml_sequence_start_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
						implicit, style))
	 return err("unknown error initializing yaml sequence start event");

      //printd(5, "QoreYamlEmitter::seqStart(tag=%s, anchor=%s)\n", tag, anchor ? anchor : "(null)");
      return emit("seq start");
   }

   DLLLOCAL int seqEnd() {
      if (!yaml_sequence_end_event_initialize(&event))
	 return err("unknown error initializing yaml sequence end event");

      return emit("seq end");
   }

   DLLLOCAL int mapStart(yaml_mapping_style_t style = YAML_ANY_MAPPING_STYLE,
			 const char *tag = YAML_MAP_TAG, const char *anchor = 0, bool implicit = true) {
      if (!yaml_mapping_start_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
					       implicit, style))
	 return err("unknown error initializing yaml mapping start event");

      return emit("map start");
   }

   DLLLOCAL int mapEnd() {
      if (!yaml_mapping_end_event_initialize(&event))
	 return err("unknown error initializing yaml mapping end event");

      return emit("map end");
   }

   DLLLOCAL int emitScalar(const QoreString &value, const char *tag, const char *anchor = 0, 
		       bool plain_implicit = true, bool quoted_implicit = true, 
		       yaml_scalar_style_t style = YAML_ANY_SCALAR_STYLE) {
      TempEncodingHelper str(&value, QCS_UTF8, xsink);
      if (*xsink)
	 return -1;

      if (!yaml_scalar_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
					(yaml_char_t *)str->getBuffer(), str->strlen(), 
					plain_implicit, quoted_implicit, style))
	 return err("unknown error initializing yaml scalar output event for yaml type '%s', value '%s'", tag, value.getBuffer());

      return emit("scalar", tag);
   }

   DLLLOCAL int emitScalar(const char *value, const char *tag, const char *anchor = 0, 
		       bool plain_implicit = true, bool quoted_implicit = true, 
		       yaml_scalar_style_t style = YAML_ANY_SCALAR_STYLE) {
      if (!yaml_scalar_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
					(yaml_char_t *)value, -1, 
					plain_implicit, quoted_implicit, style)) {
	 return err("unknown error initializing yaml scalar output event for yaml type '%s', value '%s'", tag, value);
      }

      return emit("scalar", tag);
   }

   DLLLOCAL int emit(const QoreString &str) {
      TempEncodingHelper tmp(&str, QCS_UTF8, xsink);
      if (!tmp)
	 return -1;
      return emitScalar(**tmp, YAML_STR_TAG, 0, true, true, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
   }
   
   DLLLOCAL int emit(const QoreBigIntNode &bi) {
      QoreString tmp;
      tmp.sprintf("%lld", bi.val);
      return emitScalar(tmp, YAML_INT_TAG);
   }

   DLLLOCAL int emit(const QoreFloatNode &f) {
      QoreString tmp;
      if (((double)((int64)f.f)) == f.f)
         tmp.sprintf("%g.0", f.f);
      else
         tmp.sprintf("%.25g", f.f);
      return emitScalar(tmp, YAML_FLOAT_TAG);
   }

#ifdef _QORE_HAS_NUMBER_TYPE
   DLLLOCAL int emit(const QoreNumberNode& n) {
      QoreString tmp;
      n.getStringRepresentation(tmp);
      tmp.concat('n');
      //printd(5, "yaml emit number: %s\n", tmp.getBuffer());
      return emitScalar(tmp, QORE_YAML_NUMBER_TAG);
   }
#endif

   DLLLOCAL int emit(const QoreBoolNode& b) {
      QoreString tmp;
      tmp.sprintf("%s", b.getValue() ? "true" : "false");
      return emitScalar(tmp, YAML_BOOL_TAG);
   }

   DLLLOCAL int emit(const QoreListNode &l) {
      if (seqStart(block ? YAML_BLOCK_SEQUENCE_STYLE : YAML_FLOW_SEQUENCE_STYLE))
	 return -1;
      ConstListIterator li(l);
      while (li.next()) {
	 if (emit(li.getValue()))
	    return -1;
      }
      return seqEnd();
   }

   DLLLOCAL int emit(const QoreHashNode &h) {
      if (mapStart(block ? YAML_BLOCK_MAPPING_STYLE : YAML_FLOW_MAPPING_STYLE))
	 return -1;
      ConstHashIterator hi(h);
      while (hi.next()) {
	 if (emitScalar(hi.getKey(), YAML_STR_TAG))
	    return -1;
	 if (emit(hi.getValue()))
	    return -1;
      }
      return mapEnd();
   }

   DLLLOCAL int emit(const DateTime &d);

   DLLLOCAL int emit(const BinaryNode &b) {
      QoreString str(QCS_UTF8);
      str.concatBase64(&b);
      return emitScalar(str, YAML_BINARY_TAG, 0, false, false, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
   }

   DLLLOCAL int emit(const AbstractQoreNode *p);
   
   DLLLOCAL int emitNull() {
      return emitScalar(NullStr, YAML_NULL_TAG);
   }
   
   DLLLOCAL void setCanonical(bool b = true) {
      yaml_emitter_set_canonical(&emitter, b);
   }

   DLLLOCAL void setUnicode(bool b = true) {
      yaml_emitter_set_unicode(&emitter, b);
   }
   
   DLLLOCAL bool getBlock() const {
      return block;
   }
};

class QoreYamlStringWriteHandler : public QoreYamlWriteHandler {
protected:
   QoreStringNode *str;

public:
   DLLLOCAL QoreYamlStringWriteHandler() : str(new QoreStringNode(QCS_UTF8)) {
   }
   DLLLOCAL ~QoreYamlStringWriteHandler() {
      if (str)
	 str->deref();
   }
   DLLLOCAL QoreStringNode *take() {
      QoreStringNode *rv = str;
      str = 0;
      return rv;
   }
   DLLLOCAL int write(unsigned char *buffer, size_t size) {
      assert(str);
      str->concat((const char *)buffer, size);
      return 1;
   }
};

class QoreYamlParser : public QoreYamlBase {
protected:
   yaml_parser_t parser;
   bool discard;

   DLLLOCAL void discardEvent() {
      if (discard) {
	 yaml_event_delete(&event);
	 discard = false;
      }
   }

   DLLLOCAL int getEvent() {
      discardEvent();
	 
      if (!yaml_parser_parse(&parser, &event)) {
	 valid = false;
      xsink->raiseException(QY_PARSE_ERR, "getEvent: unexpected event '%s' when parsing YAML document", get_event_name(event.type));
	 return -1;
      }
      //printd(5, "QoreYamlParser::getEvent() got %s event (%d)\n", get_event_name(event.type), event.type);

      discard = true;
      return 0;
   }

   DLLLOCAL int checkEvent(yaml_event_type_t type) {
      if (event.type != type) {
	 xsink->raiseException(QY_PARSE_ERR, "expecting '%s' event; got '%s' event instead", get_event_name(type), get_event_name(event.type));
	 return -1;
      }
      return 0;
   }

   DLLLOCAL int getCheckEvent(yaml_event_type_t type) {
      if (getEvent())
	 return -1;

      return checkEvent(type);
   }

   DLLLOCAL QoreListNode *parseSeq();
   DLLLOCAL QoreHashNode *parseMap();
   DLLLOCAL AbstractQoreNode *parseScalar(bool favor_string = false);
   DLLLOCAL AbstractQoreNode *parseNode(bool favor_string = false);
   DLLLOCAL DateTimeNode *parseAbsoluteDate();
   DLLLOCAL QoreBoolNode *parseBool();

public:
   DLLLOCAL QoreYamlParser(const QoreString &str, ExceptionSink *n_xsink) : QoreYamlBase(n_xsink), discard(false) {
      yaml_parser_initialize(&parser);
      yaml_parser_set_input_string(&parser, (const unsigned char *)str.getBuffer(), str.strlen());
      yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
      valid = true;
   }

   DLLLOCAL AbstractQoreNode *parse();

   DLLLOCAL ~QoreYamlParser() {
      discardEvent();
      yaml_parser_delete(&parser); 
   }
};

#endif
