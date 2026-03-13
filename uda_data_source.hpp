#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>

#include <map_types/data_source_mapping.hpp>
#include <map_types/map_arguments.hpp>
#include <utils/ram_cache.hpp>

#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>

namespace json_plugin
{

class UDADataSource : public libtokamap::DataSource
{
  public:
    explicit UDADataSource(std::string plugin_name, std::optional<std::string> function, const PluginList* plugin_list,
                           bool cache_enabled)
        : m_plugin_name{std::move(plugin_name)}, m_function{std::move(function)}, m_plugin_list{plugin_list},
          m_cache_enabled{cache_enabled}
    {
    }
    libtokamap::TypedDataArray get(const libtokamap::DataSourceArgs& data_source_args,
                                   const libtokamap::MapArguments& arguments, libtokamap::RamCache* ram_cache) override;

  private:
    std::string m_plugin_name;
    std::optional<std::string> m_function;
    const PluginList* m_plugin_list;
    bool m_cache_enabled;

    [[nodiscard]] std::string get_request_str(const libtokamap::DataSourceArgs& data_source_args,
                                              const libtokamap::MapArguments& arguments) const;
    [[nodiscard]] int call_plugins(DATA_BLOCK* data_block, const libtokamap::DataSourceArgs& data_source_args,
                                   const libtokamap::MapArguments& arguments, libtokamap::RamCache* ram_cache) const;
};

} // namespace json_plugin
