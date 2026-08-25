#include "PluginManager.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include <QPluginLoader>
#include <QString>

#include "plugin/IVisionPlugin.h"

namespace visionlab {

namespace {

QString toQtPath(const std::filesystem::path& path)
{
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

bool isNativeLibrary(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext == ".dll" || ext == ".so" || ext == ".dylib";
}

std::string pathText(const std::filesystem::path& path)
{
    return toQtPath(path).toStdString();
}

} // namespace

struct PluginManager::Impl
{
    struct Loaded
    {
        std::unique_ptr<QPluginLoader> loader;
        IVisionPlugin* plugin = nullptr;
        PluginMetadata meta;
    };

    std::vector<Loaded> loaded;
    std::vector<std::string> errors;

    Loaded* find(const std::string& id)
    {
        for (auto& entry : loaded)
        {
            if (entry.meta.id == id)
                return &entry;
        }
        return nullptr;
    }

    bool hasId(const std::string& id) const
    {
        for (const auto& entry : loaded)
        {
            if (entry.meta.id == id)
                return true;
        }
        return false;
    }
};

PluginManager::PluginManager()
    : m_impl(std::make_unique<Impl>())
{
}

PluginManager::~PluginManager() = default;

void PluginManager::scan(const std::filesystem::path& directory)
{
    m_impl->loaded.clear();
    m_impl->errors.clear();

    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
    {
        if (ec)
            break;
        if (!entry.is_regular_file(ec))
            continue;

        const std::filesystem::path path = entry.path();
        if (!isNativeLibrary(path))
            continue;

        auto loader = std::make_unique<QPluginLoader>(toQtPath(path));
        if (!loader->load())
        {
            m_impl->errors.push_back(pathText(path) + ": " + loader->errorString().toStdString());
            continue;
        }

        IVisionPlugin* plugin = qobject_cast<IVisionPlugin*>(loader->instance());
        if (!plugin)
        {
            m_impl->errors.push_back(pathText(path)
                                     + ": not a vision plugin (iid/interface)");
            continue;
        }

        PluginMetadata meta = plugin->metadata();
        if (meta.interfaceVersion != 1)
        {
            m_impl->errors.push_back(pathText(path) + ": unsupported interfaceVersion");
            continue;
        }
        if (meta.id.empty())
        {
            m_impl->errors.push_back(pathText(path) + ": empty id");
            continue;
        }
        if (m_impl->hasId(meta.id))
        {
            m_impl->errors.push_back("duplicate id '" + meta.id + "': " + pathText(path));
            continue;
        }

        Impl::Loaded loaded;
        loaded.plugin = plugin;
        loaded.meta = std::move(meta);
        loaded.loader = std::move(loader);
        m_impl->loaded.push_back(std::move(loaded));
    }
}

std::vector<PluginMetadata> PluginManager::metadata() const
{
    std::vector<PluginMetadata> result;
    result.reserve(m_impl->loaded.size());
    for (const auto& entry : m_impl->loaded)
        result.push_back(entry.meta);
    return result;
}

std::vector<std::string> PluginManager::errors() const
{
    return m_impl->errors;
}

std::unique_ptr<IDetector> PluginManager::createDetector(const std::string& pluginId,
                                                         const DetectorCreateRequest& request)
{
    Impl::Loaded* entry = m_impl->find(pluginId);
    if (!entry)
    {
        m_impl->errors.push_back("unknown plugin id '" + pluginId + "'");
        return nullptr;
    }

    std::unique_ptr<IDetector> detector = entry->plugin->createDetector(request);
    if (!detector)
    {
        m_impl->errors.push_back("createDetector failed for id '" + pluginId + "'");
        return nullptr;
    }
    return detector;
}

} // namespace visionlab
