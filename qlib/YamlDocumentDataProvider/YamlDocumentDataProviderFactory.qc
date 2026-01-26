# -*- mode: qore; indent-tabs-mode: nil -*-

%new-style
%strict-args
%require-types
%enable-all-warnings

public namespace YamlDocumentDataProvider {

#! Factory for creating YAML document data providers
public class YamlDocumentDataProviderFactory inherits DataProvider::AbstractDataProviderFactory {
    public {
        #! Factory metadata
        const FactoryInfo = <DataProviderFactoryInfo>{
            "name": "yamldoc",
            "desc": "Data provider factory for YAML document streams",
            "children_can_support_observers": False,
        };
    }

    #! Returns factory info
    hash<DataProviderFactoryInfo> getInfoImpl() {
        return FactoryInfo;
    }

    #! Returns the provider class
    Class getClassImpl() {
        return Class::forName("YamlDocumentDataProvider::YamlDocumentReadDataProvider");
    }

    #! Creates a provider instance
    private DataProvider::AbstractDataProvider createImpl(hash<auto> options,
            *LoggerInterface logger) {
        return new YamlDocumentReadDataProvider(options);
    }

    #! Returns info about the provider
    *hash<auto> getInfoAsDataImpl(hash<auto> constructor_options, *bool with_type_info) {
        return YamlDocumentReadDataProvider::ProviderInfo;
    }

    #! Returns the description
    string getDescImpl() {
        return FactoryInfo.desc;
    }

    #! Returns provider info
    private hash<DataProvider::DataProviderInfo> getProviderInfoImpl() {
        return YamlDocumentReadDataProvider::ProviderInfo;
    }
}

} # namespace YamlDocumentDataProvider
