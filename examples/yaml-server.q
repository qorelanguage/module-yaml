#!/usr/bin/env qore
# -*- mode: qore; indent-tabs-mode: nil -*-

%require-our
%new-style
%require-types
%strict-args

%requires HttpServer
%requires YamlRpcHandler
%requires Mime
%requires Logger

# our API - we can only shut down the server
const ApiMethods = (
    {
        "name": "^sys\\.shutdown\$",
        "text": "sys.shutdown",
        "function": string sub () { background http.stop(); return "OK"; },
        "help": "shuts down this server",
        "logopt": 0,
    },
);

# default bind address
const Bind = 8888;

# our logger
Logger logger("YAML-server", LoggerLevel::getLevelInfo());
logger.addAppender(new StdoutAppender());

code log = sub (string str) {printf("%y: %s\n", now_us(), vsprintf(str, argv));};

# create the handler
YamlRpcHandler yamlRpcHandler(new AbstractAuthenticator(), ApiMethods);

# create the HTTP server
our HttpServer http(<HttpServerOptionInfo>{
    "logger": logger,
});

# set the handler
http.setHandler("yamlrpc", "", MimeTypeYamlRpc, yamlRpcHandler);

# set it as the default (it is anyway because it's the only one)
http.setDefaultHandler("yamlrpc", yamlRpcHandler);

# start a listener
softint bind = shift ARGV ?? Bind;
http.addListener(bind);

# log an interesting message
log("now listening on %s", bind);

# wait for the server to stop
http.waitStop();
