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

#define QYE_NONE                0
#define QYE_CANONICAL           (1 << 0)
#define QYE_ESCAPE_UNICODE      (1 << 1)
#define QYE_IMPLICIT_START_DOC  (1 << 2)
#define QYE_IMPLICIT_END_DOC    (1 << 3)

#define QYE_DEFAULT (QYE_IMPLICIT_END_DOC)

#ifndef YAML_BINARY_TAG
#define YAML_BINARY_TAG "tag:yaml.org,2002:binary"
#endif

DLLLOCAL extern const char *QY_EMIT_ERR;
DLLLOCAL extern QoreString NullStr;

class QoreYamlWriteHandler {
public:
   DLLLOCAL QoreYamlWriteHandler() {
   }
   DLLLOCAL virtual ~QoreYamlWriteHandler() {
   }
   DLLLOCAL virtual int write(unsigned char *buffer, size_t size) = 0;
};

DLLLOCAL int qore_yaml_write_handler(QoreYamlWriteHandler *wh, unsigned char *buffer, size_t size);

class QoreYamlEmitter {
protected:
   yaml_emitter_t emitter;
   QoreYamlWriteHandler &wh;
   yaml_event_t event;
   ExceptionSink *xsink;

   bool valid,
      implicit_start_doc, 
      implicit_end_doc;

   DLLLOCAL int err(const char *msg) {
      xsink->raiseException(QY_EMIT_ERR, msg);
      valid = false;
      return -1;
   }

   DLLLOCAL int emit() {
      if (!yaml_emitter_emit(&emitter, &event))
	 return err("error emitting yaml event");

      return 0;
   }

   DLLLOCAL int streamStart() {
      if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING))
	 return err("error initializing yaml stream start event");

      return emit();
   }

   DLLLOCAL int streamEnd() {
      if (!yaml_stream_end_event_initialize(&event))
	 return err("error initializing yaml stream end event");

      return emit();
   }

public:
   DLLLOCAL QoreYamlEmitter(QoreYamlWriteHandler &n_wh, int flags, ExceptionSink *n_xsink);

   DLLLOCAL ~QoreYamlEmitter() {
      if (valid) {
	 docEnd();
	 streamEnd();
      }
      yaml_emitter_delete(&emitter);
   }

   DLLLOCAL int docStart(yaml_tag_directive_t *start = 0, unsigned elements = 0) {
      if (!yaml_document_start_event_initialize(&event, 0, start, start + elements, implicit_start_doc))
	 return err("unknown error initializing yaml document start event");

      return emit();
   }

   DLLLOCAL int docEnd() {
      //printd(5, "QoreYamlEmitter::docEnd() ied=%d\n", implicit_end_doc);
      if (!yaml_document_end_event_initialize(&event, implicit_end_doc))
	 return err("unknown error initializing yaml document end event");

      return emit();
   }

   DLLLOCAL int seqStart(yaml_sequence_style_t style = YAML_ANY_SEQUENCE_STYLE,
			 const char *tag = 0, const char *anchor = 0, bool implicit = true) {
      if (!yaml_sequence_start_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
						implicit, style))
	 return err("unknown error initializing yaml sequence start event");

      return emit();
   }

   DLLLOCAL int seqEnd() {
      if (!yaml_sequence_end_event_initialize(&event))
	 return err("unknown error initializing yaml sequence end event");

      return emit();
   }

   DLLLOCAL int mapStart(yaml_mapping_style_t style = YAML_ANY_MAPPING_STYLE,
			 const char *tag = 0, const char *anchor = 0, bool implicit = true) {
      if (!yaml_mapping_start_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
					       implicit, style))
	 return err("unknown error initializing yaml mapping start event");

      return emit();
   }

   DLLLOCAL int mapEnd() {
      if (!yaml_mapping_end_event_initialize(&event))
	 return err("unknown error initializing yaml mapping end event");

      return emit();
   }

   DLLLOCAL int scalar(const QoreString &value, const char *tag = 0, const char *anchor = 0, 
		       bool plain_implicit = true, bool quoted_implicit = true, 
		       yaml_scalar_style_t style = YAML_ANY_SCALAR_STYLE) {
      if (!yaml_scalar_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
					(yaml_char_t *)value.getBuffer(), value.strlen(), 
					plain_implicit, quoted_implicit, style))
	 return err("unknown error initializing yaml scalar output event");

      return emit();
   }

   DLLLOCAL int scalar(const char *value, const char *tag = 0, const char *anchor = 0, 
		       bool plain_implicit = true, bool quoted_implicit = true, 
		       yaml_scalar_style_t style = YAML_ANY_SCALAR_STYLE) {
      if (!yaml_scalar_event_initialize(&event, (yaml_char_t *)anchor, (yaml_char_t *)tag, 
					(yaml_char_t *)value, -1, 
					plain_implicit, quoted_implicit, style))
	 return err("unknown error initializing yaml scalar output event");

      return emit();
   }

   DLLLOCAL int emit(const QoreString &str) {
      TempEncodingHelper tmp(&str, QCS_UTF8, xsink);
      if (!tmp)
	 return -1;
      return scalar(**tmp, YAML_STR_TAG, 0, true, true, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
   }
   
   DLLLOCAL int emit(const QoreBigIntNode &bi) {
      QoreString tmp;
      tmp.sprintf("%lld", bi.val);
      return scalar(tmp, YAML_INT_TAG);
   }

   DLLLOCAL int emit(const QoreFloatNode &f) {
      QoreString tmp;
      tmp.sprintf("%g", f.f);
      return scalar(tmp, YAML_FLOAT_TAG);
   }

   DLLLOCAL int emit(const QoreBoolNode &b) {
      QoreString tmp;
      tmp.sprintf("%s", b.getValue() ? "true" : "false");
      return scalar(tmp, YAML_BOOL_TAG);
   }

   DLLLOCAL int emit(const QoreListNode &l) {
      if (seqStart(YAML_FLOW_SEQUENCE_STYLE))
	 return -1;
      ConstListIterator li(l);
      while (li.next()) {
	 if (emit(li.getValue()))
	    return -1;
      }
      return seqEnd();
   }

   DLLLOCAL int emit(const QoreHashNode &h) {
      if (mapStart(YAML_FLOW_MAPPING_STYLE))
	 return -1;
      ConstHashIterator hi(h);
      while (hi.next()) {
	 if (scalar(hi.getKey(), YAML_STR_TAG, 0, true, true, YAML_DOUBLE_QUOTED_SCALAR_STYLE))
	    return -1;
	 if (emit(hi.getValue()))
	    return -1;
      }
      return mapEnd();
   }

   DLLLOCAL int emit(const DateTime &d) {
      qore_tm info;
      d.getInfo(info);

      // shorthand type name
      QoreString str(QCS_UTF8);
      if (emitter.canonical) {
	 d.format(str, "YYYY-MM-DDTHH:mm:SS.xx");
	 // remove trailing zeros from microseconds time
	 str.trim_trailing('0');
	 // remove empty decimal point if us == 0
	 str.trim_trailing('.');
	 // add time zone offset (or "Z")
	 d.format(str, "Z");
      }
      else {
	 d.format(str, "YYYY-MM-DD");
	 if (!info.isTimeNull() || info.secsEast()) {
	    // use spaces for enhanced readability
	    if (!info.us)
	       d.format(str, " HH:mm:SS Z");
	    else if (!(info.us % 1000))
	       d.format(str, " HH:mm:SS.ms Z");
	    else
	       d.format(str, " HH:mm:SS.xx Z");
	 }
      }

      return scalar(str, YAML_TIMESTAMP_TAG, 0, false, false);
   }

   DLLLOCAL int emit(const BinaryNode &b) {
      QoreString str(QCS_UTF8);
      str.concatBase64(&b);
      return scalar(str, YAML_BINARY_TAG, 0, false, false, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
   }

   DLLLOCAL int emit(const AbstractQoreNode *p);
   
   DLLLOCAL int emitNull() {
      return scalar(NullStr, YAML_NULL_TAG);
   }
   
   DLLLOCAL void setCanonical(bool b = true) {
      yaml_emitter_set_canonical(&emitter, b);
   }

   DLLLOCAL void setUnicode(bool b = true) {
      yaml_emitter_set_unicode(&emitter, b);
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

#endif
