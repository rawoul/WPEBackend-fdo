#include "../../include/wpe/extensions/video-foreign-surface.h"

#include "../ws-client.h"
#include "wpe-video-foreign-surface-client-protocol.h"
#include <wpe/wpe-egl.h>
#include <cstring>

namespace Impl {

class ForeignSurfaceThread {
public:
    static ForeignSurfaceThread& singleton();
    static void initialize(struct wl_display*);

    struct wl_event_queue* eventQueue() const { return m_wl.eventQueue; }

private:
    static ForeignSurfaceThread* s_thread;
    static gpointer s_threadEntrypoint(gpointer);

    explicit ForeignSurfaceThread(struct wl_display*);

    struct ThreadSpawn {
        GMutex mutex;
        GCond cond;
        ForeignSurfaceThread* thread;
    };

    struct {
        struct wl_display* display;
        struct wl_event_queue* eventQueue;
    } m_wl;

    struct {
        GThread* thread;
        GSource* wlSource;
    } m_glib;
};

ForeignSurfaceThread* ForeignSurfaceThread::s_thread = nullptr;

ForeignSurfaceThread& ForeignSurfaceThread::singleton()
{
    return *s_thread;
}

void ForeignSurfaceThread::initialize(struct wl_display* display)
{
    if (s_thread) {
        if (s_thread->m_wl.display != display)
            g_error("ForeignSurfaceThread: tried to reinitialize with a different wl_display object");
    }

    if (!s_thread)
        s_thread = new ForeignSurfaceThread(display);
}

ForeignSurfaceThread::ForeignSurfaceThread(struct wl_display* display)
{
    m_wl.display = display;
    m_wl.eventQueue = wl_display_create_queue(m_wl.display);

    {
        ThreadSpawn threadSpawn;
        threadSpawn.thread = this;

        g_mutex_init(&threadSpawn.mutex);
        g_cond_init(&threadSpawn.cond);

        g_mutex_lock(&threadSpawn.mutex);

        m_glib.thread = g_thread_new("WPEBackend-fdo::video-foreign-surface-thread", s_threadEntrypoint, &threadSpawn);
        g_cond_wait(&threadSpawn.cond, &threadSpawn.mutex);

        g_mutex_unlock(&threadSpawn.mutex);

        g_mutex_clear(&threadSpawn.mutex);
        g_cond_clear(&threadSpawn.cond);
    }
}

gpointer ForeignSurfaceThread::s_threadEntrypoint(gpointer data)
{
    auto& threadSpawn = *reinterpret_cast<ThreadSpawn*>(data);
    g_mutex_lock(&threadSpawn.mutex);

    auto& thread = *threadSpawn.thread;

    GMainContext* context = g_main_context_new();
    GMainLoop* loop = g_main_loop_new(context, FALSE);

    g_main_context_push_thread_default(context);

    thread.m_glib.wlSource = WS::ws_polling_source_new("WPEBackend-fdo::video-foreign-surface", thread.m_wl.display, thread.m_wl.eventQueue);
    // The source is attached in the idle callback.

    {
        GSource* source = g_idle_source_new();
        g_source_set_callback(source,
            [](gpointer data) -> gboolean {
                auto& threadSpawn = *reinterpret_cast<ThreadSpawn*>(data);

                auto& thread = *threadSpawn.thread;
                g_source_attach(thread.m_glib.wlSource, g_main_context_get_thread_default());

                g_cond_signal(&threadSpawn.cond);
                g_mutex_unlock(&threadSpawn.mutex);
                return FALSE;
            }, &threadSpawn, nullptr);
        g_source_attach(source, context);
        g_source_unref(source);
    }

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
    return nullptr;
}

class ForeignSurface {
public:
    ForeignSurface(WS::BaseBackend& backend)
    {
        struct wl_display* display = backend.display();
        ForeignSurfaceThread::initialize(display);

        struct wl_event_queue* eventQueue = wl_display_create_queue(display);

        struct wl_registry* registry = wl_display_get_registry(display);
        wl_proxy_set_queue(reinterpret_cast<struct wl_proxy*>(registry), eventQueue);
        wl_registry_add_listener(registry, &s_registryListener, this);
        wl_display_roundtrip_queue(display, eventQueue);
        wl_registry_destroy(registry);

        wl_event_queue_destroy(eventQueue);
    }

    ~ForeignSurface()
    {
        if (m_wl.videoForeignSurface)
            wpe_video_foreign_surface_destroy(m_wl.videoForeignSurface);
    }

    void update(uint32_t id, uint32_t foreign_surface_id, int32_t x, int32_t y,
        wpe_video_foreign_surface_source_update_release_notify_t notify, void* notify_data)
    {
        if (!m_wl.videoForeignSurface) {
            notify(notify_data);
            return;
        }
        auto* update = wpe_video_foreign_surface_create_update(m_wl.videoForeignSurface, id, foreign_surface_id, x, y);

        wl_proxy_set_queue(reinterpret_cast<struct wl_proxy*>(update), ForeignSurfaceThread::singleton().eventQueue());
        wpe_video_foreign_surface_update_add_listener(update, &s_videoForeignSurfaceUpdateListener, new ListenerData { notify, notify_data });
    }

    void end_of_stream(uint32_t id)
    {
        if (m_wl.videoForeignSurface)
            wpe_video_foreign_surface_end_of_stream(m_wl.videoForeignSurface, id);
    }

private:
    static const struct wl_registry_listener s_registryListener;
    static const struct wpe_video_foreign_surface_update_listener s_videoForeignSurfaceUpdateListener;

    struct ListenerData {
        wpe_video_foreign_surface_source_update_release_notify_t notify;
        void* notify_data;
    };

    struct {
        struct wpe_video_foreign_surface* videoForeignSurface { nullptr };
    } m_wl;
};

const struct wl_registry_listener ForeignSurface::s_registryListener = {
    // global
    [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
    {
        auto& impl = *reinterpret_cast<ForeignSurface*>(data);
        if (!std::strcmp(interface, "wpe_video_foreign_surface"))
            impl.m_wl.videoForeignSurface = static_cast<struct wpe_video_foreign_surface*>(wl_registry_bind(registry, name, &wpe_video_foreign_surface_interface, 1));
    },
    // global_remove
    [](void*, struct wl_registry*, uint32_t) { },
};

const struct wpe_video_foreign_surface_update_listener ForeignSurface::s_videoForeignSurfaceUpdateListener = {
    // release
    [](void* data, struct wpe_video_foreign_surface_update* update)
    {
        auto* listenerData = static_cast<ListenerData*>(data);
        if (listenerData->notify)
            listenerData->notify(listenerData->notify_data);
        delete listenerData;

        wpe_video_foreign_surface_update_destroy(update);
    },
};

}

extern "C" {

__attribute__((visibility("default")))
struct wpe_video_foreign_surface_source*
wpe_video_foreign_surface_source_create(struct wpe_renderer_backend_egl* backend)
{
    auto* base = reinterpret_cast<struct wpe_renderer_backend_egl_base*>(backend);
    auto* impl = new Impl::ForeignSurface(*static_cast<WS::BaseBackend*>(base->interface_data));
    return reinterpret_cast<struct wpe_video_foreign_surface_source*>(impl);
}

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_source_destroy(struct wpe_video_foreign_surface_source* source)
{
    delete reinterpret_cast<Impl::ForeignSurface*>(source);
}

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_source_update(struct wpe_video_foreign_surface_source* foreign_surface_source, uint32_t foreign_surface_id, int32_t x, int32_t y,
    wpe_video_foreign_surface_source_update_release_notify_t notify, void* notify_data)
{
    auto& impl = *reinterpret_cast<Impl::ForeignSurface*>(foreign_surface_source);
    uint32_t id = reinterpret_cast<uintptr_t>(foreign_surface_source);
    impl.update(id, foreign_surface_id, x, y, notify, notify_data);
}

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_source_end_of_stream(struct wpe_video_foreign_surface_source* foreign_surface_source)
{
    auto& impl = *reinterpret_cast<Impl::ForeignSurface*>(foreign_surface_source);
    uint32_t id = reinterpret_cast<uintptr_t>(foreign_surface_source);
    impl.end_of_stream(id);
}

}
