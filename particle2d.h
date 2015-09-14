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

struct p2d_particle {
	struct p2d_symbol* symbol;

	float life;

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
		} C;
	} mode;
};

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

struct p2d_particle_system {
	struct p2d_particle *start, *last, *end;

	float emit_counter;

	bool is_active;
	bool is_loop;

	struct p2d_ps_config* cfg;
};

struct p2d_particle_system* p2d_create(int num, struct p2d_ps_config* cfg);
void p2d_init(struct p2d_particle_system* ps, int num);
void p2d_update(struct p2d_particle_system* ps, float dt);

#endif // particle2d_h

#ifdef __cplusplus
}
#endif