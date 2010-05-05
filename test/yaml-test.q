#!/usr/bin/env qore

%requires yaml

%exec-class yaml_test

class yaml_test {
    constructor() {
	my $d = (1, "two", 
		 NOTHING,
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
	printf("%s\n", $str);

	printf("parsed: %N\n", parseYAML($str));
    }
}
