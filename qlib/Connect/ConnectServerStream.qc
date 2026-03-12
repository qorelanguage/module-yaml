# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file ConnectServerStream.qc Connect server-side stream handle

/*  ConnectServerStream.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Server-side Connect stream handle for streaming RPC patterns
/** Provides framed message reading and writing on the server side.
    Used for server-streaming, client-streaming, and bidirectional-streaming
    handler implementations.

    Two operational modes:
    - <b>Buffered</b>: for server-streaming where the client request has already
      been fully received.  Pre-parsed client messages are returned by read().
    - <b>Live</b>: for client-streaming and bidirectional-streaming where client
      messages arrive incrementally via the HTTP/2 or HTTP/3 stream.

    @par Example (server-streaming handler):
    @code{.py}
handler.registerServerStream("MyService", "ServerStream",
    sub(auto request, ConnectServerStream stream, hash<auto> metadata) {
        for (int i = 0; i < 5; ++i) {
            stream.write({"message": sprintf("msg %d", i)});
        }
    });
    @endcode

    @par Example (client-streaming handler):
    @code{.py}
handler.registerClientStream("MyService", "ClientStream",
    auto sub(ConnectServerStream stream, hash<auto> metadata) {
        list<string> names;
        while (*auto msg = stream.read()) {
            names += msg.name;
        }
        return {"names": names};
    });
    @endcode
*/
public class ConnectServerStream {
    private {
        #! The underlying Socket
        Socket m_socket;
        #! The codec for serialization/deserialization
        AbstractConnectCodec m_codec;
        #! The HTTP/2 stream ID
        int m_stream_id;
        #! Whether this is an HTTP/3 connection
        bool m_http3;
        #! HTTP/3 session ID (HTTP/3 only)
        *int m_session_id;
        #! Read buffer for accumulating partial frames (live mode)
        binary m_read_buf = binary();
        #! Whether all client messages have been consumed
        bool m_reads_done = False;
        #! Whether response headers have been sent
        bool m_headers_sent = False;
        #! Live mode: read from socket incrementally (vs pre-parsed buffer)
        bool m_live = False;
        #! Pre-parsed messages for buffered mode
        list<auto> m_messages = ();
        #! Current read position in pre-parsed messages
        int m_read_pos = 0;
        #! Custom trailing metadata set by the handler
        hash<auto> m_trailing_metadata();
        #! Maximum message size in bytes (0 = no limit)
        int m_max_message_size;
        #! Compression encoding for outgoing messages (server→client)
        *string m_send_encoding;
        #! Compression encoding for incoming messages (client→server)
        *string m_recv_encoding;
    }

    #! Create a server stream in buffered mode
    /** @param socket the client Socket
        @param codec the codec for message serialization
        @param stream_id the HTTP/2 stream ID
        @param http3 whether this is an HTTP/3 connection
        @param session_id the HTTP/3 session ID (only used for HTTP/3)
        @param messages pre-parsed list of client messages
        @param max_message_size maximum message size in bytes (0 = no limit);
        defaults to @ref CONNECT_DEFAULT_MAX_MESSAGE_SIZE
        @param send_encoding optional compression encoding for outgoing messages
        (server→client); when set, all written messages are compressed
        @param recv_encoding optional compression encoding for incoming messages
        (client→server); when set, compressed frames from the client are decompressed
    */
    constructor(Socket socket, AbstractConnectCodec codec, int stream_id,
            bool http3, *int session_id, list<auto> messages = (),
            int max_message_size = CONNECT_DEFAULT_MAX_MESSAGE_SIZE,
            *string send_encoding, *string recv_encoding) {
        m_socket = socket;
        m_codec = codec;
        m_stream_id = stream_id;
        m_http3 = http3;
        m_session_id = session_id;
        m_messages = messages;
        m_max_message_size = max_message_size;
        m_send_encoding = send_encoding;
        m_recv_encoding = recv_encoding;
    }

    #! Create a server stream in live mode
    /** @param socket the client Socket
        @param codec the codec for message serialization
        @param stream_id the HTTP/2 stream ID
        @param http3 whether this is an HTTP/3 connection
        @param session_id the HTTP/3 session ID (only used for HTTP/3)
        @param live if True, read from the stream incrementally
        @param max_message_size maximum message size in bytes (0 = no limit);
        defaults to @ref CONNECT_DEFAULT_MAX_MESSAGE_SIZE
        @param send_encoding optional compression encoding for outgoing messages
        @param recv_encoding optional compression encoding for incoming messages
    */
    constructor(Socket socket, AbstractConnectCodec codec, int stream_id,
            bool http3, *int session_id, bool live,
            int max_message_size = CONNECT_DEFAULT_MAX_MESSAGE_SIZE,
            *string send_encoding, *string recv_encoding) {
        m_socket = socket;
        m_codec = codec;
        m_stream_id = stream_id;
        m_http3 = http3;
        m_session_id = session_id;
        m_live = live;
        m_max_message_size = max_message_size;
        m_send_encoding = send_encoding;
        m_recv_encoding = recv_encoding;
    }

    #! Read the next client message
    /** For client-streaming and bidirectional-streaming handlers.

        In buffered mode, returns pre-parsed messages one at a time.
        In live mode, blocks until a complete Connect frame arrives from the
        stream, the stream completes (END_STREAM), or the timeout expires.

        @param timeout_ms read timeout (default 30s); only used in live mode

        @return the next deserialized message, or NOTHING when all messages
        have been read
    */
    *auto read(*timeout timeout_ms) {
        if (!m_live) {
            # Buffered mode
            if (m_read_pos >= m_messages.size()) {
                return NOTHING;
            }
            return m_messages[m_read_pos++];
        }

        # Live mode: read from HTTP/2 or HTTP/3 stream incrementally
        if (m_reads_done) {
            return NOTHING;
        }
        if (!timeout_ms) {
            timeout_ms = 30s;
        }
        date deadline = now_us() + milliseconds(timeout_ms);

        while (True) {
            # Try to parse a complete Connect frame from buffer
            *auto msg = tryParseFrame();
            if (exists msg) {
                return msg;
            }

            # Check if reads are done (set by tryParseFrame on end-stream)
            if (m_reads_done) {
                return NOTHING;
            }

            # Check if stream is complete (END_STREAM received)
            if (isStreamComplete()) {
                # Drain any remaining data
                *binary remaining = readStreamData(0ms);
                if (remaining) {
                    m_read_buf += remaining;
                }
                # Try to parse remaining buffered data
                *auto result = tryParseFrame();
                if (!exists result && m_read_buf.size() > 0) {
                    # Incomplete frame after END_STREAM — unrecoverable
                    m_read_buf = binary();
                }
                m_reads_done = True;
                return result;
            }

            # Calculate remaining timeout
            int remaining_ms = (deadline - now_us()).durationMilliseconds();
            if (remaining_ms <= 0) {
                return NOTHING;  # Timeout
            }

            # Blocking read from stream
            *binary chunk = readStreamData(milliseconds(remaining_ms));
            if (chunk) {
                m_read_buf += chunk;
                # Validate buffer size to prevent unbounded accumulation
                if (m_max_message_size > 0 && m_read_buf.size() > 5 + m_max_message_size) {
                    throw "CONNECT-MESSAGE-SIZE-ERROR",
                        sprintf("read buffer size %d exceeds maximum message size %d bytes",
                            m_read_buf.size(), m_max_message_size);
                }
            }
        }
    }

    #! Write a response message to the client
    /** For server-streaming and bidirectional-streaming handlers.
        Serializes, frames, and sends a Connect message.

        In live mode, the stream is flushed after each write to ensure
        immediate delivery.

        @param message the message data to send
    */
    write(auto message) {
        ensureHeadersSent();
        binary encoded = m_codec.serialize(message);
        int flags = 0;
        if (m_send_encoding) {
            encoded = ConnectCompression::compress(encoded, m_send_encoding);
            flags = CONNECT_FLAG_COMPRESSED;
        }
        binary framed = ConnectHelper::frameMessage(encoded, flags);
        sendStreamData(framed, False);
        if (m_live) {
            flushStream();
        }
    }

    #! Check if all client messages have been consumed
    /** @return True if all messages have been read
    */
    bool readsDone() {
        if (!m_live) {
            return m_read_pos >= m_messages.size();
        }
        if (m_reads_done) {
            return True;
        }
        if (!isStreamComplete()) {
            return False;
        }
        # Stream complete; check if there's a parseable frame remaining
        if (m_read_buf.size() < 5) {
            return True;
        }
        int len = (m_read_buf[1] << 24) | (m_read_buf[2] << 16)
            | (m_read_buf[3] << 8) | m_read_buf[4];
        return m_read_buf.size() < 5 + len;
    }

    #! Set custom trailing metadata to send with the response
    /** @param metadata the trailing metadata key-value pairs
    */
    setTrailingMetadata(hash<auto> metadata) {
        map m_trailing_metadata{$1.key} = $1.value, metadata.pairIterator();
    }

    #! Get the custom trailing metadata set by the handler
    /** @return the custom trailing metadata
    */
    hash<auto> getTrailingMetadata() {
        return m_trailing_metadata;
    }

    #! Ensure response headers have been sent
    /** Called automatically by write(), but also called by the handler
        before sending the EndStreamResponse in case the handler wrote nothing.
    */
    ensureHeadersSent() {
        if (!m_headers_sent) {
            hash<auto> headers = {
                "content-type": ConnectStreamContentTypePrefix + m_codec.name(),
            };
            if (m_send_encoding) {
                headers{ConnectContentEncodingHeader} = m_send_encoding;
            }
            if (m_http3) {
                m_socket.submitQuicResponseStreaming(m_session_id, m_stream_id, 200, headers);
            } else {
                m_socket.submitHttp2StreamingResponseHeaders(m_stream_id, 200, headers);
            }
            m_headers_sent = True;
        }
    }

    #! Try to parse a complete Connect frame from the read buffer
    /** Handles both regular data frames and end-of-stream frames.

        @return the deserialized message from a data frame, or NOTHING if no
        complete frame is available.  Sets m_reads_done if an end-stream
        frame is found.
    */
    private *auto tryParseFrame() {
        if (m_read_buf.size() < 5) {
            return NOTHING;
        }
        int flags = m_read_buf[0];
        int len = (m_read_buf[1] << 24) | (m_read_buf[2] << 16)
            | (m_read_buf[3] << 8) | m_read_buf[4];
        if (m_max_message_size > 0 && len > m_max_message_size) {
            throw "CONNECT-MESSAGE-SIZE-ERROR",
                sprintf("Connect frame payload size %d exceeds maximum %d bytes",
                    len, m_max_message_size);
        }
        if (m_read_buf.size() < 5 + len) {
            return NOTHING;
        }
        # Extract the frame payload
        binary payload = m_read_buf.substr(5, len);
        # Remove consumed bytes from buffer
        if (m_read_buf.size() > 5 + len) {
            m_read_buf = m_read_buf.substr(5 + len);
        } else {
            m_read_buf = binary();
        }

        # Check for end-of-stream flag
        if (flags & CONNECT_FLAG_END_STREAM) {
            m_reads_done = True;
            return NOTHING;
        }

        # Decompress if needed
        if ((flags & CONNECT_FLAG_COMPRESSED) && m_recv_encoding) {
            payload = ConnectCompression::decompress(payload, m_recv_encoding);
            if (m_max_message_size > 0 && payload.size() > m_max_message_size) {
                throw "CONNECT-MESSAGE-SIZE-ERROR",
                    sprintf("decompressed payload size %d exceeds maximum %d bytes",
                        payload.size(), m_max_message_size);
            }
        }

        # Regular data frame — deserialize with codec
        return m_codec.deserialize(payload);
    }

    #! Check if the transport stream is complete
    private bool isStreamComplete() {
        if (m_http3) {
            return m_socket.isQuicStreamComplete(m_session_id, m_stream_id);
        }
        return m_socket.isHttp2StreamComplete(m_stream_id);
    }

    #! Read data from the transport stream
    private *binary readStreamData(timeout timeout_ms) {
        if (m_http3) {
            return m_socket.readQuicStreamDataBlock(m_session_id, m_stream_id, timeout_ms);
        }
        return m_socket.readHttp2StreamDataBlock(m_stream_id, timeout_ms);
    }

    #! Send data on the transport stream
    private sendStreamData(binary data, bool end_stream) {
        if (m_http3) {
            while (m_socket.submitQuicStreamData(m_session_id, m_stream_id, data,
                    end_stream) == 1) {
                m_socket.waitForQuicStreamDrain(m_session_id, m_stream_id, 5s);
            }
        } else {
            m_socket.sendHttp2StreamData(m_stream_id, data, end_stream);
        }
    }

    #! Flush the transport stream
    private flushStream() {
        if (!m_http3) {
            m_socket.flushHttp2();
        }
        # HTTP/3: I/O thread flushes automatically via writePackets()
    }
}

} # namespace Connect
