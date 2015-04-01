#!/usr/bin/env qore
# -*- mode: qore; indent-tabs-mode: nil -*-

%requires yaml

# require type declarations
%require-types

%requires qore >= 0.8.12

%requires UnitTest
    
# the current libyaml version can only output YAML 1.1
const opts = (
    "canon"  : "canonical,c",
    #"yaml10" : "yaml10,o",
    #"yaml12" : "yaml12,n",
    "len"    : "length,l=i",
    "indent" : "I,indent=i",
    "help"   : "help,h"
    );

const def_len = -1;
const def_indent = 2;

our UnitTest $unit(\$ARGV, UnitTest::Opts + opts);

yaml_test();

sub yaml_test() {
    my int $len = def_len;
    my int $indent = def_indent;

    my hash $o = $unit.options();
    
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

    my list $l = parseYAML($ystr);

    $unit.cmp($l, $d, "yaml-1");

    /*
    if ($l !== $d) {
        for (my int $i = 0; $i < elements $l; ++$i) {
            if ($l[$i] !== $d[$i]) {
                printf("ERROR %d: %s != %s\n", $i, get_val_str($l[$i]), get_val_str($d[$i]));
            }
        }
    }
    */

    my TimeZone $tz("Europe/Vienna");
    my date $dt1 = 2015-03-29T01:00:00Z;
    my date $dt2 = $tz.date($dt1);
    $unit.cmp($dt1, $dt2, "date-1");
    my date $dt3 = parseYAML(makeYAML($dt1));
    $unit.cmp($dt2, $dt3, "yaml-date-1");
}

string sub get_val_str(any $v) {
    if ($v.typeCode() == NT_NUMBER)
        return sprintf("%y (prec %d)", $v, $v.prec());
    return sprintf("%y", $v);
}

sub usage() {
    printf("usage: %s [options]
 -c,--canonical   emit canonical YAML output
 -l,--line=ARG    line length (default: %d)
 -I,--indent=ARG  number of spaces when indenting (default: %d)
 -h,--help        this help text
", get_script_name(), def_len, def_indent);
    exit(1);
}
