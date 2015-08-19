#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle3d_h
#define particle3d_h

#include <stdbool.h>

#include "utility.h"

struct ps_cfg_3d;

struct particle_symbol {
	float scale_start, scale_end;

	float angle, angle_var;

	struct ps_color4f col_mul, col_add;
	float alpha_start, alpha_end;

	struct ps_cfg_3d* bind_ps_cfg;

	void* ud;
};

struct particle_cfg {
	float lifetime;

	struct ps_vec2 dir;

	struct ps_vec3 spd_dir;

	float scale;

	float linear_acc;

	float angular_spd;

	float dis_spd;
	float dis_region;

	struct particle_symbol* symbol;
};

struct particle_3d {
	//	struct ps_vec2 pos;

	struct particle_cfg cfg;

	float life;

	struct ps_vec3 pos;

	struct ps_vec3 spd;

	struct ps_vec2 dis_dir;
	float dis_curr_len;

	float angle;

	// 创建时发射器的状态
	struct ps_vec2 init_pos;

	struct particle_system_3d* bind_ps;
};

struct ps_cfg_3d {
	float lifetime;

	float emission_time;
	int count;

	float life, life_var;

	float hori, hori_var;
	float vert, vert_var;

	float spd, spd_var;
	float angular_spd, angular_spd_var;

	float dis_region, dis_region_var;
	float dis_spd, dis_spd_var;

	float gravity;

	float linear_acc, linear_acc_var;

	float fadeout_time;

	bool bounce;

	float start_radius;
	bool is_start_radius_3d;

	bool orient_to_movement;

	struct ps_vec3 dir;

	// todo: additive_blend, inertia

	int symbol_count;
	struct particle_symbol* symbols;
};

struct particle_system_3d {
	struct particle_3d *start, *last, *end;

	float life;
	float emit_counter;

	bool active;
	bool loop;

	void (*add_func)(struct particle_3d*);
	void (*remove_func)(struct particle_3d*);

	struct ps_cfg_3d* cfg;
};

struct particle_system_3d* ps_create(int num, struct ps_cfg_3d* cfg);
void ps_init(struct particle_system_3d* ps, int num);
void ps_update(struct particle_system_3d* ps, float dt);

#endif // particle3d_h

#ifdef __cplusplus
}
#endif