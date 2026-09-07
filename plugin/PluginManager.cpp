#include "PluginManager.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include <QLibrary>
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
    struct Resident
    {
        std::unique_ptr<QPluginLoader> loader;
        IVisionPlugin* plugin = nullptr;
        PluginMetadata meta;
        std::filesystem::path path;
    };

    // Residents are never erased: scan() must not unload while IDetector
    // instances from a previous catalog still exist (CameraManager backup).
    std::vector<std::unique_ptr<Resident>> residents;
    std::vector<Resident*> catalog;
    std::vector<std::string> errors;

    Resident* findResident(const std::filesystem::path& path)
    {
        std::error_code ec;
        for (auto& resident : residents)
        {
            if (std::filesystem::equivalent(resident->path, path, ec) && !ec)
                return resident.get();
            ec.clear();
        }
        return nullptr;
    }

    bool catalogHasId(const std::string& id) const
    {
        for (const Resident* resident : catalog)
        {
            if (resident->meta.id == id)
                return true;
        }
        return false;
    }

    Resident* catalogFind(const std::string& id)
    {
        for (Resident* resident : catalog)
        {
            if (resident->meta.id == id)
                return resident;
        }
        return nullptr;
    }
};

PluginManager::PluginManager()
    : m_impl(std::make_unique<Impl>())
{
}

PluginManager::~PluginManager() = default;

void PluginManager::scan(const std::filesystem::path& directory)
{
    m_impl->catalog.clear();
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

        if (Impl::Resident* existing = m_impl->findResident(path))
        {
            if (m_impl->catalogHasId(existing->meta.id))
            {
                m_impl->errors.push_back("duplicate id '" + existing->meta.id + "': " + pathText(path));
                continue;
            }
            m_impl->catalog.push_back(existing);
            continue;
        }

        auto loader = std::make_unique<QPluginLoader>(toQtPath(path));
        loader->setLoadHints(QLibrary::PreventUnloadHint);
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
        if (m_impl->catalogHasId(meta.id))
        {
            m_impl->errors.push_back("duplicate id '" + meta.id + "': " + pathText(path));
            continue;
        }

        auto resident = std::make_unique<Impl::Resident>();
        resident->plugin = plugin;
        resident->meta = std::move(meta);
        resident->path = path;
        resident->loader = std::move(loader);
        m_impl->catalog.push_back(resident.get());
        m_impl->residents.push_back(std::move(resident));
    }
}

std::vector<PluginMetadata> PluginManager::metadata() const
{
    std::vector<PluginMetadata> result;
    result.reserve(m_impl->catalog.size());
    for (const Impl::Resident* resident : m_impl->catalog)
        result.push_back(resident->meta);
    return result;
}

std::vector<std::string> PluginManager::errors() const
{
    return m_impl->errors;
}

std::unique_ptr<IDetector> PluginManager::createDetector(const std::string& pluginId,
                                                         const DetectorCreateRequest& request)
{
    Impl::Resident* entry = m_impl->catalogFind(pluginId);
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
