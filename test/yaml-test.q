#!/usr/bin/env qore

%requires yaml

%exec-class yaml_test

class yaml_test {
    constructor() {
	my list $d = (1, "two", 
		      NOTHING,
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
		       "key" : True)
	    );

	my string $ystr = makeYAML($d, Yaml1_1);
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
		    #printf("ERROR %d: %s != %s\n", $i, dbg_node_info($l[$i]), dbg_node_info($d[$i]));
		    printf("ERROR %d: %s != %s\n", $i, $l[$i], $d[$i]);
		}
	    }
	}
    }
}
