#ifndef __video_foreign_surface_h__
#define __video_foreign_surface_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wpe_renderer_backend_egl;
struct wpe_video_foreign_surface_source;

struct wpe_video_foreign_surface_source*
wpe_video_foreign_surface_source_create(struct wpe_renderer_backend_egl*, uint32_t foreign_surface_id);

void
wpe_video_foreign_surface_source_destroy(struct wpe_video_foreign_surface_source*);

void
wpe_video_foreign_surface_source_set_position(struct wpe_video_foreign_surface_source*, int32_t x, int32_t y);

struct wpe_video_foreign_surface_export;
void wpe_video_foreign_surface_export_set_data(struct wpe_video_foreign_surface_export* foreign_surface_export, void *data);
void *wpe_video_foreign_surface_export_get_data(struct wpe_video_foreign_surface_export* foreign_surface_export);

struct wpe_video_foreign_surface_receiver {
    void (*handle_foreign_surface)(void* data, struct wpe_video_foreign_surface_export*, uint32_t foreign_surface_id);
    void (*set_position)(void* data, struct wpe_video_foreign_surface_export*, int32_t x, int32_t y);
    void (*_wpe_reserved0)(void);
    void (*_wpe_reserved1)(void);
    void (*_wpe_reserved2)(void);
    void (*_wpe_reserved3)(void);
};

void
wpe_video_foreign_surface_register_receiver(const struct wpe_video_foreign_surface_receiver*, void* data);

#ifdef __cplusplus
}
#endif

#endif // __video_foreign_surface_h__
