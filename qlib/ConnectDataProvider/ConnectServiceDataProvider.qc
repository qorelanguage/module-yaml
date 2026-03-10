# -*- mode: qore; indent-tabs-mode: nil -*-
#! @file ConnectServiceDataProvider.qc Connect service-level data provider

/*  ConnectServiceDataProvider.qc Copyright 2026 Qore Technologies, s.r.o.

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

#! Connect service-level data provider
/** Represents a single Connect service.  Child providers are method names
    within this service.

    @since ConnectDataProvider 1.0
*/
public class ConnectServiceDataProvider inherits DataProvider::AbstractDataProvider {
    public {
        #! Provider info
        const ProviderInfo = <DataProviderInfo>{
            "name": "connect-service",
            "desc": "Connect RPC service data provider; children are method names",
            "type": "ConnectServiceDataProvider",
            "supports_children": True,
            "children_can_support_apis": True,
            "children_can_support_records": False,
            "children_can_support_observers": False,
        };
    }

    private {
        #! Service name
        string m_service;
        #! Provider options
        hash<ConnectDataProviderOptions> m_opts;
    }

    #! Creates the service provider
    /** @param service the service name
        @param opts provider options (inherited from root)
    */
    constructor(string service, hash<ConnectDataProviderOptions> opts) {
        m_service = service;
        m_opts = opts;
    }

    #! Returns the service name
    string getName() {
        return m_service;
    }

    #! Returns provider description
    *string getDesc() {
        return sprintf("Connect service '%s' on %s", m_service, m_opts.url);
    }

    #! Returns a child provider for the given method name
    private *DataProvider::AbstractDataProvider getChildProviderImpl(string name) {
        return new ConnectMethodDataProvider(m_service, name, m_opts);
    }

    #! Returns known child provider names
    /** Returns an empty list since Connect methods are dynamic.
    */
    private *list<string> getChildProviderNamesImpl() {
        return ();
    }

    #! Returns provider info
    private hash<DataProviderInfo> getStaticInfoImpl() {
        return ProviderInfo;
    }
}

} # namespace ConnectDataProvider
