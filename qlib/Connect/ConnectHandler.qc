# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file ConnectHandler.qc Connect protocol server handler

/*  ConnectHandler.qc Copyright 2026 Qore Technologies, s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

public namespace Connect {

#! Connect server method info hashdecl
/** @since Connect 1.0
*/
public hashdecl ConnectServerMethodInfo {
    #! Service name
    string service;
    #! Method name
    string method;
    #! Full path: /{service}/{method}
    string path;
    #! Handler callback
    code handler;
    #! RPC type (one of CONNECT_RPC_* constants)
    int rpc_type;
}

#! Connect RPC server handler for HttpServer
/** Integrates with HttpServer as an @ref AbstractHttpRequestHandler.
    Routes requests by URL path (<tt>/{service}/{method}</tt>) and dispatches
    to registered handler closures.

    Supports multiple codecs simultaneously via a codec registry keyed by name.
    Content-Type header determines which codec is used for each request.

    @par Example:
    @code{.py}
ConnectHandler handler();
handler.registerUnary("Greeter", "SayHello",
    auto sub (auto request, hash<auto> metadata) {
        return {"message": "Hello, " + request.name};
    });

# Add to HttpServer
http.setHandler("connect", "", NOTHING, handler);
    @endcode

    @since Connect 1.0
*/
public class ConnectHandler inherits HttpServer::AbstractHttpRequestHandler {
    private {
        #! Codec registry: codec name -> codec instance
        hash<string, AbstractConnectCodec> m_codecs;
        #! Default codec (used when no specific codec is requested)
        AbstractConnectCodec m_default_codec;
        #! Method registry: "service/method" -> method info
        hash<string, hash<ConnectServerMethodInfo>> m_methods;
        #! Enable debug logging
        bool m_debug = False;
        #! Maximum message size in bytes (0 = no limit)
        int m_max_message_size = CONNECT_DEFAULT_MAX_MESSAGE_SIZE;
    }

    #! Create a Connect handler
    /** @param auth optional authenticator (use new AbstractAuthenticator() for no auth)
        @param codec optional default codec (defaults to YamlConnectCodec)

        @since Connect 1.0
    */
    constructor(*HttpServer::AbstractAuthenticator auth, *AbstractConnectCodec codec)
            : HttpServer::AbstractHttpRequestHandler(auth) {
        if (!codec) {
            codec = new YamlConnectCodec();
        }
        m_default_codec = codec;
        m_codecs{codec.name()} = codec;
    }

    #! Register a unary RPC handler
    /** Handler signature: \c auto \c sub(auto \c request, hash<auto> \c metadata)

        @param service the service name (e.g., \c "Greeter")
        @param method the method name (e.g., \c "SayHello")
        @param handler the handler callback

        @since Connect 1.0
    */
    registerUnary(string service, string method, code handler) {
        string key = service + "/" + method;
        m_methods{key} = <ConnectServerMethodInfo>{
            "service": service,
            "method": method,
            "path": "/" + service + "/" + method,
            "handler": handler,
            "rpc_type": CONNECT_RPC_UNARY,
        };
    }

    #! Register a server-streaming RPC handler
    /** Handler signature: \c nothing \c sub(auto \c request,
        ConnectServerStream \c stream, hash<auto> \c metadata)

        @param service the service name
        @param method the method name
        @param handler the handler callback

        @since Connect 1.0
    */
    registerServerStream(string service, string method, code handler) {
        string key = service + "/" + method;
        m_methods{key} = <ConnectServerMethodInfo>{
            "service": service,
            "method": method,
            "path": "/" + service + "/" + method,
            "handler": handler,
            "rpc_type": CONNECT_RPC_SERVER_STREAM,
        };
    }

    #! Register a client-streaming RPC handler
    /** Handler signature: \c auto \c sub(ConnectServerStream \c stream,
        hash<auto> \c metadata)

        @param service the service name
        @param method the method name
        @param handler the handler callback

        @since Connect 1.0
    */
    registerClientStream(string service, string method, code handler) {
        string key = service + "/" + method;
        m_methods{key} = <ConnectServerMethodInfo>{
            "service": service,
            "method": method,
            "path": "/" + service + "/" + method,
            "handler": handler,
            "rpc_type": CONNECT_RPC_CLIENT_STREAM,
        };
    }

    #! Register a bidirectional streaming RPC handler
    /** Handler signature: \c nothing \c sub(ConnectServerStream \c stream,
        hash<auto> \c metadata)

        @param service the service name
        @param method the method name
        @param handler the handler callback

        @since Connect 1.0
    */
    registerBidiStream(string service, string method, code handler) {
        string key = service + "/" + method;
        m_methods{key} = <ConnectServerMethodInfo>{
            "service": service,
            "method": method,
            "path": "/" + service + "/" + method,
            "handler": handler,
            "rpc_type": CONNECT_RPC_BIDI,
        };
    }

    #! Add a codec to the registry
    /** @param codec the codec to add

        @since Connect 1.0
    */
    addCodec(AbstractConnectCodec codec) {
        m_codecs{codec.name()} = codec;
    }

    #! Set the default codec
    /** @param codec the default codec (also added to registry if not present)

        @since Connect 1.0
    */
    setDefaultCodec(AbstractConnectCodec codec) {
        m_default_codec = codec;
        if (!m_codecs{codec.name()}) {
            m_codecs{codec.name()} = codec;
        }
    }

    #! Enable or disable debug logging
    /** @param dbg True to enable debug logging

        @since Connect 1.0
    */
    setDebug(bool dbg = True) {
        m_debug = dbg;
    }

    #! Set the maximum message size
    /** @param size maximum message size in bytes; 0 means no limit

        @since Connect 1.0
    */
    setMaxMessageSize(int size) {
        m_max_message_size = size;
    }

    #! Get the maximum message size
    /** @return the maximum message size in bytes; 0 means no limit

        @since Connect 1.0
    */
    int getMaxMessageSize() {
        return m_max_message_size;
    }

    #! HttpServer handler entry point
    /** Routes the request based on URL path and Content-Type, then dispatches
        to the appropriate registered handler.

        @param listener the HTTP listener interface
        @param s the client socket
        @param cx the connection context hash
        @param hdr the HTTP request headers
        @param body the request body

        @return an HttpResponseInfo hash

        @since Connect 1.0
    */
    hash<HttpResponseInfo> handleRequest(HttpListenerInterface listener, Socket s,
            hash<auto> cx, hash<auto> hdr, *data body) {
        @debug {
            printf("ConnectHandler::handleRequest() method=%y path=%y ct=%y body_size=%d\n",
                hdr.method, hdr.path, hdr."content-type", body.size());
        }

        # Check for DataStream v1 backward compatibility (unary only)
        if (isDataStreamV1Request(hdr)) {
            return handleDataStreamV1Request(s, cx, hdr, body);
        }

        # Only POST is valid for Connect RPC
        if (hdr.method != "POST") {
            return makeErrorResponse(405, CONNECT_ERROR_UNIMPLEMENTED,
                sprintf("method %y not allowed; POST required", hdr.method));
        }

        # Extract service/method from path
        string path = hdr.path;
        # Remove query string if present
        int qpos = path.find("?");
        if (qpos >= 0) {
            path = path.substr(0, qpos);
        }
        # Remove leading slash
        if (path[0] == "/") {
            path = path.substr(1);
        }
        # Split into service/method
        int slash = path.find("/");
        if (slash < 0) {
            return makeErrorResponse(404, CONNECT_ERROR_UNIMPLEMENTED,
                sprintf("invalid path: %y; expected /{service}/{method}", hdr.path));
        }
        string service = path.substr(0, slash);
        string method_name = path.substr(slash + 1);
        # Remove any trailing slashes or path components after the method
        int slash2 = method_name.find("/");
        if (slash2 >= 0) {
            method_name = method_name.substr(0, slash2);
        }

        string handler_key = service + "/" + method_name;
        *hash<ConnectServerMethodInfo> method_info = m_methods{handler_key};
        if (!method_info) {
            return makeErrorResponse(404, CONNECT_ERROR_UNIMPLEMENTED,
                sprintf("unknown method: %s/%s", service, method_name));
        }

        # Parse Content-Type to determine codec and streaming mode
        string content_type = hdr."content-type" ?? "";
        *hash<auto> ct_info = ConnectHelper::parseContentType(content_type);

        # Resolve codec
        AbstractConnectCodec codec;
        if (ct_info) {
            *AbstractConnectCodec found = m_codecs{ct_info.codec_name};
            if (!found) {
                return <HttpResponseInfo>{
                    "code": 415,
                    "body": sprintf("unsupported codec: %y", ct_info.codec_name),
                    "hdr": {"Content-Type": "text/plain"},
                };
            }
            codec = found;
        } else {
            # No recognized content type — use default codec
            codec = m_default_codec;
        }

        # Determine if request is streaming based on content type
        bool streaming = ct_info ? ct_info.streaming : False;

        # Extract metadata from request headers
        hash<auto> metadata = extractMetadata(hdr);

        # Parse deadline from Connect-Timeout-Ms
        *date deadline;
        *string timeout_str = hdr{ConnectTimeoutHeader};
        if (timeout_str.val()) {
            deadline = now_us() + milliseconds(int(timeout_str));
        }

        # Dispatch based on RPC type and streaming mode
        try {
            if (!streaming) {
                # Unary request (plain HTTP POST)
                return handleUnary(s, cx, hdr, body, method_info, codec, metadata, deadline);
            }
            # Streaming request
            switch (method_info.rpc_type) {
                case CONNECT_RPC_SERVER_STREAM:
                    return handleServerStream(s, cx, hdr, body, method_info, codec, metadata,
                        deadline);
                case CONNECT_RPC_CLIENT_STREAM:
                    return handleClientStream(s, cx, hdr, body, method_info, codec, metadata,
                        deadline);
                case CONNECT_RPC_BIDI:
                    return handleBidiStream(s, cx, hdr, body, method_info, codec, metadata,
                        deadline);
                default:
                    # Unary via streaming content type is allowed
                    return handleUnary(s, cx, hdr, body, method_info, codec, metadata, deadline);
            }
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            return makeErrorResponse(500, CONNECT_ERROR_INTERNAL,
                sprintf("%s: %s", ex.err, ex.desc));
        }
    }

    #! Handle a unary RPC request
    private hash<HttpResponseInfo> handleUnary(Socket s, hash<auto> cx, hash<auto> hdr,
            *data body, hash<ConnectServerMethodInfo> method_info,
            AbstractConnectCodec codec, hash<auto> metadata, *date deadline) {
        # Deserialize request body
        auto request;
        if (body) {
            binary body_bin;
            if (body.typeCode() == NT_BINARY) {
                body_bin = body;
            } else {
                body_bin = binary(body);
            }
            if (body_bin.size() > 0) {
                if (m_max_message_size > 0 && body_bin.size() > m_max_message_size) {
                    return makeErrorResponse(413, CONNECT_ERROR_RESOURCE_EXHAUSTED,
                        sprintf("request body size %d exceeds maximum %d bytes",
                            body_bin.size(), m_max_message_size));
                }
                request = codec.deserialize(body_bin);
            }
        }

        # Call handler
        auto response;
        try {
            response = method_info.handler(request, metadata);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            return makeErrorResponse(500, CONNECT_ERROR_INTERNAL,
                sprintf("%s: %s", ex.err, ex.desc));
        }

        # Check deadline after handler returns
        if (deadline && now_us() > deadline) {
            return makeErrorResponse(408, CONNECT_ERROR_DEADLINE_EXCEEDED, "deadline exceeded");
        }

        # Serialize response
        binary response_body = codec.serialize(response);
        return <HttpResponseInfo>{
            "code": 200,
            "body": response_body,
            "hdr": {
                "Content-Type": ConnectContentTypePrefix + codec.name(),
            },
        };
    }

    #! Handle a server-streaming RPC request
    private hash<HttpResponseInfo> handleServerStream(Socket s, hash<auto> cx,
            hash<auto> hdr, *data body, hash<ConnectServerMethodInfo> method_info,
            AbstractConnectCodec codec, hash<auto> metadata, *date deadline) {
        # Determine transport
        bool http3 = cx."header-info".http3 ?? False;
        int stream_id = cx."header-info".stream_id ?? 0;
        *int session_id = cx."header-info".session_id;

        # Deserialize request from streaming frames
        auto request;
        if (body) {
            binary body_bin;
            if (body.typeCode() == NT_BINARY) {
                body_bin = body;
            } else {
                body_bin = binary(body);
            }
            if (body_bin.size() >= 5) {
                hash<ConnectFrameInfo> frame = ConnectHelper::unframeMessage(body_bin,
                    m_max_message_size);
                if (!(frame.flags & CONNECT_FLAG_END_STREAM)) {
                    request = codec.deserialize(frame.data);
                }
            }
        }

        # Create server stream for handler to write responses
        ConnectServerStream stream(s, codec, stream_id, http3, session_id, (),
            m_max_message_size);

        *hash<ConnectErrorInfo> error;
        try {
            method_info.handler(request, stream, metadata);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            error = <ConnectErrorInfo>{
                "code": CONNECT_ERROR_INTERNAL,
                "message": sprintf("%s: %s", ex.err, ex.desc),
            };
        }

        # Check deadline
        if (!error && deadline && now_us() > deadline) {
            error = <ConnectErrorInfo>{
                "code": CONNECT_ERROR_DEADLINE_EXCEEDED,
                "message": "deadline exceeded",
            };
        }

        # Ensure response headers are sent
        stream.ensureHeadersSent();

        # Send EndStreamResponse frame
        binary end_frame = ConnectHelper::frameEndStream(
            stream.getTrailingMetadata().size() > 0 ? stream.getTrailingMetadata() : NOTHING,
            error);
        if (http3) {
            while (s.submitQuicStreamData(session_id, stream_id, end_frame, True) == 1) {
                s.waitForQuicStreamDrain(session_id, stream_id, 5s);
            }
        } else {
            s.sendHttp2StreamData(stream_id, end_frame, True);
            s.flushHttp2();
        }

        bool is_multiplexed = http3 || (cx."header-info".http2 ?? False);
        return <HttpResponseInfo>{
            "reply_sent": True,
            "streaming": is_multiplexed,
        };
    }

    #! Handle a client-streaming RPC request
    private hash<HttpResponseInfo> handleClientStream(Socket s, hash<auto> cx,
            hash<auto> hdr, *data body, hash<ConnectServerMethodInfo> method_info,
            AbstractConnectCodec codec, hash<auto> metadata, *date deadline) {
        # Determine transport
        bool http3 = cx."header-info".http3 ?? False;
        int stream_id = cx."header-info".stream_id ?? 0;
        *int session_id = cx."header-info".session_id;

        # Parse all client messages from the buffered body
        list<auto> messages = parseStreamMessages(body, codec);

        # Create server stream with pre-parsed messages
        ConnectServerStream stream(s, codec, stream_id, http3, session_id, messages,
            m_max_message_size);

        auto response;
        *hash<ConnectErrorInfo> error;
        try {
            response = method_info.handler(stream, metadata);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            error = <ConnectErrorInfo>{
                "code": CONNECT_ERROR_INTERNAL,
                "message": sprintf("%s: %s", ex.err, ex.desc),
            };
        }

        # Check deadline
        if (!error && deadline && now_us() > deadline) {
            error = <ConnectErrorInfo>{
                "code": CONNECT_ERROR_DEADLINE_EXCEEDED,
                "message": "deadline exceeded",
            };
        }

        # Ensure headers sent
        stream.ensureHeadersSent();

        if (!error && exists response) {
            # Send the response as a framed message
            binary encoded = codec.serialize(response);
            binary framed = ConnectHelper::frameMessage(encoded);
            if (http3) {
                while (s.submitQuicStreamData(session_id, stream_id, framed, False) == 1) {
                    s.waitForQuicStreamDrain(session_id, stream_id, 5s);
                }
            } else {
                s.sendHttp2StreamData(stream_id, framed, False);
            }
        }

        # Send EndStreamResponse
        binary end_frame = ConnectHelper::frameEndStream(
            stream.getTrailingMetadata().size() > 0 ? stream.getTrailingMetadata() : NOTHING,
            error);
        if (http3) {
            while (s.submitQuicStreamData(session_id, stream_id, end_frame, True) == 1) {
                s.waitForQuicStreamDrain(session_id, stream_id, 5s);
            }
        } else {
            s.sendHttp2StreamData(stream_id, end_frame, True);
            s.flushHttp2();
        }

        bool is_multiplexed = http3 || (cx."header-info".http2 ?? False);
        return <HttpResponseInfo>{
            "reply_sent": True,
            "streaming": is_multiplexed,
        };
    }

    #! Handle a bidirectional-streaming RPC request
    private hash<HttpResponseInfo> handleBidiStream(Socket s, hash<auto> cx,
            hash<auto> hdr, *data body, hash<ConnectServerMethodInfo> method_info,
            AbstractConnectCodec codec, hash<auto> metadata, *date deadline) {
        # Determine transport
        bool http3 = cx."header-info".http3 ?? False;
        int stream_id = cx."header-info".stream_id ?? 0;
        *int session_id = cx."header-info".session_id;

        # Parse all client messages from the buffered body
        list<auto> messages = parseStreamMessages(body, codec);

        # Create server stream with pre-parsed messages
        ConnectServerStream stream(s, codec, stream_id, http3, session_id, messages,
            m_max_message_size);

        *hash<ConnectErrorInfo> error;
        try {
            method_info.handler(stream, metadata);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            error = <ConnectErrorInfo>{
                "code": CONNECT_ERROR_INTERNAL,
                "message": sprintf("%s: %s", ex.err, ex.desc),
            };
        }

        # Check deadline
        if (!error && deadline && now_us() > deadline) {
            error = <ConnectErrorInfo>{
                "code": CONNECT_ERROR_DEADLINE_EXCEEDED,
                "message": "deadline exceeded",
            };
        }

        # Ensure headers sent
        stream.ensureHeadersSent();

        # Send EndStreamResponse
        binary end_frame = ConnectHelper::frameEndStream(
            stream.getTrailingMetadata().size() > 0 ? stream.getTrailingMetadata() : NOTHING,
            error);
        if (http3) {
            while (s.submitQuicStreamData(session_id, stream_id, end_frame, True) == 1) {
                s.waitForQuicStreamDrain(session_id, stream_id, 5s);
            }
        } else {
            s.sendHttp2StreamData(stream_id, end_frame, True);
            s.flushHttp2();
        }

        bool is_multiplexed = http3 || (cx."header-info".http2 ?? False);
        return <HttpResponseInfo>{
            "reply_sent": True,
            "streaming": is_multiplexed,
        };
    }

    #! Check if this is a DataStream v1 request (backward compatibility)
    /** @param hdr the HTTP request headers
        @return True if the request uses DataStream v1 protocol (has DataStream-Accept header
        and no Connect protocol version header)

        @since Connect 1.0
    */
    private bool isDataStreamV1Request(hash<auto> hdr) {
        # DataStream v1 is indicated by DataStream-Accept header without Connect protocol header
        if (hdr{"datastream-accept"}.val() && !hdr{ConnectProtocolVersionHeader}.val()) {
            return True;
        }
        return False;
    }

    #! Handle a DataStream v1 request (unary only — chunked not supported)
    /** DataStream v1 chunked requests cannot be supported because the body is already
        reassembled by HttpServer. Only non-chunked (unary) requests are supported.

        @param s the socket
        @param cx the connection context
        @param hdr the HTTP headers
        @param body the reassembled request body

        @since Connect 1.0
    */
    private hash<HttpResponseInfo> handleDataStreamV1Request(Socket s, hash<auto> cx,
            hash<auto> hdr, *data body) {
        @debug {
            printf("ConnectHandler::handleDataStreamV1Request() path=%y\n", hdr.path);
        }

        # Reject chunked DataStream v1 requests — body has been reassembled, so per-chunk
        # encoding is lost. Only non-chunked (unary) requests are supported.
        if (hdr."transfer-encoding" == "chunked") {
            return <HttpResponseInfo>{
                "code": 400,
                "body": make_yaml({"err": "DATASTREAM-ERROR",
                    "desc": "chunked DataStream v1 is not supported by this handler; "
                        "use non-chunked requests or upgrade to Connect protocol"}),
                "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
            };
        }

        # Only POST is supported
        if (hdr.method != "POST") {
            return <HttpResponseInfo>{
                "code": 405,
                "body": make_yaml({"err": "DATASTREAM-ERROR",
                    "desc": sprintf("method %y not allowed; POST required", hdr.method)}),
                "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
            };
        }

        # Extract service/method from path
        string path = hdr.path;
        int qpos = path.find("?");
        if (qpos >= 0) {
            path = path.substr(0, qpos);
        }
        if (path[0] == "/") {
            path = path.substr(1);
        }
        int slash = path.find("/");
        if (slash < 0) {
            return <HttpResponseInfo>{
                "code": 404,
                "body": make_yaml({"err": "DATASTREAM-ERROR",
                    "desc": sprintf("invalid path: %y; expected /{service}/{method}",
                        hdr.path)}),
                "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
            };
        }
        string service = path.substr(0, slash);
        string method_name = path.substr(slash + 1);
        int slash2 = method_name.find("/");
        if (slash2 >= 0) {
            method_name = method_name.substr(0, slash2);
        }

        # Look up the handler
        string handler_key = service + "/" + method_name;
        *hash<ConnectServerMethodInfo> method_info = m_methods{handler_key};
        if (!method_info) {
            return <HttpResponseInfo>{
                "code": 404,
                "body": make_yaml({"err": "DATASTREAM-ERROR",
                    "desc": sprintf("unknown method: %s/%s", service, method_name)}),
                "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
            };
        }

        # Only unary handlers are supported via DataStream v1
        if (method_info.rpc_type != CONNECT_RPC_UNARY) {
            return <HttpResponseInfo>{
                "code": 400,
                "body": make_yaml({"err": "DATASTREAM-ERROR",
                    "desc": sprintf("method %s/%s is a streaming method; "
                        "DataStream v1 only supports unary methods", service, method_name)}),
                "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
            };
        }

        # Deserialize the YAML request body
        auto request;
        if (body) {
            string body_str;
            if (body.typeCode() == NT_BINARY) {
                body_str = body.toString("utf-8");
            } else {
                body_str = body;
            }
            if (body_str.val()) {
                request = parse_yaml(body_str);
            }
        }

        # Extract metadata
        hash<auto> metadata = extractMetadata(hdr);

        # Call the handler
        auto response;
        try {
            response = method_info.handler(request, metadata);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            return <HttpResponseInfo>{
                "code": 409,
                "body": make_yaml({"err": ex.err, "desc": ex.desc}),
                "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
                "close": True,
            };
        }

        # Serialize response as YAML
        return <HttpResponseInfo>{
            "code": 200,
            "body": make_yaml(response),
            "hdr": {"Content-Type": MimeTypeYaml + ";charset=utf8"},
        };
    }

    #! Create an error response for unary requests
    private hash<HttpResponseInfo> makeErrorResponse(int http_status, string error_code,
            string message) {
        string error_json = ConnectError::serializeError(<ConnectErrorInfo>{
            "code": error_code,
            "message": message,
        });
        return <HttpResponseInfo>{
            "code": http_status,
            "body": error_json,
            "hdr": {"Content-Type": "application/json"},
        };
    }

    #! Extract metadata from request headers
    /** Extracts custom headers (non-standard, non-protocol headers) as metadata
        to pass to handlers.
    */
    private hash<auto> extractMetadata(hash<auto> hdr) {
        hash<auto> metadata();
        map metadata{$1.key} = $1.value,
            hdr.pairIterator(),
            $1.key != "content-type"
                && $1.key != "content-length"
                && $1.key != "host"
                && $1.key != "method"
                && $1.key != "path"
                && $1.key != "http_version"
                && $1.key != "user-agent"
                && $1.key != "accept"
                && $1.key != "accept-encoding"
                && $1.key != "connection"
                && $1.key !~ /^:/
                && $1.key != ConnectProtocolVersionHeader
                && $1.key != ConnectTimeoutHeader;
        return metadata;
    }

    #! Parse streaming messages from a request body
    private list<auto> parseStreamMessages(*data body, AbstractConnectCodec codec) {
        list<auto> messages = ();
        if (!body) {
            return messages;
        }
        binary body_bin;
        if (body.typeCode() == NT_BINARY) {
            body_bin = body;
        } else {
            body_bin = binary(body);
        }
        if (body_bin.size() < 5) {
            return messages;
        }
        list<hash<ConnectFrameInfo>> frames = ConnectHelper::unframeMessages(body_bin,
            m_max_message_size);
        for (int i = 0; i < frames.size(); ++i) {
            if (frames[i].flags & CONNECT_FLAG_END_STREAM) {
                # End-of-stream frame — skip (it's the client's end marker)
                continue;
            }
            messages += codec.deserialize(frames[i].data);
        }
        return messages;
    }
}

} # namespace Connect
