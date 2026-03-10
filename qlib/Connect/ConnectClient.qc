# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file ConnectClient.qc Connect protocol client

/*  ConnectClient.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Connect client options hashdecl
/** @since Connect 1.0
*/
public hashdecl ConnectClientOptions {
    #! TLS options (hash passed to connection manager)
    *hash<auto> ssl;
    #! Maximum message size in bytes (0 = no limit)
    *int max_message_size;
    #! Accept all server certificates (for testing)
    *bool accept_all_certs;
    #! HTTP protocol: "auto" (default, try HTTP/3 first then HTTP/2), "h2", or "h3"
    *string protocol;
}

} # namespace Connect

%ifndef NoHttpClientIo
%ifndef NoAsyncSocketIo

public namespace Connect {

#! Connect RPC client
/** Makes Connect RPC calls to a remote server using HTTP/2 and HTTP/3 via
    HttpClientIo connection pooling.

    Supports all four RPC patterns: unary, server-streaming, client-streaming,
    and bidirectional streaming.

    @par Example (unary call):
    @code{.py}
ConnectClient client("http://localhost:8080");
auto response = client.call("Greeter", "SayHello", {"name": "World"});
printf("Response: %y\n", response);
client.shutdown();
    @endcode

    @par Example (server-streaming):
    @code{.py}
ConnectClientStream stream = client.serverStream("Events", "Subscribe",
    {"topic": "news"});
while (*auto msg = stream.read()) {
    printf("Event: %y\n", msg);
}
    @endcode

    @since Connect 1.0
*/
public class ConnectClient {
    private {
        #! Base URL
        string m_url;
        #! Codec for serialization
        AbstractConnectCodec m_codec;
        #! HTTP/2 connection manager
        HttpClientIo::HttpClientConnectionManager m_mgr;
        #! Async I/O controller
        AsyncSocketIo::AsyncSocketIoController m_controller(False);
        #! Maximum message size in bytes (0 = no limit)
        int m_max_message_size;
    }

    #! Create a Connect client
    /** @param url base URL (e.g., \c "https://example.com:8080")
        @param codec serialization codec (default: @ref YamlConnectCodec)
        @param opts connection options

        @since Connect 1.0
    */
    constructor(string url, *AbstractConnectCodec codec, *hash<ConnectClientOptions> opts) {
        m_url = url;
        if (!codec) {
            codec = new YamlConnectCodec();
        }
        m_codec = codec;
        m_max_message_size = opts.max_message_size ?? CONNECT_DEFAULT_MAX_MESSAGE_SIZE;

        hash<HttpClientIo::HttpClientConnectionManagerOptions> mgr_opts();
        if (url =~ /^https:/) {
            if (opts.accept_all_certs ?? True) {
                mgr_opts.accept_all_certs = True;
            } else if (opts.ssl) {
                mgr_opts.ssl_verify_mode = SSL_VERIFY_PEER;
            }
        }
        if (opts.protocol) {
            mgr_opts.protocol = opts.protocol;
        }

        m_mgr = new HttpClientIo::HttpClientConnectionManager(mgr_opts);
        m_mgr.setController(m_controller);
        m_controller.start();
    }

    #! Unary RPC: single request, single response
    /** @param service the service name (e.g., \c "Greeter")
        @param method the method name (e.g., \c "SayHello")
        @param request the request data
        @param opts optional call options (timeout, metadata)

        @return the deserialized response

        @throw CONNECT-ERROR if the server returns an error

        @since Connect 1.0
    */
    auto call(string service, string method, auto request, *hash<ConnectCallOptions> opts) {
        # Serialize request
        binary payload = m_codec.serialize(request);

        # Build headers
        hash<string, string> headers = ConnectHelper::buildUnaryRequestHeaders(m_codec.name(),
            opts);

        # Make request
        string path = "/" + service + "/" + method;
        HttpClientIo::HttpClientStreamHandle stream = m_mgr.acquireStream(m_url);
        hash<auto> response;
        try {
            response = stream.request("POST", path, headers, payload);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                rethrow;
            }
            throw "CONNECT-ERROR",
                sprintf("Connect call to %s failed: %s: %s", path, ex.err, ex.desc);
        }

        # Check HTTP status
        int status = response.status_code ?? 200;
        if (status >= 400) {
            # Try to parse JSON error body
            *hash<ConnectErrorInfo> err;
            try {
                if (response.body) {
                    string body_str;
                    if (response.body.typeCode() == NT_BINARY) {
                        body_str = response.body.toString("utf-8");
                    } else {
                        body_str = response.body;
                    }
                    err = ConnectError::parseError(body_str);
                }
            } catch (hash<ExceptionInfo> ex) {
                if (ex.err == "THREAD-CANCELLED" || ex.err == "PROGRAM-INTERRUPTED") {
                    rethrow;
                }
                # Ignore parse errors — construct error from HTTP status
            }
            if (!err) {
                err = <ConnectErrorInfo>{
                    "code": ConnectError::errorForHttpStatus(status),
                    "message": sprintf("HTTP %d", status),
                };
            }
            throw "CONNECT-ERROR", err.message ?? err.code, err;
        }

        # Deserialize response body
        if (response.body) {
            binary body_bin;
            if (response.body.typeCode() == NT_BINARY) {
                body_bin = response.body;
            } else {
                body_bin = binary(response.body);
            }
            if (body_bin.size() > 0) {
                if (m_max_message_size > 0 && body_bin.size() > m_max_message_size) {
                    throw "CONNECT-MESSAGE-SIZE-ERROR",
                        sprintf("unary response size %d exceeds maximum %d bytes",
                            body_bin.size(), m_max_message_size);
                }
                return m_codec.deserialize(body_bin);
            }
        }
        return NOTHING;
    }

    #! Server streaming: single request, stream of responses
    /** @param service the service name
        @param method the method name
        @param request the request data
        @param opts optional call options

        @return a @ref ConnectClientStream for reading responses

        @since Connect 1.0
    */
    ConnectClientStream serverStream(string service, string method, auto request,
            *hash<ConnectCallOptions> opts) {
        # Serialize and frame the request (compress if encoding specified)
        binary encoded = m_codec.serialize(request);
        int flags = 0;
        if (opts.encoding) {
            encoded = ConnectCompression::compress(encoded, opts.encoding);
            flags = CONNECT_FLAG_COMPRESSED;
        }
        binary framed = ConnectHelper::frameMessage(encoded, flags);

        # Build streaming headers
        hash<string, string> headers = ConnectHelper::buildStreamRequestHeaders(
            m_codec.name(), opts);

        # Start streaming
        string path = "/" + service + "/" + method;
        HttpClientIo::HttpClientStreamHandle stream = m_mgr.acquireStream(m_url);
        ensureStreamReady(stream);
        stream.startStreaming("POST", path, headers);

        # Send request with END_STREAM (client has one message)
        stream.sendData(framed, True);

        # Writes are done; recv_encoding is the same encoding we advertised
        return new ConnectClientStream(stream, m_codec, True, m_max_message_size,
            NOTHING, opts.encoding);
    }

    #! Client streaming: stream of requests, single response
    /** @param service the service name
        @param method the method name
        @param opts optional call options

        @return a @ref ConnectClientStream for writing requests and reading the response

        @since Connect 1.0
    */
    ConnectClientStream clientStream(string service, string method,
            *hash<ConnectCallOptions> opts) {
        # Build streaming headers
        hash<string, string> headers = ConnectHelper::buildStreamRequestHeaders(
            m_codec.name(), opts);

        # Start streaming (no request body yet)
        string path = "/" + service + "/" + method;
        HttpClientIo::HttpClientStreamHandle stream = m_mgr.acquireStream(m_url);
        ensureStreamReady(stream);
        stream.startStreaming("POST", path, headers);

        return new ConnectClientStream(stream, m_codec, False, m_max_message_size,
            opts.encoding, opts.encoding);
    }

    #! Bidirectional streaming: stream of requests and responses
    /** @param service the service name
        @param method the method name
        @param opts optional call options

        @return a @ref ConnectClientStream for reading and writing

        @since Connect 1.0
    */
    ConnectClientStream bidiStream(string service, string method,
            *hash<ConnectCallOptions> opts) {
        return clientStream(service, method, opts);
    }

    #! Shut down the client and release resources
    /** @since Connect 1.0
    */
    shutdown() {
        m_controller.stop();
    }

    #! Destructor — ensures resources are released
    /** @since Connect 1.0
    */
    destructor() {
        m_controller.stop();
    }

    #! Ensure the HTTP connection is ready for streaming
    private ensureStreamReady(HttpClientIo::HttpClientStreamHandle stream,
            timeout timeout_ms = 30s) {
        HttpClientIo::HttpClientConnection conn = stream.getConnection();
        if (!conn.isReady()) {
            if (conn.isControllerMode()) {
                if (!conn.waitForReady(timeout_ms)) {
                    throw "CONNECT-ERROR", "HTTP connection establishment timed out";
                }
            } else {
                if (!conn.driveToReady(timeout_ms)) {
                    throw "CONNECT-ERROR", "HTTP connection establishment timed out";
                }
            }
        }
    }
}

} # namespace Connect

%endif
%endif
