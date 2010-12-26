# -*- mode: qore; indent-tabs-mode: nil -*-
/** @file YamlRpcClient.qc @brief provides the definition for the YamlRpcClient class
    @details YAML-RPC

    @anchor YAMLRPC

    YAML-RPC is an HTTP-based RPC protocol used by Qorus; it is not a standard protocol, however it is loosely based on JSON-RPC 1.1.

    YAML-RPC is the preferred protocol when communicating with Qorus because it is fast, it supports the best data serialization for Qore data of all RPC protocols supported by Qorus, and it is concise while still being very easy to read.  The major drawback is that it is a proprietary protocol, which is why XML-RPC and JSON-RPC will continue to be supported in the future alongside YAML-RPC.

    The information below provides technical information about how the Qorus-specific YAML-RPC protocol is implemented; details which are not normally visible when using the YamlRpcClient class.

    Request messages are sent as a YAML-encoded request string as the message body of an HTTP POST message.  The \c "Content-Type" header should be set to \c "application/x-yaml", otherwise there are no special restrictions on the message headers.

    YAML-RPC request strings are constructed as a simple YAML map with the following keys:
    - \c method: the string method name to call
    - \c params: any value giving the arguments for the method; if the method takes more than one argument, then the \c "params" key will hold a list of the arguments.   The YamlRpcClient class will always package a single argument in a list.

    Note that the Qorus YAML-RPC implementation uses the Qore YAML module to encode and decode YAML strings; this ensures maximum data fidelity when serializaing data; note that the Qore yaml module uses the proprietary \c !duration tag for durations, otherwise all other tags are standard YAML tags.  For details on how Qore data is serialized and deserialized by the yaml module, please see the yaml module documentation at <a href="http://qore.org/modules-mainmenu-31/38-yaml-module">yaml module</a>.

    The following is an example of a simple request message with headers:
    @code
POST YAML HTTP/1.1
Content-Type: application/x-yaml;charset=utf-8
Accept: application/x-yaml
User-Agent: Qore YAML-RPC Client v0.1
Accept-Encoding: deflate,gzip,bzip2
Connection: Keep-Alive
Host: localhost:8001
Content-Length: 49

{method: "omq.system.help", params: ["add-user"]}
    @endcode

    @anchor YAMLRPCResult

    Normal (non-error) responses are returned as a YAML map as well with the following key:
    - \c result: a map where the only key is the method name and the value is the value returned by the method.

    For example, for the message above, the response message would be:
    @code
HTTP/1.1 200 OK
Content-type: application/x-yaml;charset=utf-8
Server: Qorus HTTP Server v0.2.9
Connection: Keep-Alive
Date: Mon, 27 Sep 2010 10:34:06 GMT
Content-Encoding: deflate
Content-Length: 152

{result: {omq.system.add-user: {params: "username, password, descriptive_name, role_list, groups", category: "role based access control", maskargs: 1, description: "creates a new RBAC user"}}}
@endcode
    
    @anchor YAMLRPCError

    Error responses are also returned as a YAML map with a single key, \c error, which is itself a map with the following keys:
    - \c name: always \c "YAMLRPCError"
    - \c code: an integer error code
    - \c message: an error message
    - \c error: an optional error value of any format

    The following is an example error response message:
    @code
HTTP/1.1 200 OK
Content-type: application/x-yaml;charset=utf-8
Server: Qorus HTTP Server v0.2.9
Connection: Keep-Alive
Date: Mon, 27 Sep 2010 10:38:06 GMT
Content-Encoding: deflate
Content-Length: 118

{error: {name: "YAMLRPCError", code: 105, message: "YAML-RPC-SERVER-UNKNOWN-METHOD: unknown method \"omq.system.none\""}}
    @endcode
        
*/

# this file is automatically included by the Qorus client library and should not be included directly by Qorus client programs

#! defines a YAML-RPC client class; normally the OMQ::QorusSystemAPIHelper class should be used to communicate with remote Qorus servers instead
/** The YamlRpcClient class will correctly serialize YAML-RPC messages, send them to the server, deserialize the responses and return a Qore-language data structure for the response.  

    YAML-RPC is a proprietary HTTP-based RPC protocol of Qorus Integration Engine; for a description of the YAML-RPC protocol implemented by the YamlRpcClient class, see @ref YAMLRPC "YAML-RPC Protocol Implementation".\n

    @note subclassed from Qore''s HTTPClient class
    @note to communicate with Qorus servers, normally the OMQ::QorusSystemAPIHelper class should be used instead.  At the time this documentation was written, Qorus servers are the only servers that can understand this protocol, and the OMQ::QorusSystemAPIHelper class provides a much user-friendlier interface for RPC communication to Qorus servers than this class; the OMQ::QorusSystemAPIHelper class uses this class to communicate if the remote server is new enough to implement this protocol.
  */
class YamlRpcClient inherits HTTPClient {
    #! YAML-RPC Client Version
    const Version = "0.1";

    #! default options for the YamlRpcClient::constructor()
    const DefaultOptions = ( "protocols" :
			     ( "yamlrpc"  : ( "port" : 80,  "ssl" : False ),
			       "yamlrpcs" : ( "port" : 443, "ssl" : True ) ),
			     "default_path" : "YAML" );

    #! YAML-RPC CLient Version String
    const VersionString = sprintf("Qore YAML-RPC Client v%s", YamlRpcClient::Version);

    #! default HTTP headers
    const DefaultHeaders = ( "Content-Type" : "application/x-yaml",
			     "Accept"       : "application/x-yaml",
			     "User-Agent"   : YamlRpcClient::VersionString);

    private {
	hash $.headers;
	int $.flags = YAML::None;
	string $.path;
    }
    
    #! calls the base class HTTPClient constructor, overrides the "protocols" key to "yamlrpc"
    /** @param $opts valid options are:
    - \c url: A string giving the URL to connect to
        - \c default_port: The default port number to connect to if none is given in the URL
        - \c http_version: Either '1.0' or '1.1' for the claimed HTTP protocol version compliancy in outgoing message headers
        - \c default_path: The default path to use for new connections if a path is not otherwise specified in the connection URL
        - \c max_redirects: The maximum number of redirects before throwing an exception (the default is 5)
        - \c proxy: The proxy URL for connecting through a proxy
        - \c timeout: The timeout value in milliseconds (also can be a relative date-time value for clarity, ex: 5m)
        - \c connect_timeout: The timeout value in milliseconds for establishing a new socket connection (also can be a relative date-time value for clarity, ex: 30s)
        @param $do_not_connect if \c False (the default), then a connection will be immediately established to the remote server
        */
    constructor(hash $opts = hash(), bool $do_not_connect = False) : HTTPClient(YamlRpcClient::DefaultOptions + $opts) {
	if (!$do_not_connect)
	    $.connect();

	$.headers = YamlRpcClient::DefaultHeaders + $opts.headers;
	$.flags |= $opts.flags;
	
	# set path
	my any $path = parse_url($.getURL()).path;
	if (exists $path)
	    $.path = $path;
	else
	    $.path = exists $opts.default_path ? $opts.default_path : DefaultOptions.default_path;
    }

    #! simple constructor using default arguments
    /** @param $do_not_connect if \c False (the default), then a connection will be immediately established to the remote server
        */
    constructor(bool $do_not_connect = False) : HTTPClient(YamlRpcClient::DefaultOptions) {
	if (!$do_not_connect)
	    $.connect();
    }

    #! makes a call to the YAML-RPC server using the second argument as the list of arguments to send
    /** @param $method the remote method to call
        @param $args the argument(s) to the call
        @return the value returned from the server; if an error response is returned, an exception is raised
    */
    any callArgs(string $method, any $args) {
	# create outgoing message in YamlRpc Call format
	my string $msg = YamlRpcClient::makeRequest($method, $args, $.flags);
	# send the message to the server and get the response as an YAML string
	my string $ans = $.post($.path, $msg, $.headers);
	# parse the response and return a qore data structure
	return parseYAML($ans);
    }

    #! makes a call to the YAML-RPC server using the third argument as the list of arguments to send
    /** @param $info a reference to a hash with technical information about the call
        @param $method the remote method to call
        @param $args the argument(s) to the call
        @return the value returned from the server; if an error response is returned, an exception is raised
    */
    any callArgsWithInfo(reference $info, string $method, any $args) {
	# create outgoing message in YamlRpc Call format
	my string $msg = YamlRpcClient::makeRequest($method, $args, $.flags);
	trim $msg;
	on_exit $info.request = $msg;

	# send the message to the server and get the response as an YAML string
	$info.response_headers = $.send($msg, "POST", $.path, $.headers, False, \$info);

	$info.response = remove $info.response_headers.body;
	trim $info.response;

	# parse the response and return a qore data structure
	return parseYAML($info.response);
    }

    #! makes a method call to the YAML-RPC server using the remaining arguments after the method name as the list of arguments to send
    /** @param $method the remote method to call
        @return the value returned from the server; if no error occurred, then a hash is returned with a \c result key (see @ref YAMLRPCResult "YAML-RPC Response"), otherwise a hash is returned with an \c error key containing the error information (see @ref YAMLRPCError "YAML-RPC Error")
    */
    any call(string $method) {
	return $.callArgs($method, $argv);
    }

    #! makes a YAML-RPC request string
    static string makeRequest(string $method, any $arg, int $flags = YAML::None) {
	return makeYAML(("method":$method,"params":$arg), $flags);
    }
}
