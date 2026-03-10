# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file ConnectMethodDataProvider.qc Connect method-level data provider

/*  ConnectMethodDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

public namespace ConnectDataProvider {

#! Connect method-level data provider
/** Represents a single Connect RPC method.  Supports unary request/response
    via \c doRequest().

    The request and response types are generic (\c auto) since Connect does
    not have a schema definition language.

    @par Example
    @code{.py}
auto provider = new ConnectMethodDataProvider("Greeter", "SayHello",
    <ConnectDataProviderOptions>{"url": "https://localhost:8080"});
auto result = provider.doRequest({"name": "World"});
    @endcode

    @since ConnectDataProvider 1.0
*/
public class ConnectMethodDataProvider inherits DataProvider::AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "connect-method",
            "desc": "Connect RPC method data provider; supports unary request/response",
            "type": "ConnectMethodDataProvider",
            "supports_request": True,
            "supports_children": False,
        };
    }

    private {
        #! Service name
        string m_service;
        #! Method name
        string m_method;
        #! Provider options
        hash<ConnectDataProviderOptions> m_opts;
        #! Cached Connect client
        *Connect::ConnectClient m_client;
    }

    #! Creates the method provider
    /** @param service the service name
        @param method the method name
        @param opts provider options
    */
    constructor(string service, string method, hash<ConnectDataProviderOptions> opts) {
        m_service = service;
        m_method = method;
        m_opts = opts;
    }

    #! Returns the method name
    string getName() {
        return m_method;
    }

    #! Returns provider description
    *string getDesc() {
        return sprintf("Connect method '%s/%s' on %s", m_service, m_method, m_opts.url);
    }

    #! Returns the request type (generic auto)
    private *DataProvider::AbstractDataProviderType getRequestTypeImpl() {
        return AbstractDataProviderTypeMap."auto";
    }

    #! Returns the response type (generic auto)
    private *DataProvider::AbstractDataProviderType getResponseTypeImpl() {
        return AbstractDataProviderTypeMap."auto";
    }

    #! Makes a unary Connect RPC call
    /** @param req the request data
        @param request_options optional request options

        @return the response data

        @throw CONNECT-ERROR if the call fails
    */
    private auto doRequestImpl(auto req, *hash<auto> request_options) {
        Connect::ConnectClient client = getClient();
        hash<Connect::ConnectCallOptions> call_opts();
        if (m_opts.timeout_ms) {
            call_opts.timeout_ms = m_opts.timeout_ms;
        }
        if (m_opts.encoding) {
            call_opts.encoding = m_opts.encoding;
        }
        return client.call(m_service, m_method, req, call_opts);
    }

    #! Returns provider info
    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }

    #! Returns or creates the Connect client
    private Connect::ConnectClient getClient() {
        if (!m_client) {
            Connect::AbstractConnectCodec codec = createCodec();
            m_client = new Connect::ConnectClient(m_opts.url, codec,
                <Connect::ConnectClientOptions>{
                    "accept_all_certs": m_opts.accept_all_certs ?? True,
                    "protocol": m_opts.protocol,
                });
        }
        return m_client;
    }

    #! Creates the codec from options
    private Connect::AbstractConnectCodec createCodec() {
        switch (m_opts.codec) {
            case "json":
                return new Connect::JsonConnectCodec();
            case "cbor": {
                *Class cls = Class::forName("Connect::CborConnectCodec");
                if (!cls) {
                    throw "CONNECT-PROVIDER-ERROR", "CBOR codec requires the json module "
                        "version 1.12+; install it and restart";
                }
                return cls.create();
            }
            default:
                return new Connect::YamlConnectCodec();
        }
    }

    #! Destructor — release client resources
    destructor() {
        if (m_client) {
            m_client.shutdown();
        }
    }
}

} # namespace ConnectDataProvider
