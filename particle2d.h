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

	struct p2d_particle* next;
};

#define SIZEOF_P2D_PARTICLE (sizeof(struct p2d_particle) + PTR_SIZE_DIFF)

struct p2d_emitter_cfg {
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

#define SIZEOF_P2D_PS_CONFIG (sizeof(struct p2d_emitter_cfg) + PTR_SIZE_DIFF)

struct p2d_emitter {
	struct p2d_particle *head, *tail;

	float emit_counter;

	bool active;
	bool loop;
	char _pad[2];	// unused: dummy for align to 64bit

	struct p2d_emitter_cfg* cfg;
};

#define SIZEOF_P2D_PARTICLE_SYSTEM (sizeof(struct p2d_emitter) + 4 * PTR_SIZE_DIFF)

void p2d_init();
void p2d_regist_cb(void (*render_func)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud));

struct p2d_emitter* p2d_create(struct p2d_emitter_cfg* cfg);
void p2d_emitter_release(struct p2d_emitter* et);
void p2d_emitter_clear(struct p2d_emitter* et);

void p2d_emitter_update(struct p2d_emitter* et, float dt);
void p2d_emitter_draw(struct p2d_emitter* et, const void* ud);

#endif // particle2d_h

#ifdef __cplusplus
}
#endif