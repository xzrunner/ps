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

	void* dummy;	// todo: adapt old data

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

	void* ud;

	struct p3d_particle* next;
};

#define SIZEOF_P3D_PARTICLE (sizeof(struct p3d_particle) + PTR_SIZE_DIFF * 2 - sizeof(struct p3d_particle_cfg) + SIZEOF_P3D_PARTICLE_CFG)

enum GROUND_TYPE {
	P3D_NO_GROUND = 0,
	P3D_GROUND_WITH_BOUNCE,
	P3D_GROUND_WITHOUT_BOUNCE,
};

struct p3d_emitter_cfg {
	int blend;

	bool static_mode;
	char _pad[3];		// unused: dummy for align to 64bit

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

	int sym_count;
	struct p3d_symbol* syms;
};

#define SIZEOF_P3D_EMITTER_CFG (sizeof(struct p3d_emitter_cfg) + PTR_SIZE_DIFF)

struct p3d_emitter {
	struct p3d_particle *head, *tail;

	// not static mode
	float emit_counter;
	int particle_count;
	int index;
	int expire;

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
void p3d_regist_cb(void (*blend_begin_func)(int blend),
				   void (*blend_end_func)(),
				   void (*render_func)(void* spr, void* sym, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, int fast_blend, const void* ud, float time),
				   void (*update_func)(void* spr, float x, float y),
				   void (*add_func)(struct p3d_particle*, void* ud),
				   void (*remove_func)(struct p3d_particle*, void* ud));
void p3d_clear();
void p3d_tick();
void p3d_gc();
void p3d_set_disabled(int disabled);

// if no error occurs, it returns emitter id; otherwise it returns 0
int p3d_emitter_create(const struct p3d_emitter_cfg* cfg);
void p3d_emitter_release(int emitter_id);
void p3d_emitter_clear(int emitter_id);

void p3d_emitter_stop(int emitter_id);
void p3d_emitter_start(int emitter_id);
void p3d_emitter_pause(int emitter_id);
void p3d_emitter_resume(int emitter_id);

bool p3d_emitter_check(int emitter_id);
void p3d_emitter_update(int emitter_id, float dt, float* mat);
void p3d_emitter_draw(int emitter_id, const void* ud);

bool p3d_emitter_is_loop(int emitter_id);
void p3d_emitter_set_loop(int emitter_id, bool loop);

bool p3d_emitter_get_time(int emitter_id, float *time);
void p3d_emitter_set_time(int emitter_id, float time);

bool p3d_emitter_is_finished(int emitter_id);

// for debug
int  p3d_emitter_count();

#endif // particle_system_3d_h

#ifdef __cplusplus
}
#endif