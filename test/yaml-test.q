#!/usr/bin/env qore
# -*- mode: qore; indent-tabs-mode: nil -*-

%requires yaml

# the yaml module requires qore 0.8.0+, so we know we have types
%require-types

%requires qore >= 0.8.6

%exec-class yaml_test

# the current libyaml version can only output YAML 1.1
const opts = (
    "canon"  : "canonical,c",
    #"yaml10" : "yaml10,o",
    #"yaml12" : "yaml12,n",
    "len"    : "length,l=i",
    "indent" : "indent,i=i",
    "help"   : "help,h"
    );

const def_len = -1;
const def_indent = 2;

class yaml_test {
    constructor() {
	my int $len = def_len;
	my int $indent = def_indent;

	my GetOpt $g(opts);
	my hash $o = $g.parse2(\$ARGV);
	if ($o.help)
	    $.usage();

	my int $opts = YAML::Yaml1_1;
	#if ($o.yaml12)
	#    $opts = YAML::Yaml1_2;
	#else if ($o.yaml10)
	#    $opts = YAML::Yaml1_0;

	if ($o.canon)
	    $opts |= YAML::Canonical;

	if (exists $o.len)
	    $len = $o.len;

	if (exists $o.indent)
	    $indent = $o.indent;

	my list $d = (
            1, "two", 
            NOTHING,
            0,
            0.0,
            22,
            9223372036854775807,
            -9223372036854775807,
            500n,
            M_PIn,
            2.141578291e50n,
            2010-05-05T15:35:02.100,
            False,
            M_PI,
            250.192393,
            1970-01-01Z,
            (hash(), (), "three \"things\""),
            P2M3DT10H14u,
            now_us(),
            binary("hello, how's it going, this is a long string, you know XXXXXXXXXXXXXXXXXXXXXXXX"),
            ("a" : 2.0, 
             "b" : "hello",
             "key" : True),
            @nan@n,
            @inf@n,
            -@inf@n,
            @inf@,
            -@inf@,
	    );

	my string $ystr = makeYAML($d, $opts, $o.len, $o.indent);
	#my string $xstr = makeXMLString(("data":$d));
	#my string $jstr = makeJSONString($d);
	#printf("ystr: %d, xstr: %d, jstr: %d\n", strlen($ystr), strlen($xstr), strlen($jstr));
	printf("orig=%n\n", $d);
	printf("%s", $ystr);
	my list $l = parseYAML($ystr);
	printf("parsed: %n\n", $l);
	printf("equal: %n\n", $l === $d);

	if ($l !== $d) {
	    for (my int $i = 0; $i < elements $l; ++$i) {
		if ($l[$i] !== $d[$i]) {
		    printf("ERROR %d: %s != %s\n", $i, $.getValStr($l[$i]), $.getValStr($d[$i]));
		}
	    }
	}
    }

    string getValStr(any $v) {
        if ($v.typeCode() == NT_NUMBER)
            return sprintf("%y (prec %d)", $v, $v.prec());
        return sprintf("%y", $v);
    }

    static usage() {
	printf("usage: %s [options]
 -c,--canonical   emit canonical YAML output
 -l,--line=ARG    line length (default: %d)
 -i,--indent=ARG  number of spaces when indenting (default: %d)
 -h,--help        this help text
", get_script_name(), def_len, def_indent);
      exit(1);
   }
}
