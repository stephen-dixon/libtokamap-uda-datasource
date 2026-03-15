#include "uda_data_source.hpp"

#include <cstddef>
#include <cstring>
#include <exception>
#include <inja/inja.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <libtokamap.hpp>

// UDA includes
#include <client/accAPI.h>
#include <client/udaGetAPI.h>
#include <clientserver/compressDim.h>
#include <clientserver/udaTypes.h>

// TODO:
//  - handle compressed dims
//  - handle error arrays (how to determine not empty?)
//  - only read required data out of cache for each request (i.e. data, error, or a single dim)

namespace
{

std::unique_ptr<libtokamap::DataSource> uda_data_source_factory(const libtokamap::DataSourceFactoryArgs& args)
{
    auto host = libtokamap::get_arg<std::string>(args, "host");
    auto port = libtokamap::get_arg<int>(args, "port");
    auto plugin_name = libtokamap::get_arg<std::string>(args, "plugin_name");
    std::optional<std::string> function = {};
    if (args.contains("function")) {
        function = libtokamap::get_arg<std::string>(args, "function");
    }
    return std::make_unique<uda_data_source::UDADataSource>(host, port, plugin_name, function);
}

} // namespace

void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& factory) { factory.function = uda_data_source_factory; }

/**
 * @brief
 *
 * eg. UDA::get(signal=/AMC/ROGEXT/P1U, source=45460, host=uda2.hpc.l, port=56565)
 * eg. GEOM::get(signal=/magnetics/pfcoil/d1_upper, Config=1);
 * eg. JSONDataReader::get(signal=/APC/plasma_current);
 *
 * @param json_globals
 * @return
 */
std::string uda_data_source::UDADataSource::get_request_str(const libtokamap::DataSourceArgs& data_source_args,
                                                            const libtokamap::MapArguments& arguments) const
{
    std::stringstream string_stream;
    // string_stream << m_plugin_name << "::" << m_function.value_or("get") << "(";
    string_stream << m_plugin_name << "::";

    if (data_source_args.count("function") != 0) {
        string_stream << data_source_args.at("function").get<std::string>();
    } else {
        string_stream << m_function.value_or("get");
    }
    string_stream << "(";

    if (data_source_args.at("signal") == "void" || data_source_args.at("signal").empty()) { // temporary ugliness
        return {};
    }

    // m_map_args 'field' currently nlohmann json
    // parse to string/bool
    // TODO: change, however std::any/std::variant functionality for free
    const char* delim = "";
    for (const auto& [key, field] : data_source_args) {
        if (field.is_string()) {
            // Double inja
            try {
                auto value =
                    inja::render(inja::render(field.get<std::string>(), arguments.global_data), arguments.global_data);
                string_stream << delim << key << "=" << value;
            } catch (std::exception& e) {
                // UDA_LOG(UDA_LOG_DEBUG, "Inja template error in request : %s\n", e.what());
                return {};
            }
        } else if (field.is_boolean()) {
            string_stream << delim << key;
        } else {
            continue;
        }
        delim = ", ";
    }
    string_stream << ")";

    auto request = string_stream.str();
    // UDA_LOG(UDA_LOG_DEBUG, "Plugin Mapping Request : %s\n", request.c_str());
    return request;
}

DATA_BLOCK* uda_data_source::UDADataSource::call_uda(const libtokamap::DataSourceArgs& data_source_args,
                                                     const libtokamap::MapArguments& arguments,
                                                     libtokamap::RamCache* ram_cache) const
{
    auto request_str = get_request_str(data_source_args, arguments);
    if (request_str.empty()) {
        return nullptr;
    } // Return 1 if no request receieved

    REQUEST_DATA request = {0};
    strcpy(request.signal, request_str.c_str());

    int handle = idamGetAPIWithHost(request_str.c_str(), "", m_host.c_str(), m_port);
    if (handle < 0) {
        return nullptr;
    }

    return getIdamDataBlock(handle);
}

namespace
{
class ArrayBuilder
{
  private:
    const char* m_data = nullptr;
    size_t m_size = {};
    std::vector<size_t> m_shape;
    int m_data_type = UDA_TYPE_UNKNOWN;
    bool m_owning = true;
    bool m_ownership_locked = false;
    bool m_buildable = false;
    bool m_free_data_required = false;

  public:
    ArrayBuilder() = default;
    ~ArrayBuilder()
    {
        if (m_free_data_required and m_data != nullptr) {
            free(const_cast<char*>(m_data));
        }
    }

    ArrayBuilder(const ArrayBuilder&) = delete;
    ArrayBuilder& operator=(const ArrayBuilder&) = delete;
    ArrayBuilder(ArrayBuilder&& other) = delete;
    ArrayBuilder& operator=(ArrayBuilder&& other) = delete;

    enum class OwnershipPolicy { VIEW, COPY };

    void set_ownership(OwnershipPolicy policy)
    {
        bool is_owning = (policy == OwnershipPolicy::COPY);
        if (m_ownership_locked and m_owning != is_owning) {
            throw std::runtime_error("Ownership policy already enforced by a previous option");
        }
        m_owning = is_owning;
    }
    ArrayBuilder& ownership(OwnershipPolicy policy)
    {
        set_ownership(policy);
        return *this;
    }

    void set_data(const DATA_BLOCK& db)
    {
        m_data = db.data;
        m_size = db.data_n;
        m_shape.reserve(db.rank);
        for (unsigned int i = db.rank - 1; i >= 0; --i) {
            m_shape.push_back(db.dims[i].dim_n);
        }
        m_data_type = db.data_type;
        m_buildable = true;
    }
    ArrayBuilder& data(const DATA_BLOCK& db)
    {
        set_data(db);
        return *this;
    }

    void set_dimension_data(const DIMS& dim)
    {
        if (dim.compressed > 0) {
            DIMS tmp_dim = dim;

            uncompressDim(&tmp_dim);
            tmp_dim.compressed = 0;
            tmp_dim.method = 0;

            m_data = tmp_dim.dim;
            tmp_dim.dim = nullptr;
            m_free_data_required = true;

            m_owning = true;
            m_ownership_locked = true; // cannot guarantee sufficient object lifetime?
        } else {
            m_data = dim.dim;
        }
        m_size = dim.dim_n;
        m_shape = {m_size};
        m_data_type = dim.data_type;
        m_buildable = true;
    }
    ArrayBuilder& dimension(const DIMS& dim)
    {
        set_dimension_data(dim);
        return *this;
    }

    void set_time_data(const DATA_BLOCK& db)
    {
        auto index = db.order;
        if (index < 0 or index > db.rank or db.rank < 1) {
            throw std::runtime_error("No time data available for this signal");
        }
        set_dimension_data(db.dims[index]);
    }
    ArrayBuilder& time(const DATA_BLOCK& db)
    {
        set_time_data(db);
        return *this;
    }

  private:
    template <typename T> libtokamap::TypedDataArray _array_factory()
    {
        if constexpr (std::is_same_v<T, char>) {
            return libtokamap::TypedDataArray(const_cast<char*>(m_data), m_size, std::move(m_shape), m_owning);
        } else {
            return libtokamap::TypedDataArray(reinterpret_cast<T*>(const_cast<char*>(m_data)), m_size,
                                              std::move(m_shape), m_owning);
        }
    }

  public:
    libtokamap::TypedDataArray build()
    {
        switch (m_data_type) {
            case UDA_TYPE_SHORT:
                return _array_factory<short>();
            case UDA_TYPE_INT:
                return _array_factory<int>();
            case UDA_TYPE_UNSIGNED_INT:
                return _array_factory<short>();
            case UDA_TYPE_LONG:
                return _array_factory<long>();
            case UDA_TYPE_LONG64:
                return _array_factory<int64_t>();
            case UDA_TYPE_FLOAT:
                return _array_factory<float>();
            case UDA_TYPE_DOUBLE:
                return _array_factory<double>();
            case UDA_TYPE_UNSIGNED_CHAR:
                return _array_factory<unsigned char>();
            case UDA_TYPE_UNSIGNED_SHORT:
                return _array_factory<unsigned short>();
            case UDA_TYPE_UNSIGNED_LONG:
                return _array_factory<unsigned long>();
            case UDA_TYPE_UNSIGNED_LONG64:
                return _array_factory<uint64_t>();
            case UDA_TYPE_CHAR:
            case UDA_TYPE_STRING:
                return _array_factory<char>();
            case UDA_TYPE_COMPLEX:
                return _array_factory<COMPLEX>();
            case UDA_TYPE_DCOMPLEX:
                return _array_factory<DCOMPLEX>();
            default:
                throw std::runtime_error{"unknown data type"};
        }
    }
};
} // namespace

libtokamap::TypedDataArray uda_data_source::UDADataSource::get(const libtokamap::DataSourceArgs& data_source_args,
                                                               const libtokamap::MapArguments& arguments,
                                                               libtokamap::RamCache* ram_cache)
{
    DATA_BLOCK* data_block = call_uda(data_source_args, arguments, ram_cache);

    if (data_block == nullptr) {
        return {};
    }

    if (data_source_args.count("time") != 0 && data_source_args.at("time").get<bool>()) {
        return ArrayBuilder().ownership(ArrayBuilder::OwnershipPolicy::COPY).time(*data_block).build();
    }
    return ArrayBuilder().ownership(ArrayBuilder::OwnershipPolicy::COPY).data(*data_block).build();
}
