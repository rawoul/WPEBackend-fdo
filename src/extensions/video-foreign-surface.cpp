#include "../../include/wpe/extensions/video-foreign-surface.h"

#include "../ws-client.h"
#include "wpe-video-foreign-surface-client-protocol.h"
#include <wpe/wpe-egl.h>
#include <cstring>

namespace Impl {

class ForeignSurfaceBackendExtension : public WS::BaseBackendExtension {
public:
    ForeignSurfaceBackendExtension(WS::BaseBackend& backend)
    {
    }

    ~ForeignSurfaceBackendExtension()
    {
        if (m_videoForeignSurfaceManager)
            wpe_video_foreign_surface_manager_destroy(m_videoForeignSurfaceManager);
    }

    struct wpe_video_foreign_surface* exportSurface(uint32_t id)
    {
        if (m_videoForeignSurfaceManager)
            return wpe_video_foreign_surface_manager_export_foreign_surface(m_videoForeignSurfaceManager, id);
        return nullptr;
    }

    const char* name() const {
        return "wpe_video_foreign_surface";
    }

    void registryGlobal(struct wl_registry* registry, uint32_t name, const char *interface, uint32_t version)
    {
        if (!std::strcmp(interface, "wpe_video_foreign_surface_manager"))
            m_videoForeignSurfaceManager = static_cast<struct wpe_video_foreign_surface_manager*>(
                    wl_registry_bind(registry, name, &wpe_video_foreign_surface_manager_interface, 1));
    }

private:
    struct wpe_video_foreign_surface_manager* m_videoForeignSurfaceManager { nullptr };
};

}

void wpe_video_foreign_surface_extension_register(WS::BaseBackend& backend)
{
    backend.registerExtension(new Impl::ForeignSurfaceBackendExtension(backend));
}

extern "C" {

__attribute__((visibility("default")))
struct wpe_video_foreign_surface_source*
wpe_video_foreign_surface_source_create(struct wpe_renderer_backend_egl* backend, uint32_t foreign_surface_id)
{
    auto* egl_base = reinterpret_cast<struct wpe_renderer_backend_egl_base*>(backend);
    auto* base_backend = static_cast<WS::BaseBackend*>(egl_base->interface_data);
    auto* ext = reinterpret_cast<Impl::ForeignSurfaceBackendExtension*>(base_backend->getExtension("wpe_video_foreign_surface"));
    auto* surface = ext->exportSurface(foreign_surface_id);
    return reinterpret_cast<struct wpe_video_foreign_surface_source*>(surface);
}

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_source_destroy(struct wpe_video_foreign_surface_source* source)
{
    if (source) {
        auto* surface = reinterpret_cast<struct wpe_video_foreign_surface*>(source);
        wpe_video_foreign_surface_destroy(surface);
    }
}

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_source_set_position(struct wpe_video_foreign_surface_source* source, int32_t x, int32_t y)
{
    if (source) {
        auto* surface = reinterpret_cast<struct wpe_video_foreign_surface*>(source);
        wpe_video_foreign_surface_set_position(surface, x, y);
    }
}

}
