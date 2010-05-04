#!/usr/bin/env qore

%requires yaml

%exec-class yaml_test

class yaml_test {
    constructor() {
	my $d = ( 1, 2, NOTHING, hash(), (), "three",
		  now_us(),
		  1970-01-01,
		  binary("hello"),
		  False,
		  ( "a" : 2.0, 
		    "b" : "hello",
		    "key" : True ) );
	my string $str = makeYAML($d, Flow, DoubleQuoted);
	printf("%s\n", $str);
    }
}
