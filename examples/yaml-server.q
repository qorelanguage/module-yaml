#!/usr/bin/env qore
# -*- mode: qore; indent-tabs-mode: nil -*-

%require-our

%requires HttpServer
%requires YamlRpcHandler
%requires Mime

# our API - we can only shut down the server
const ApiMethods = (
     ("name": "^sys\\.shutdown\$",
      "text": "sys.shutdown",
      "function": sub () { background $http.stop();  return "OK"; },
      "help": "shuts down this server",
      "logopt": 0,
      ),
    );

# a logging closure
my code $log = sub (string $str) {printf("%y: %s\n", now_us(), $str);};

# our bind address
const Bind = 8888;

# create the handler
my YamlRpcHandler $yamlRpcHandler(new AbstractAuthenticator(), ApiMethods);

# create the HTTP server
our HttpServer $http($log, $log);

# set the handler
$http.setHandler("yamlrpc", "", MimeTypeYamlRpc, $yamlRpcHandler);

# set it as the default (it is anyway because it's the only one)
$http.setDefaultHandler("yamlrpc", $yamlRpcHandler);

# start a listener
$http.addListener(8888);

# log an interesting message
$log("now listening on %s\n", Bind);

# wait for the server to stop
$http.waitStop();
