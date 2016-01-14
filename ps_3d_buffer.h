#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_3d_buffer_h
#define particle_system_3d_buffer_h

#include <stdbool.h>

struct p3d_sprite {
	struct p3d_emitter* et;
	bool local_mode_draw;
	float mat[6];
	struct p3d_sprite** ud;
};

void p3d_buffer_init(void* (*create_render_params_func)(),
					  void (*wrap_render_params_func)(void* params, float* mat));

struct p3d_sprite* p3d_buffer_add();
void p3d_buffer_remove(struct p3d_sprite*);
void p3d_buffer_clear();

bool p3d_buffer_update(float time);
void p3d_buffer_draw();

#endif // particle_system_3d_buffer_h

#ifdef __cplusplus
}
#endif