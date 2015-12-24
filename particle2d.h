#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle2d_h
#define particle2d_h

#include <stdbool.h>

#include "utility.h"

#define P2D_MODE_GRAVITY	0
#define P2D_MODE_RADIUS		1
#define P2D_MODE_SPD_COS	2

struct p2d_symbol {
	float angle_start, angle_end;

	float scale_start, scale_end;

	struct ps_color4f col_mul_start, col_mul_end;
	struct ps_color4f col_add_start, col_add_end;

	void* ud;
};

#define SIZEOF_P2D_SYMBOL (sizeof(struct p2d_symbol) + PTR_SIZE_DIFF)

struct p2d_particle {
	struct p2d_symbol* symbol;

	float life;

	int _dummy;		// unused: dummy for align to 64bit

	struct ps_vec2 position, position_ori;

	float angle, angle_delta;
	float scale, scale_delta;
	struct ps_color4f col_mul, col_mul_delta;
	struct ps_color4f col_add, col_add_delta;

	union {
		struct {
			struct ps_vec2 speed;
			float tangential_accel;
			float radial_accel;
		} A;

		struct {
			float direction, direction_delta;
			float radius, radius_delta;
		} B;

		struct {
			float lifetime;
			struct ps_vec2 speed;
			float cos_amplitude, cos_frequency;
			int _dummy;		// unused: dummy for align to 64bit
		} C;
	} mode;
};

#define SIZEOF_P2D_PARTICLE (sizeof(struct p2d_particle) + PTR_SIZE_DIFF)

struct p2d_ps_config {
	int mode_type;

	union {
		// gravity + tangential + radial
		struct {
			struct ps_vec2 gravity;

			float speed, speed_var;

			float tangential_accel, tangential_accel_var;
			float radial_accel, radial_accel_var;

			bool rotation_is_dir;
			char _pad3[7];		// unused: dummy for align to 64bit
		} A;

		// radius + rotate
		struct {
			float start_radius, start_radius_var;
			float end_radius, end_radius_var;

			float direction_delta, direction_delta_var;
		} B;

		// tangential spd cos
		struct {
			float speed, speed_var;

			float cos_amplitude, cos_amplitude_var;
			float cos_frequency, cos_frequency_var;
		} C;

	} mode;

	float emission_time;
	int count;

	float life, life_var;

	struct ps_vec2 position, position_var;

	float direction, direction_var;

	int symbol_count;
	struct p2d_symbol* symbols;
};

#define SIZEOF_P2D_PS_CONFIG (sizeof(struct p2d_ps_config) + PTR_SIZE_DIFF)

struct p2d_particle_system {
	struct p2d_particle *start, *last, *end;

	float emit_counter;

	bool is_active;
	bool is_loop;
	char _pad[2];	// unused: dummy for align to 64bit

	void (*render_func)(void* symbol, float x, float y, float angle, float scale, 
		struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud);

	struct p2d_ps_config* cfg;
};

#define SIZEOF_P2D_PARTICLE_SYSTEM (sizeof(struct p2d_particle_system) + 5 * PTR_SIZE_DIFF)

struct p2d_particle_system* p2d_create(int num, struct p2d_ps_config* cfg);
void p2d_release(struct p2d_particle_system* ps);

struct p2d_particle_system* p2d_create_with_mem(void* mem, int num, struct p2d_ps_config* cfg);

void p2d_update(struct p2d_particle_system* ps, float dt);
void p2d_draw(struct p2d_particle_system* ps, const void* ud);

#endif // particle2d_h

#ifdef __cplusplus
}
#endif