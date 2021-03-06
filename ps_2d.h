#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_2d_h
#define particle_system_2d_h

#include <stdbool.h>

#include "ps_utility.h"

#define P2D_MODE_GRAVITY	0
#define P2D_MODE_RADIUS		1
#define P2D_MODE_SPD_COS	2

struct p2d_symbol {
	float angle_start, angle_end;

	float scale_start, scale_end;

	struct ps_color mul_col_begin, mul_col_end;
	struct ps_color add_col_begin, add_col_end;

	void* ud;
};

#define SIZEOF_P2D_SYMBOL (sizeof(struct p2d_symbol) + PTR_SIZE_DIFF)

struct p2d_particle {
	struct p2d_symbol* sym;

	float mat[6];

	float life;

	int _dummy;		// unused: dummy for align to 64bit

	struct ps_vec2 position, position_ori;

	float angle, angle_delta;
	float scale, scale_delta;
	struct ps_color mul_col, mul_col_delta;
	struct ps_color add_col, add_col_delta;

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

	int sym_count;
	struct p2d_symbol* syms;
};

#define SIZEOF_P2D_EMITTER_CFG (sizeof(struct p2d_emitter_cfg) + PTR_SIZE_DIFF)

struct p2d_emitter {
	struct p2d_particle *head, *tail;

	float emit_counter;
	int particle_count;

	bool active;
	bool loop;
	bool local_mode_draw;
	char _pad[1];	// unused: dummy for align to 64bit

	float time;

	const struct p2d_emitter_cfg* cfg;

	struct p2d_emitter* next;
};

void p2d_init();
void p2d_regist_cb(void (*render_func)(void* sym, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, const void* ud));

struct p2d_emitter* p2d_emitter_create(const struct p2d_emitter_cfg* cfg);
void p2d_emitter_release(struct p2d_emitter* et);
void p2d_emitter_clear(struct p2d_emitter* et);

void p2d_emitter_stop(struct p2d_emitter*);
void p2d_emitter_start(struct p2d_emitter*);
void p2d_emitter_pause(struct p2d_emitter*);
void p2d_emitter_resume(struct p2d_emitter*);

void p2d_emitter_update(struct p2d_emitter* et, float dt, float* mat);
void p2d_emitter_draw(struct p2d_emitter* et, const void* ud);

bool p2d_emitter_is_finished(struct p2d_emitter*);

#endif // particle_system_2d_h

#ifdef __cplusplus
}
#endif