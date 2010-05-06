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
		 (hash(), (), "three \"things\""),
		 now_us(),
		 binary("hello, how's it going, this is a long string, you know XXXXXXXXXXXXXXXXXXXXXXXX"),
		 ("a" : 2.0, 
		  "b" : "hello",
		  "key" : True)
	    );
	my string $ystr = makeYAML($d, Yaml1_1);
	#my string $xstr = makeXMLString(("data":$d));
	#my string $jstr = makeJSONString($d);
	#printf("ystr: %d, xstr: %d, jstr: %d\n", strlen($ystr), strlen($xstr), strlen($jstr));
	printf("%s", $ystr);
	printf("parsed: %N\n", parseYAML($ystr));
    }
}
