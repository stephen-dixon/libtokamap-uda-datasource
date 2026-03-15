#pragma once

#include <_stdlib.h>
#include <climits>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>

#include <libtokamap.hpp>

#include <clientserver/udaStructs.h>
#include <server/initPluginList.h>
#include <plugins/pluginStructs.h>

extern "C" void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& factory);

namespace uda_data_source
{

class UDADataSource : public libtokamap::DataSource
{
  public:
    explicit UDADataSource(std::string plugin_name,
                           std::optional<std::string> function,
                           std::optional<std::string> plugin_config_path)
        : m_plugin_name{std::move(plugin_name)}
        , m_function{std::move(function)}
    {
        if (plugin_config_path) {
            // This environment variable needs to be set before initPluginList for
            // the UDA plugins to be correctly found and loaded.
            setenv("UDA_PLUGIN_CONFIG", plugin_config_path.value().c_str(), 0);
        }
        initPluginList(&m_plugin_list, nullptr);
    }
    libtokamap::TypedDataArray get(const libtokamap::DataSourceArgs& data_source_args,
                                   const libtokamap::MapArguments& arguments,
                                   libtokamap::RamCache* ram_cache) override;

  private:
    std::string m_plugin_name;
    std::optional<std::string> m_function;
    PluginList m_plugin_list;

    [[nodiscard]] std::string get_request_str(const libtokamap::DataSourceArgs& data_source_args,
                                              const libtokamap::MapArguments& arguments) const;
    [[nodiscard]] int call_plugins(DATA_BLOCK* data_block, const libtokamap::DataSourceArgs& data_source_args,
                                   const libtokamap::MapArguments& arguments, libtokamap::RamCache* ram_cache) const;
};

} // namespace uda_data_source
