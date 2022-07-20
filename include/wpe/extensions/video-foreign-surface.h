#ifndef __video_foreign_surface_h__
#define __video_foreign_surface_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wpe_renderer_backend_egl;
struct wpe_video_foreign_surface_source;

struct wpe_video_foreign_surface_source*
wpe_video_foreign_surface_source_create(struct wpe_renderer_backend_egl*);

void
wpe_video_foreign_surface_source_destroy(struct wpe_video_foreign_surface_source*);

typedef void (*wpe_video_foreign_surface_source_update_release_notify_t)(void *data);

void
wpe_video_foreign_surface_source_update(struct wpe_video_foreign_surface_source*, uint32_t foreign_surface_id, int32_t x, int32_t y,
    wpe_video_foreign_surface_source_update_release_notify_t notify, void* notify_data);

void
wpe_video_foreign_surface_source_end_of_stream(struct wpe_video_foreign_surface_source*);


struct wpe_video_foreign_surface_export;

struct wpe_video_foreign_surface_receiver {
    void (*handle_foreign_surface)(void* data, struct wpe_video_foreign_surface_export*, uint32_t id, uint32_t foreign_surface_id, int32_t x, int32_t y);
    void (*end_of_stream)(void* data, uint32_t id);
    void (*_wpe_reserved0)(void);
    void (*_wpe_reserved1)(void);
    void (*_wpe_reserved2)(void);
    void (*_wpe_reserved3)(void);
};

void
wpe_video_foreign_surface_register_receiver(const struct wpe_video_foreign_surface_receiver*, void* data);

void
wpe_video_foreign_surface_export_release(struct wpe_video_foreign_surface_export*);

#ifdef __cplusplus
}
#endif

#endif // __video_foreign_surface_h__
