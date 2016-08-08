#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_3d_h
#define particle_system_3d_h

#include <stdbool.h>

#include "ps_utility.h"

struct p3d_emitter_cfg;

struct p3d_symbol {
	int count;
	int _dummy;		// unused: dummy for align to 64bit

	float scale_start, scale_end;

	float angle, angle_var;

	struct ps_color mul_col_begin, mul_col_end;
	struct ps_color add_col_begin, add_col_end;

	struct p3d_emitter_cfg* bind_ps_cfg;

	void* ud;
};

#define SIZEOF_P3D_SYMBOL (sizeof(struct p3d_symbol) + 2 * PTR_SIZE_DIFF)

struct p3d_particle_cfg {
	float lifetime;

	struct ps_vec2 dir;

	struct ps_vec3 spd_dir;

	float scale;

	float linear_acc;

	float tangential_spd;

	float angular_spd;

	float dis_spd;
	float dis_region;

	int _dummy;		// unused: dummy for align to 64bit

	struct p3d_symbol* sym;
};

#define SIZEOF_P3D_PARTICLE_CFG (sizeof(struct p3d_particle_cfg) + PTR_SIZE_DIFF)

struct p3d_particle {
	struct p3d_particle_cfg cfg;

	float mat[6];

	float life;

	int _dummy;		// unused: dummy for align to 64bit

	struct ps_vec3 pos;

	struct ps_vec3 spd;

	struct ps_vec2 dis_dir;
	float dis_curr_len;

	float angle;

	struct p3d_emitter* bind_ps;

	struct p3d_particle* next;
};

#define SIZEOF_P3D_PARTICLE (sizeof(struct p3d_particle) + PTR_SIZE_DIFF * 2 - sizeof(struct p3d_particle_cfg) + SIZEOF_P3D_PARTICLE_CFG)

enum GROUND_TYPE {
	P3D_NO_GROUND = 0,
	P3D_GROUND_WITH_BOUNCE,
	P3D_GROUND_WITHOUT_BOUNCE,
};

struct p3d_emitter_cfg {
	bool static_mode;
	char _pad[7];		// unused: dummy for align to 64bit

	float emission_time;
	int count;

	float life, life_var;

	float hori, hori_var;
	float vert, vert_var;

	float radial_spd, radial_spd_var;
	float tangential_spd, tangential_spd_var;
	float angular_spd, angular_spd_var;

	float dis_region, dis_region_var;
	float dis_spd, dis_spd_var;

	float gravity;

	float linear_acc, linear_acc_var;

	float fadeout_time;

	int ground;
	
	float start_radius;
	float start_height;

	bool orient_to_movement;
	char _pad3[3];		// unused: dummy for align to 64bit

	struct ps_vec3 dir;

	// todo: additive_blend, inertia

	int symbol_count;
	struct p3d_symbol* symbols;
};

#define SIZEOF_P3D_EMITTER_CFG (sizeof(struct p3d_emitter_cfg) + PTR_SIZE_DIFF)

struct p3d_emitter {
	struct p3d_particle *head, *tail;

	// not static mode
	float emit_counter;
	int particle_count;

	// static mode
	bool static_mode_finished;

	bool active;
	bool loop;
	char _pad[1];	// unused: dummy for align to 64bit

	float time;

	const struct p3d_emitter_cfg* cfg;

	void* ud;

	struct p3d_emitter* next;
};

void p3d_init();
void p3d_regist_cb(void (*render_func)(void* symbol, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, const void* ud),
				   void (*add_func)(struct p3d_particle*, void* ud),
				   void (*remove_func)(struct p3d_particle*, void* ud));
void p3d_clear();

struct p3d_emitter* p3d_emitter_create(const struct p3d_emitter_cfg* cfg);
void p3d_emitter_release(struct p3d_emitter*);
void p3d_emitter_clear(struct p3d_emitter*);

void p3d_emitter_stop(struct p3d_emitter*);
void p3d_emitter_start(struct p3d_emitter*);
void p3d_emitter_pause(struct p3d_emitter*);
void p3d_emitter_resume(struct p3d_emitter*);

void p3d_emitter_update(struct p3d_emitter*, float dt, float* mat);
void p3d_emitter_draw(struct p3d_emitter*, const void* ud);

bool p3d_emitter_is_finished(struct p3d_emitter*);

#endif // particle_system_3d_h

#ifdef __cplusplus
}
#endif