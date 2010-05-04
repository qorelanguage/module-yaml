/*
  yaml Qore module

  Copyright (C) 2010 David Nichols, all rights reserved
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

static const char *YAML_EMITTER_ERROR = "YAML-EMITTER-ERROR";

static YAML::Emitter& operator << (YAML::Emitter& emitter, int64 v) {
   return emitter.WriteIntegralType(v);
}

static YAML::Emitter& operator << (YAML::Emitter& emitter, const DateTime &d) { 
   //if (d.isRelative()) {}
   qore_tm info;
   d.getInfo(info);

   // shorthand type name
   QoreString str("!!timestamp ");
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
   return emitter.WriteStreamable(str.getBuffer());
}

static YAML::Emitter& operator << (YAML::Emitter& emitter, const BinaryNode &b) {
   // shorthand type name
   QoreString str("!!binary \"");
   str.concatBase64(&b);
   str.concat('"');
   return emitter.WriteStreamable(str.getBuffer());
}

static int makeYAML(YAML::Emitter &out, const AbstractQoreNode *p, ExceptionSink *xsink) {
   qore_type_t t = get_node_type(p);
   switch (t) {
      case NT_STRING: {
	 TempEncodingHelper tmp(reinterpret_cast<const QoreStringNode *>(p), QCS_UTF8, xsink);
	 if (!tmp)
	    return -1;
	 out << tmp->getBuffer();
	 break;
      }	 
      case NT_INT: {
	 out << reinterpret_cast<const QoreBigIntNode *>(p)->val;
	 break;
      }
      case NT_FLOAT: {
	 out << reinterpret_cast<const QoreFloatNode *>(p)->f;
	 break;
      }
      case NT_BOOLEAN: {
	 out << reinterpret_cast<const QoreBoolNode *>(p)->getValue();
	 break;
      }
      case NT_LIST: {
	 out << YAML::BeginSeq;
	 ConstListIterator li(reinterpret_cast<const QoreListNode *>(p));
	 while (li.next()) {
	    if (makeYAML(out, li.getValue(), xsink))
	       return -1;
	 }
	 out << YAML::EndSeq;
	 break;
      }
      case NT_HASH: {
	 out << YAML::BeginMap;
	 ConstHashIterator hi(reinterpret_cast<const QoreHashNode *>(p));
	 while (hi.next()) {
	    out << YAML::Key << hi.getKey();
	    out << YAML::Value;
	    if (makeYAML(out, hi.getValue(), xsink))
	       return -1;
	 }
	 out << YAML::EndMap;
	 break;
      }
      case NT_DATE: {
	 out << *reinterpret_cast<const DateTimeNode *>(p);
	 break;
      }
      case NT_BINARY: {
	 out << *reinterpret_cast<const BinaryNode *>(p);
	 break;
      }
      case NT_NOTHING: {
	 out << YAML::Null;
	 break;
      }
      default:
	 xsink->raiseException(YAML_EMITTER_ERROR, "cannot convert Qore type '%s' to YAML", get_type_name(p));
	 break;
   }

   if (!out.good()) {
      xsink->raiseException(YAML_EMITTER_ERROR, out.GetLastError().c_str());
      return -1;
   }

   return 0;
}

static AbstractQoreNode *f_makeYAML(const QoreListNode *args, ExceptionSink *xsink) {
   YAML::Emitter out;
   out << YAML::Flow;

   unsigned i = 0;
   while (true) {
      const AbstractQoreNode *p = get_param(args, ++i);
      if (!p)
	 break;

      YAML::EMITTER_MANIP m = (YAML::EMITTER_MANIP)p->getAsInt();
      out << m;
   }


   if (makeYAML(out, get_param(args, 0), xsink))
      return 0;

   return new QoreStringNode(out.c_str(), QCS_UTF8);
}

QoreNamespace YNS("YAML");

QoreStringNode *yaml_module_init() {
   // add functions
   builtinFunctions.add2("makeYAML", f_makeYAML, QC_RET_VALUE_ONLY | QC_USES_EXTRA_ARGS, QDOM_DEFAULT, stringTypeInfo);

   // setup namespace
   YNS.addConstant("Auto", new QoreBigIntNode(YAML::Auto));
   YNS.addConstant("EmitNonAscii", new QoreBigIntNode(YAML::EmitNonAscii));
   YNS.addConstant("EscapeNonAscii", new QoreBigIntNode(YAML::EscapeNonAscii));
   YNS.addConstant("SingleQuoted", new QoreBigIntNode(YAML::SingleQuoted));
   YNS.addConstant("DoubleQuoted", new QoreBigIntNode(YAML::DoubleQuoted));
   YNS.addConstant("Literal", new QoreBigIntNode(YAML::Literal));
   YNS.addConstant("YesNoBool", new QoreBigIntNode(YAML::YesNoBool));
   YNS.addConstant("TrueFalseBool", new QoreBigIntNode(YAML::TrueFalseBool));
   YNS.addConstant("OnOffBool", new QoreBigIntNode(YAML::OnOffBool));
   YNS.addConstant("UpperCase", new QoreBigIntNode(YAML::UpperCase));
   YNS.addConstant("LowerCase", new QoreBigIntNode(YAML::LowerCase));
   YNS.addConstant("CamelCase", new QoreBigIntNode(YAML::CamelCase));
   YNS.addConstant("LongBool", new QoreBigIntNode(YAML::LongBool));
   YNS.addConstant("ShortBool", new QoreBigIntNode(YAML::ShortBool));
   YNS.addConstant("Dec", new QoreBigIntNode(YAML::Dec));
   YNS.addConstant("Hex", new QoreBigIntNode(YAML::Hex));
   YNS.addConstant("Oct", new QoreBigIntNode(YAML::Oct));
   YNS.addConstant("Flow", new QoreBigIntNode(YAML::Flow));
   YNS.addConstant("Block", new QoreBigIntNode(YAML::Block));

   return 0;
}


void yaml_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(YNS.copy());
}

void yaml_module_delete() {
}

