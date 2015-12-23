#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle3d_h
#define particle3d_h

#include <stdbool.h>

#include "utility.h"

struct p3d_ps_config;

struct p3d_symbol {
	float scale_start, scale_end;

	float angle, angle_var;

	struct ps_color4f col_mul, col_add;
	float alpha_start, alpha_end;

	struct p3d_ps_config* bind_ps_cfg;

	void* ud;
};

#define SIZEOF_P3D_SYMBOL (sizeof(struct p3d_symbol) + 2 * PTR_SIZE_DIFF)

struct p3d_particle_cfg {
	float lifetime;

	struct ps_vec2 dir;

	struct ps_vec3 spd_dir;

	float scale;

	float linear_acc;

	float angular_spd;

	float dis_spd;
	float dis_region;

	int _dummy;		// unused: dummy for align to 64bit

	struct p3d_symbol* symbol;
};

#define SIZEOF_P3D_PARTICLE_CFG (sizeof(struct p3d_particle_cfg) + PTR_SIZE_DIFF)

struct p3d_particle {
	//	struct ps_vec2 pos;

	struct p3d_particle_cfg cfg;

	float life;

	int _dummy;		// unused: dummy for align to 64bit

	struct ps_vec3 pos;

	struct ps_vec3 spd;

	struct ps_vec2 dis_dir;
	float dis_curr_len;

	float angle;

	// 创建时发射器的状态
	struct ps_vec2 init_pos;

	struct p3d_particle_system* bind_ps;
};

#define SIZEOF_P3D_PARTICLE (sizeof(struct p3d_particle) + PTR_SIZE_DIFF - sizeof(struct p3d_particle_cfg) + SIZEOF_P3D_PARTICLE_CFG)

enum GROUND_TYPE {
	P3D_NO_GROUND = 0,
	P3D_GROUND_WITH_BOUNCE,
	P3D_GROUND_WITHOUT_BOUNCE,
};

struct p3d_ps_config {
//	float lifetime;

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

	int ground;
	
	float start_radius;
	bool is_start_radius_3d;
	char _pad2[3];		// unused: dummy for align to 64bit

	bool orient_to_movement;
	char _pad3[3];		// unused: dummy for align to 64bit

	struct ps_vec3 dir;

	// todo: additive_blend, inertia

	int symbol_count;
	struct p3d_symbol* symbols;
};

#define SIZEOF_P3D_PS_CONFIG (sizeof(struct p3d_ps_config) + PTR_SIZE_DIFF)

struct p3d_particle_system {
	struct p3d_particle *start, *last, *end;

	float emit_counter;
	int particle_count;

	bool active;
	bool loop;
	char _pad[6];	// unused: dummy for align to 64bit

	void (*add_func)(struct p3d_particle*, void* ud);
	void (*remove_func)(struct p3d_particle*, void* ud);
	void* ud;

	struct p3d_ps_config* cfg;
};

#define SIZEOF_P3D_PARTICLE_SYSTEM (sizeof(struct p3d_particle_system) + 7 * PTR_SIZE_DIFF)

struct p3d_particle_system* p3d_create(int num, struct p3d_ps_config* cfg);
void p3d_release(struct p3d_particle_system* ps);

struct p3d_particle_system* p3d_create_with_mem(void* mem, int num, struct p3d_ps_config* cfg);

void p3d_update(struct p3d_particle_system* ps, float dt);

#endif // particle3d_h

#ifdef __cplusplus
}
#endif