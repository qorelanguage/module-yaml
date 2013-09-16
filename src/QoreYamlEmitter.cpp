/* indent-tabs-mode: nil -*- */
/*
  yaml Qore module

  Copyright (C) 2010 - 2013 David Nichols, all rights reserved

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

const char *QY_EMIT_ERR = "YAML-EMITTER-ERROR";

static int qore_yaml_write_handler(QoreYamlWriteHandler *wh, unsigned char *buffer, size_t size) {
   return wh->write(buffer, size);
}

QoreYamlEmitter::QoreYamlEmitter(QoreYamlWriteHandler &n_wh, int flags, int width, int indent, ExceptionSink *n_xsink)
   : QoreYamlBase(n_xsink), wh(n_wh), block(flags & QYE_BLOCK_STYLE), 
     implicit_start_doc(!(flags & QYE_EXPLICIT_START_DOC)), 
     implicit_end_doc(!(flags & QYE_EXPLICIT_END_DOC)),
     yaml_ver(0) {
   if (!yaml_emitter_initialize(&emitter)) {
      err("unknown error initializing yaml emitter");
      return;
   }

   if (flags & QYE_VER_1_1 && flags & QYE_VER_1_2) {
      err("cannot set both Yaml1_1 and Yaml1_2 version flags");
      return;
   }

   if (flags & QYE_CANONICAL)
      setCanonical();

   if (!(flags & QYE_ESCAPE_UNICODE))
      setUnicode();

   if (flags & QYE_VER_1_2)
      yaml_ver = &yaml_ver_1_2;
   else if (flags & QYE_VER_1_1)
      yaml_ver = &yaml_ver_1_1;
   else if (flags & QYE_VER_1_0)
      yaml_ver = &yaml_ver_1_0;

   yaml_emitter_set_output(&emitter, (yaml_write_handler_t*)qore_yaml_write_handler, &wh);   

   //printd(5, "QoreYamlEmitter::QoreYamlEmitter() indent=%d width=%d\n", indent, width);
   yaml_emitter_set_indent(&emitter, indent);
   yaml_emitter_set_width(&emitter, width);

   if (!streamStart() && !docStart())
      valid = true;   
}

int QoreYamlEmitter::emit(const AbstractQoreNode* p) {
   qore_type_t t = get_node_type(p);
   switch (t) {
      case NT_STRING:
	 return emit(*reinterpret_cast<const QoreStringNode*>(p));

      case NT_INT:
	 return emit(*reinterpret_cast<const QoreBigIntNode*>(p));

      case NT_FLOAT:
	 return emit(*reinterpret_cast<const QoreFloatNode*>(p));

      case NT_BOOLEAN:
	 return emit(*reinterpret_cast<const QoreBoolNode*>(p));

      case NT_LIST:
	 return emit(*reinterpret_cast<const QoreListNode*>(p));

      case NT_HASH:
	 return emit(*reinterpret_cast<const QoreHashNode*>(p));

      case NT_DATE:
	 return emit(*reinterpret_cast<const DateTimeNode*>(p));

      case NT_BINARY:
	 return emit(*reinterpret_cast<const BinaryNode*>(p));

#ifdef _QORE_HAS_NUMBER_TYPE
      case NT_NUMBER:
	 return emit(*reinterpret_cast<const QoreNumberNode*>(p));
#endif

      case NT_NULL:
      case NT_NOTHING:
	 return emitNull();

      default:
	 err("cannot convert Qore type '%s' to YAML", get_type_name(p));
	 return -1;
   }

   return 0;
}

int QoreYamlEmitter::emit(const DateTime &d) {
   qore_tm info;
   d.getInfo(info);

   QoreString str(QCS_UTF8);

   if (d.isRelative()) {
      str.concat('P');
      if (d.hasValue()) {
	 if (info.year)
	    str.sprintf("%dY", info.year);
	 if (info.month)
	    str.sprintf("%dM", info.month);
	 if (info.day)
	    str.sprintf("%dD", info.day);
	 
	 bool has_t = false;
	 
	 if (info.hour) {
	    str.sprintf("T%dH", info.hour);
	    has_t = true;
	 }
	 if (info.minute) {
	    if (!has_t) {
	       str.concat('T');
	       has_t = true;
	    }
	    str.sprintf("%dM", info.minute);
	 }
	 if (info.second) {
	    if (!has_t) {
	       str.concat('T');
	       has_t = true;
	    }
	    str.sprintf("%dS", info.second);
	 }
	 if (info.us) {
	    if (!has_t) {
	       str.concat('T');
	       has_t = true;
	    }
	    str.sprintf("%du", info.us);
	 }
      }
      else
	 str.concat("0D");

      return emitScalar(str, QORE_YAML_DURATION_TAG);
   }

   // shorthand type name
   if (emitter.canonical) {
      d.format(str, "YYYY-MM-DDTHH:mm:SS.xx");
   }
   else {
      d.format(str, "YYYY-MM-DD");
      if (!info.isTimeNull() || info.secsEast()) {
	 // use spaces for enhanced readability
	 if (!info.us)
	    d.format(str, " HH:mm:SS.");
	 else if (!(info.us % 1000))
	    d.format(str, " HH:mm:SS.ms");
	 else
	    d.format(str, " HH:mm:SS.xx");
      }
   }

   // remove trailing zeros from microseconds time
   str.trim_trailing('0');
   // remove empty decimal point if us == 0
   str.trim_trailing('.');

   // if not emitting the canonical format and there is a time zone offset,
   // then add a space
   if (!emitter.canonical) {
      if (!info.isTimeNull() || info.secsEast()) {
	 str.concat(' ');
	 // add time zone offset (or "Z")
	 d.format(str, "Z");
      }
   }
   else
      d.format(str, "Z");

   return emitScalar(str, YAML_TIMESTAMP_TAG);
}
