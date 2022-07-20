#include "../../include/wpe/extensions/video-foreign-surface.h"

#include "../ws.h"
#include <unistd.h>

static struct {
    const struct wpe_video_foreign_surface_receiver* receiver;
    void* data;
} s_registered_receiver;

extern "C" {

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_register_receiver(const struct wpe_video_foreign_surface_receiver* receiver, void* data)
{
    s_registered_receiver.receiver = receiver;
    s_registered_receiver.data = data;

    WS::Instance::singleton().initializeVideoForeignSurface([](struct wpe_video_foreign_surface_export* foreign_surface_export, uint32_t id, uint32_t foreign_surface_id, int32_t x, int32_t y) {
        if (s_registered_receiver.receiver) {
            s_registered_receiver.receiver->handle_foreign_surface(s_registered_receiver.data, foreign_surface_export, id, foreign_surface_id, x, y);
            return;
        }
    }, [](uint32_t id) {
        if (s_registered_receiver.receiver)
            s_registered_receiver.receiver->end_of_stream(s_registered_receiver.data, id);
    });
}

__attribute__((visibility("default")))
void
wpe_video_foreign_surface_export_release(struct wpe_video_foreign_surface_export* foreign_surface_export)
{
    WS::Instance::singleton().releaseVideoForeignSurfaceExport(foreign_surface_export);
}

}
