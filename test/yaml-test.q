#!/usr/bin/env qore

%requires yaml

%exec-class yaml_test

class yaml_test {
    constructor() {
	my $d = (1, "two", 
		 NOTHING,
		 2010-05-05T15:35:02.100,
		 False,
		 1970-01-01Z,
		 #(hash(), (), "three \"things\""),
		 now_us(),
		 binary("hello"),
		 #("a" : 2.0, 
		 # "b" : "hello",
		 # "key" : True)
	    );
	my string $str = makeYAML($d);
	printf("%s", $str);
	printf("parsed: %N\n", parseYAML($str));
    }
}
