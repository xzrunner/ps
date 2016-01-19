#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_3d_buffer_h
#define particle_system_3d_buffer_h

#include <stdbool.h>
    
struct p3d_sprite;

void p3d_buffer_init(void (*update_srt_func)(void* params, float x, float y, float scale));

void p3d_buffer_insert(struct p3d_sprite*);
void p3d_buffer_remove(struct p3d_sprite*);
void p3d_buffer_clear();

bool p3d_buffer_update(float time);
void p3d_buffer_draw(float x, float y, float scale);

#endif // particle_system_3d_buffer_h

#ifdef __cplusplus
}
#endif