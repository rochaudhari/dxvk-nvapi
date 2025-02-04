#include "nvapi_adapter_registry.h"
#include "../util/util_log.h"

namespace dxvk {

    NvapiAdapterRegistry::NvapiAdapterRegistry(ResourceFactory& resourceFactory)
        : m_resourceFactory(resourceFactory) {}

    NvapiAdapterRegistry::~NvapiAdapterRegistry() {
        for (const auto output : m_nvapiOutputs)
            delete output;

        for (const auto adapter : m_nvapiAdapters)
            delete adapter;

        m_nvapiOutputs.clear();
        m_nvapiAdapters.clear();
    }

    bool NvapiAdapterRegistry::Initialize() {
        auto dxgiFactory = m_resourceFactory.CreateDXGIFactory1();
        if (dxgiFactory == nullptr)
            return false;

        m_vulkan = m_resourceFactory.CreateVulkan();
        if (!m_vulkan->IsAvailable())
            return false;

        m_nvml = m_resourceFactory.CreateNvml();
        if (m_nvml->IsAvailable())
            log::write("NVML loaded and initialized successfully");

        // Query all D3D11 adapter from DXVK to honor any DXVK device filtering
        Com<IDXGIAdapter1> dxgiAdapter;
        for (auto i = 0U; dxgiFactory->EnumAdapters1(i, &dxgiAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
            auto nvapiAdapter = new NvapiAdapter(*m_vulkan, *m_nvml);
            if (nvapiAdapter->Initialize(dxgiAdapter, m_nvapiOutputs))
                m_nvapiAdapters.push_back(nvapiAdapter);
            else
                delete nvapiAdapter;
        }

        return !m_nvapiAdapters.empty();
    }

    uint16_t NvapiAdapterRegistry::GetAdapterCount() const {
        return m_nvapiAdapters.size();
    }

    NvapiAdapter* NvapiAdapterRegistry::GetAdapter() const {
        return m_nvapiAdapters.front();
    }

    NvapiAdapter* NvapiAdapterRegistry::GetAdapter(const uint16_t index) const {
        return index < m_nvapiAdapters.size() ? m_nvapiAdapters[index] : nullptr;
    }

    NvapiAdapter* NvapiAdapterRegistry::GetAdapter(const LUID& luid) const {
        auto it = std::find_if(m_nvapiAdapters.begin(), m_nvapiAdapters.end(),
            [luid](const auto& adapter) {
                auto adapterLuid = adapter->GetLuid();
                return adapterLuid.has_value()
                    && adapterLuid.value().HighPart == luid.HighPart
                    && adapterLuid.value().LowPart == luid.LowPart;
            });

        return it != m_nvapiAdapters.end() ? *it : nullptr;
    }

    bool NvapiAdapterRegistry::IsAdapter(NvapiAdapter* handle) const {
        return std::find(m_nvapiAdapters.begin(), m_nvapiAdapters.end(), handle) != m_nvapiAdapters.end();
    }

    NvapiOutput* NvapiAdapterRegistry::GetOutput(const uint16_t index) const {
        return index < m_nvapiOutputs.size() ? m_nvapiOutputs[index] : nullptr;
    }

    bool NvapiAdapterRegistry::IsOutput(NvapiOutput* handle) const {
        return std::find(m_nvapiOutputs.begin(), m_nvapiOutputs.end(), handle) != m_nvapiOutputs.end();
    }

    int16_t NvapiAdapterRegistry::GetPrimaryOutputId() const {
        auto it = std::find_if(m_nvapiOutputs.begin(), m_nvapiOutputs.end(),
            [](const auto& output) {
                return output->IsPrimary();
            });

        return static_cast<int16_t>(it != m_nvapiOutputs.end() ? std::distance(m_nvapiOutputs.begin(), it) : -1);
    }

    int16_t NvapiAdapterRegistry::GetOutputId(const std::string& displayName) const {
        auto it = std::find_if(m_nvapiOutputs.begin(), m_nvapiOutputs.end(),
            [&displayName](const auto& output) {
                return output->GetDeviceName() == displayName;
            });

        return static_cast<int16_t>(it != m_nvapiOutputs.end() ? std::distance(m_nvapiOutputs.begin(), it) : -1);
    }
}
