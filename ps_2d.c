#include "ps_2d.h"
#include "ps_array.h"

#include <logger.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_PARTICLE_SZ 10000
#define MAX_EMITTER_SZ	1000

static struct p2d_particle* PARTICLE_ARRAY = NULL;
static struct p2d_emitter*	EMITTER_ARRAY = NULL;

static void (*RENDER_FUNC)(void* sym, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, const void* ud);

void 
p2d_init() {
	int sz = sizeof(struct p2d_particle) * MAX_PARTICLE_SZ;
	PARTICLE_ARRAY = (struct p2d_particle*)malloc(sz);
	if (!PARTICLE_ARRAY) {
		LOGW("%s", "malloc err: p2d_init particle");
		return;
	}
	memset(PARTICLE_ARRAY, 0, sz);
	PS_ARRAY_INIT(PARTICLE_ARRAY, MAX_PARTICLE_SZ);

	sz = sizeof(struct p2d_emitter) * MAX_EMITTER_SZ;
	EMITTER_ARRAY = (struct p2d_emitter*)malloc(sz);
	if (!EMITTER_ARRAY) {
		LOGW("%s", "malloc err: p2d_init emitter");
		return;
	}
	memset(EMITTER_ARRAY, 0, sz);
	PS_ARRAY_INIT(EMITTER_ARRAY, MAX_EMITTER_SZ);
}

void 
p2d_regist_cb(void (*render_func)(void* sym, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, const void* ud)) {
	RENDER_FUNC = render_func;	
}

struct p2d_emitter* 
p2d_emitter_create(struct p2d_emitter_cfg* cfg) {
	struct p2d_emitter* et;
	PS_ARRAY_ALLOC(EMITTER_ARRAY, et);
	if (!et) {
		return NULL;
	}
	memset(et, 0, sizeof(struct p2d_emitter));
	et->loop = true;
	et->cfg = cfg;
	return et;
}

void 
p2d_emitter_release(struct p2d_emitter* et) {
	p2d_emitter_clear(et);
	free(et);
}

void 
p2d_emitter_clear(struct p2d_emitter* et) {
	struct p2d_particle* p = et->head;
	while (p) {
		struct p2d_particle* next = p->next;
		PS_ARRAY_FREE(PARTICLE_ARRAY, p);
		p = next;
	}

	et->head = et->tail = NULL;

	et->emit_counter = 0;
	et->particle_count = 0;
}

void 
p2d_emitter_stop(struct p2d_emitter* et) {
	et->active = false;
}

void 
p2d_emitter_start(struct p2d_emitter* et) {
	et->particle_count = 0;
	et->emit_counter = 0;
	et->active = true;
}

void 
p2d_emitter_pause(struct p2d_emitter* et) {
	et->active = false;
}

void 
p2d_emitter_resume(struct p2d_emitter* et) {
	et->active = true;
}

static inline void
_init_mode_gravity(struct p2d_emitter* et, struct p2d_particle* p, uint32_t* rand) {
	float dir = et->cfg->direction + et->cfg->direction_var * ps_random_m11(rand);
	float speed = et->cfg->mode.A.speed + et->cfg->mode.A.speed_var * ps_random_m11(rand);
	p->mode.A.speed.x = cosf(dir) * speed;
	p->mode.A.speed.y = sinf(dir) * speed;

	p->mode.A.tangential_accel = et->cfg->mode.A.tangential_accel + et->cfg->mode.A.tangential_accel_var * ps_random_m11(rand);

	p->mode.A.radial_accel = et->cfg->mode.A.radial_accel + et->cfg->mode.A.radial_accel_var * ps_random_m11(rand);

	if (et->cfg->mode.A.rotation_is_dir) {
		p->angle = atan2f(p->mode.A.speed.y, p->mode.A.speed.x);
	}
}

static inline void
_init_mode_radius(struct p2d_emitter* et, struct p2d_particle* p, uint32_t* rand) {
	float dir = et->cfg->direction + et->cfg->direction_var * ps_random_m11(rand);
	p->mode.B.direction = dir;
	p->mode.B.direction_delta = et->cfg->mode.B.direction_delta + et->cfg->mode.B.direction_delta_var * ps_random_m11(rand);

	float start_radius = et->cfg->mode.B.start_radius + et->cfg->mode.B.start_radius_var * ps_random_m11(rand);
	float end_radius = et->cfg->mode.B.end_radius + et->cfg->mode.B.end_radius_var * ps_random_m11(rand);
	p->mode.B.radius = start_radius;
	p->mode.B.radius_delta = (end_radius - start_radius) / p->life;
}

static inline void
_init_mode_spd_cos(struct p2d_emitter* et, struct p2d_particle* p, uint32_t* rand) {
	float dir = et->cfg->direction + et->cfg->direction_var * ps_random_m11(rand);
	p->mode.C.lifetime = p->life;

	float speed = et->cfg->mode.C.speed + et->cfg->mode.C.speed_var * ps_random_m11(rand);
	p->mode.C.speed.x = cosf(dir) * speed;
	p->mode.C.speed.y = sinf(dir) * speed;

	p->mode.C.cos_amplitude = et->cfg->mode.C.cos_amplitude + et->cfg->mode.C.cos_amplitude_var * ps_random_m11(rand);
	p->mode.C.cos_frequency = et->cfg->mode.C.cos_frequency + et->cfg->mode.C.cos_frequency_var * ps_random_m11(rand);
}

static inline void
_init_particle(struct p2d_emitter* et, struct p2d_particle* p) {
	uint32_t RANDSEED = rand();

	p->sym = (struct p2d_symbol*)(et->cfg->syms + RANDSEED % et->cfg->sym_count);

	p->life = et->cfg->life + et->cfg->life_var * ps_random_m11(&RANDSEED);

	p->position.x = et->cfg->position.x + et->cfg->position_var.x * ps_random_m11(&RANDSEED);
	p->position.y = et->cfg->position.y + et->cfg->position_var.y * ps_random_m11(&RANDSEED);
	p->position_ori = p->position;

	float k = 1 / p->life;

	p->angle = p->sym->angle_start;
	p->angle_delta = (p->sym->angle_end - p->sym->angle_start) * k;

	p->scale = p->sym->scale_start;
	p->scale_delta = (p->sym->scale_end - p->sym->scale_start) * k;

	p->mul_col = p->sym->mul_col_begin;
	ps_color_sub(&p->sym->mul_col_end, &p->sym->mul_col_begin, &p->mul_col_delta);
	ps_color_mul(&p->mul_col_delta, k);

	p->add_col = p->sym->add_col_begin;
	ps_color_sub(&p->sym->add_col_end, &p->sym->add_col_begin, &p->add_col_delta);
	ps_color_mul(&p->add_col_delta, k);

	if (et->cfg->mode_type == P2D_MODE_GRAVITY) {
		_init_mode_gravity(et, p, &RANDSEED);
	} else if (et->cfg->mode_type == P2D_MODE_RADIUS) {
		_init_mode_radius(et, p, &RANDSEED);
	} else if (et->cfg->mode_type == P2D_MODE_SPD_COS) {
		_init_mode_spd_cos(et, p, &RANDSEED);
	}
}

static inline void
_add_particle_random(struct p2d_emitter* et, float* mat) {
	if (!et->cfg->sym_count) {
		return;
	}

	struct p2d_particle* p;
	PS_ARRAY_ALLOC(PARTICLE_ARRAY, p);
	if (!p) {
		return;
	}

	if (mat) {
		memcpy(p->mat, mat, sizeof(p->mat));
	}
	_init_particle(et, p);

	p->next = NULL;
	if (!et->head) {
		assert(!et->tail);
		et->head = et->tail = p;
	} else {
		assert(et->tail);
		et->tail->next = p;
		et->tail = p;
	}
}

static inline void
_update_mode_gravity(struct p2d_emitter* et, float dt, struct p2d_particle* p) {
	struct ps_vec2 dir;
	dir.x = p->position.x - p->position_ori.x;
	dir.y = p->position.y - p->position_ori.y;
	ps_vec2_normalize(&dir);

	struct ps_vec2 radial, tangential;
	radial.x = dir.x * p->mode.A.radial_accel;
	radial.y = dir.y * p->mode.A.radial_accel;
	tangential.x = -dir.y * p->mode.A.tangential_accel;
	tangential.y =  dir.x * p->mode.A.tangential_accel;

	struct ps_vec2 accel;
	accel.x = radial.x + tangential.x + et->cfg->mode.A.gravity.x;
	accel.y = radial.y + tangential.y + et->cfg->mode.A.gravity.y;
	p->mode.A.speed.x += accel.x * dt;
	p->mode.A.speed.y += accel.y * dt;
	p->position.x += p->mode.A.speed.x * dt;
	p->position.y += p->mode.A.speed.y * dt;
}

static inline void
_update_mode_radius(struct p2d_emitter* et, float dt, struct p2d_particle* p) {
	p->mode.B.direction += p->mode.B.direction_delta * dt;
	p->mode.B.radius += p->mode.B.radius_delta * dt;
	p->position.x = -cosf(p->mode.B.direction) * p->mode.B.radius;
	p->position.y = -sinf(p->mode.B.direction) * p->mode.B.radius;
}

static inline void
_update_mode_spd_cos(struct p2d_emitter* et, float dt, struct p2d_particle* p) {
	struct ps_vec2 tangential_dir;
	tangential_dir.x = -p->mode.C.speed.y;
	tangential_dir.y =  p->mode.C.speed.x;
	ps_vec2_normalize(&tangential_dir);

	float tangential_spd = p->mode.C.cos_amplitude * cosf(p->mode.C.cos_frequency * (p->mode.C.lifetime - p->life));
	struct ps_vec2 speed;
	speed.x = p->mode.C.speed.x + tangential_dir.x * tangential_spd;
	speed.y = p->mode.C.speed.y + tangential_dir.y * tangential_spd;

	p->position.x += speed.x * dt;
	p->position.y += speed.y * dt;
}

static inline void
_update(struct p2d_emitter* et, float dt, struct p2d_particle* p) {
	p->angle += p->angle_delta * dt;

	p->scale += p->scale_delta * dt;

	p->mul_col.r += p->mul_col_delta.r * dt;
	p->mul_col.g += p->mul_col_delta.g * dt;
	p->mul_col.b += p->mul_col_delta.b * dt;
	p->mul_col.a += p->mul_col_delta.a * dt;

	p->add_col.r += p->add_col_delta.r * dt;
	p->add_col.g += p->add_col_delta.g * dt;
	p->add_col.b += p->add_col_delta.b * dt;
	p->add_col.a += p->add_col_delta.a * dt;

	if (et->cfg->mode_type == P2D_MODE_GRAVITY) {
		_update_mode_gravity(et, dt, p);
	} else if (et->cfg->mode_type == P2D_MODE_RADIUS) {
		_update_mode_radius(et, dt, p);
	} else if (et->cfg->mode_type == P2D_MODE_SPD_COS) {
		_update_mode_spd_cos(et, dt, p);
	}
}

void 
p2d_emitter_update(struct p2d_emitter* et, float dt, float* mat) {
	if (et->active && (et->loop || et->particle_count < et->cfg->count)) {
		float rate = et->cfg->emission_time / et->cfg->count;
		et->emit_counter += dt;
		while (et->emit_counter > rate) {
			++et->particle_count;
			_add_particle_random(et, mat);
			et->emit_counter -= rate;
		}
	}

	struct p2d_particle* prev = NULL;
	struct p2d_particle* curr = et->head;
	while (curr) {
		et->tail = curr;

		curr->life -= dt;
		if (curr->life > 0) {
			_update(et, dt, curr);
			prev = curr;
			curr = curr->next;
		} else {
			struct p2d_particle* next = curr->next;
			if (prev) {
				prev->next = next;
			}
			PS_ARRAY_FREE(PARTICLE_ARRAY, curr);
			if (et->head == curr) {
				et->head = next;
				if (!next) {
					et->tail = NULL;
				}
			}
			curr = next;
		}
	}
}

void 
p2d_emitter_draw(struct p2d_emitter* et, const void* ud) {
	struct p2d_particle* p = et->head;
	while (p) {
		RENDER_FUNC(p->sym->ud, p->mat, p->position.x, p->position.y, p->angle, p->scale, &p->mul_col, &p->add_col, ud);
		p = p->next;
	}
}

bool 
p2d_emitter_is_finished(struct p2d_emitter* et) {
	return !et->loop && et->particle_count >= et->cfg->count && !et->head;
}