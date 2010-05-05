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

QoreStringNode *yaml_module_init();
void yaml_module_ns_init(QoreNamespace *rns, QoreNamespace *qns);
void yaml_module_delete();

// qore module symbols
DLLEXPORT char qore_module_name[] = "yaml";
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "yaml module";
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qoretechnologies.com/qore";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = yaml_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = yaml_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = yaml_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

const char *QY_EMIT_ERR = "YAML-EMITTER-ERROR";

QoreString NullStr("null");

#define QY_NUM_TAGS (sizeof(QoreYamlEmitter::qore_tags)/sizeof(yaml_tag_directive_t))

int qore_yaml_write_handler(QoreYamlWriteHandler *wh, unsigned char *buffer, size_t size) {
   return wh->write(buffer, size);
}

QoreYamlEmitter::QoreYamlEmitter(QoreYamlWriteHandler &n_wh, int flags, ExceptionSink *n_xsink) : wh(n_wh), xsink(n_xsink), valid(false), implicit_start_doc(flags & QYE_IMPLICIT_START_DOC), implicit_end_doc(flags & QYE_IMPLICIT_END_DOC) {
   if (!yaml_emitter_initialize(&emitter)) {
      err("unknown error initializing yaml emitter");
      return;
   }

   if (flags & QYE_CANONICAL)
      setCanonical();

   if (!(flags & QYE_ESCAPE_UNICODE))
      setUnicode();

   yaml_emitter_set_output(&emitter, (yaml_write_handler_t *)qore_yaml_write_handler, &wh);

   if (!streamStart() && !docStart())
      valid = true;
}

int QoreYamlEmitter::emit(const AbstractQoreNode *p) {
   qore_type_t t = get_node_type(p);
   switch (t) {
      case NT_STRING:
	 return emit(*reinterpret_cast<const QoreStringNode *>(p));

      case NT_INT:
	 return emit(*reinterpret_cast<const QoreBigIntNode *>(p));

      case NT_FLOAT:
	 return emit(*reinterpret_cast<const QoreFloatNode *>(p));

      case NT_BOOLEAN:
	 return emit(*reinterpret_cast<const QoreBoolNode *>(p));

      case NT_LIST:
	 return emit(*reinterpret_cast<const QoreListNode *>(p));

      case NT_HASH:
	 return emit(*reinterpret_cast<const QoreHashNode *>(p));

      case NT_DATE:
	 return emit(*reinterpret_cast<const DateTimeNode *>(p));

      case NT_BINARY:
	 return emit(*reinterpret_cast<const BinaryNode *>(p));

      case NT_NULL:
      case NT_NOTHING:
	 return emitNull();

      default:
	 xsink->raiseException(QY_EMIT_ERR, "cannot convert Qore type '%s' to YAML", get_type_name(p));
	 break;
   }

   return 0;
}

static AbstractQoreNode *f_makeYAML(const QoreListNode *args, ExceptionSink *xsink) {
   const AbstractQoreNode *p = get_param(args, 0);
   int flags = HARD_QORE_INT(args, 1);

   QoreYamlStringWriteHandler str;
   {
      QoreYamlEmitter emitter(str, flags, xsink);
      if (*xsink)
	 return 0;
   
      if (emitter.emit(p))
	 return 0;
   }

   return str.take();
}

/*
static AbstractQoreNode *f_parseYAML(const QoreListNode *args, ExceptionSink *xsink) {
   return 0;
}
*/

QoreNamespace YNS("YAML");

QoreStringNode *yaml_module_init() {
   // add functions
   builtinFunctions.add2("makeYAML", f_makeYAML, QC_RET_VALUE_ONLY, QDOM_DEFAULT, stringTypeInfo, 2, anyTypeInfo, QORE_PARAM_NO_ARG, bigIntTypeInfo, new QoreBigIntNode(QYE_DEFAULT));
   //builtinFunctions.add2("parseYAML", f_parseYAML, QC_NO_FLAGS, QDOM_DEFAULT, anyTypeInfo, 1, anyTypeInfo, QORE_PARAM_NO_ARG);

   // setup namespace
   YNS.addConstant("Canonical", new QoreBigIntNode(QYE_CANONICAL));
   YNS.addConstant("EscapeUnicode", new QoreBigIntNode(QYE_ESCAPE_UNICODE));
   YNS.addConstant("ImplicitStartDoc", new QoreBigIntNode(QYE_IMPLICIT_START_DOC));
   YNS.addConstant("ImplicitEndDoc", new QoreBigIntNode(QYE_IMPLICIT_END_DOC));

   return 0;
}

void yaml_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(YNS.copy());
}

void yaml_module_delete() {
}

