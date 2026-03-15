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

extern "C" void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& factory);

namespace uda_data_source
{

class UDADataSource : public libtokamap::DataSource
{
  public:
    explicit UDADataSource(std::string host, int port, std::string plugin_name, std::optional<std::string> function)
        : m_host{std::move(host)}, m_port{port}, m_plugin_name{std::move(plugin_name)}, m_function{std::move(function)}
    {
    }
    libtokamap::TypedDataArray get(const libtokamap::DataSourceArgs& data_source_args,
                                   const libtokamap::MapArguments& arguments, libtokamap::RamCache* ram_cache) override;

  private:
    std::string m_host;
    int m_port;
    std::string m_plugin_name;
    std::optional<std::string> m_function;

    [[nodiscard]] std::string get_request_str(const libtokamap::DataSourceArgs& data_source_args,
                                              const libtokamap::MapArguments& arguments) const;
    [[nodiscard]] DATA_BLOCK* call_uda(const libtokamap::DataSourceArgs& data_source_args,
                                       const libtokamap::MapArguments& arguments,
                                       libtokamap::RamCache* ram_cache) const;
};

} // namespace uda_data_source
