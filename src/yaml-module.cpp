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
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = yaml_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = yaml_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = yaml_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

QoreString NullStr("null");

yaml_version_directive_t yaml_ver_1_1 = {1, 1}, yaml_ver_1_2 = {1, 2};

// yaml event code to event string map
event_map_t event_map;

const char *get_event_name(yaml_event_type_t type) {
   event_map_t::iterator i = event_map.find(type);
   return i != event_map.end() ? i->second : "unknown";
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

static AbstractQoreNode *f_parseYAML(const QoreListNode *args, ExceptionSink *xsink) {
   const QoreStringNode *str = HARD_QORE_STRING(args, 0);
   QoreYamlParser parser(*str, xsink);
   return parser.parse();
}

QoreNamespace YNS("YAML");

QoreStringNode *yaml_module_init() {
   // add functions
   builtinFunctions.add2("makeYAML", f_makeYAML, QC_RET_VALUE_ONLY, QDOM_DEFAULT, stringTypeInfo, 2, anyTypeInfo, QORE_PARAM_NO_ARG, bigIntTypeInfo, new QoreBigIntNode(QYE_DEFAULT));
   builtinFunctions.add2("parseYAML", f_parseYAML, QC_NO_FLAGS, QDOM_DEFAULT, anyTypeInfo, 1, stringTypeInfo, QORE_PARAM_NO_ARG);

   // setup namespace
   YNS.addConstant("Canonical", new QoreBigIntNode(QYE_CANONICAL));
   YNS.addConstant("EscapeUnicode", new QoreBigIntNode(QYE_ESCAPE_UNICODE));
   YNS.addConstant("ExplicitStartDoc", new QoreBigIntNode(QYE_EXPLICIT_START_DOC));
   YNS.addConstant("ExplicitEndDoc", new QoreBigIntNode(QYE_EXPLICIT_END_DOC));
   YNS.addConstant("BlockStyle", new QoreBigIntNode(QYE_BLOCK_STYLE));
   YNS.addConstant("Yaml1_1", new QoreBigIntNode(QYE_VER_1_1));
   YNS.addConstant("Yaml1_2", new QoreBigIntNode(QYE_VER_1_2));

   // setup event map
   event_map[YAML_NO_EVENT] = "empty";
   event_map[YAML_STREAM_START_EVENT] = "stream-start";
   event_map[YAML_STREAM_END_EVENT] = "stream-end";
   event_map[YAML_DOCUMENT_START_EVENT] = "document-start";
   event_map[YAML_DOCUMENT_END_EVENT] = "document-end";
   event_map[YAML_ALIAS_EVENT] = "alias";
   event_map[YAML_SCALAR_EVENT] = "scalar";
   event_map[YAML_SEQUENCE_START_EVENT] = "sequence-start";
   event_map[YAML_SEQUENCE_END_EVENT] = "sequence-end";
   event_map[YAML_MAPPING_START_EVENT] = "mapping-start";
   event_map[YAML_MAPPING_END_EVENT] = "mapping-end";

   return 0;
}

void yaml_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(YNS.copy());
}

void yaml_module_delete() {
}

