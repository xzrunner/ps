#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_3d_sprite_h
#define particle_system_3d_sprite_h

#include <stdbool.h>

struct p3d_sprite {
	struct p3d_emitter* et;
	bool local_mode_draw;
	float mat[6];

	void* draw_params;

	struct p3d_sprite** ptr_self;

	struct p3d_sprite* next;

	int ref_id;
};

void p3d_sprite_init(void (*create_draw_params_func)(struct p3d_sprite*),
					 void (*release_draw_params_func)(struct p3d_sprite*));

struct p3d_sprite* p3d_sprite_create();
void p3d_sprite_release(struct p3d_sprite*);

void p3d_sprite_create_draw_params(struct p3d_sprite*);

#endif // particle_system_3d_sprite_h

#ifdef __cplusplus
}
#endif