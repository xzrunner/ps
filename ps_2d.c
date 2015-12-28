#include "ps_2d.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FREELIST_CAP 10000

static struct p2d_particle* FREELIST = NULL;

static void (*RENDER_FUNC)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud);

static inline struct p2d_particle*
_fetch_particle() {
	struct p2d_particle* p = FREELIST;
	if (!p) {
		printf("err! no free: _add \n");
	} else {
		FREELIST = p->next;
	}
	return p;
}

static inline void
_return_particle(struct p2d_particle* p) {
	p->next = FREELIST;
	FREELIST = p;
}

void 
p2d_init() {
	int sz = sizeof(struct p2d_particle) * FREELIST_CAP;
	struct p2d_particle* p = (struct p2d_particle*)malloc(sz);
	if (!p) {
		printf("malloc err: p2d_init !\n");
		return;
	}
	memset(p, 0, sz);

	for (int i = 0; i < FREELIST_CAP - 1; ++i) {
		p[i].next = &p[i + 1];
	}
	p[FREELIST_CAP - 1].next = NULL;

	FREELIST = p;
}

void 
p2d_regist_cb(void (*render_func)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud)) {
	RENDER_FUNC = render_func;	
}

struct p2d_emitter* 
p2d_create(struct p2d_emitter_cfg* cfg) {
	struct p2d_emitter* et = (struct p2d_emitter*)malloc(SIZEOF_P2D_PARTICLE_SYSTEM);
	memset(et, 0, SIZEOF_P2D_PARTICLE_SYSTEM);

	et->active = et->loop = false;

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
		_return_particle(p);
		p = next;
	}

	et->head = et->tail = NULL;

	et->emit_counter = 0;
}

static inline void
_pause(struct p2d_emitter* et) {
	et->active = false;
}

static inline void
_stop(struct p2d_emitter* et) {
	_pause(et);
	et->emit_counter = 0;
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

	p->symbol = (struct p2d_symbol*)(et->cfg->symbols + RANDSEED % et->cfg->symbol_count);

	p->life = et->cfg->life + et->cfg->life_var * ps_random_m11(&RANDSEED);

	p->position.x = et->cfg->position.x + et->cfg->position_var.x * ps_random_m11(&RANDSEED);
	p->position.y = et->cfg->position.y + et->cfg->position_var.y * ps_random_m11(&RANDSEED);
	p->position_ori = p->position;

	float k = 1 / p->life;

	p->angle = p->symbol->angle_start;
	p->angle_delta = (p->symbol->angle_end - p->symbol->angle_start) * k;

	p->scale = p->symbol->scale_start;
	p->scale_delta = (p->symbol->scale_end - p->symbol->scale_start) * k;

	p->col_mul = p->symbol->col_mul_start;
	ps_color_sub(&p->symbol->col_mul_end, &p->symbol->col_mul_start, &p->col_mul_delta);
	ps_color_mul(&p->col_mul_delta, k);

	p->col_add = p->symbol->col_add_start;
	ps_color_sub(&p->symbol->col_add_end, &p->symbol->col_add_start, &p->col_add_delta);
	ps_color_mul(&p->col_add_delta, k);

	if (et->cfg->mode_type == P2D_MODE_GRAVITY) {
		_init_mode_gravity(et, p, &RANDSEED);
	} else if (et->cfg->mode_type == P2D_MODE_RADIUS) {
		_init_mode_radius(et, p, &RANDSEED);
	} else if (et->cfg->mode_type == P2D_MODE_SPD_COS) {
		_init_mode_spd_cos(et, p, &RANDSEED);
	}
}

static inline void
_add_particle(struct p2d_emitter* et) {
	if (!et->cfg->symbol_count) {
		return;
	}

	struct p2d_particle* p = _fetch_particle();
	if (!p) {
		return;
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
_remove_particle(struct p2d_emitter* et, struct p2d_particle* p) {
	_return_particle(p);
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

	p->col_mul.r += p->col_mul_delta.r * dt;
	p->col_mul.g += p->col_mul_delta.g * dt;
	p->col_mul.b += p->col_mul_delta.b * dt;
	p->col_mul.a += p->col_mul_delta.a * dt;

	p->col_add.r += p->col_add_delta.r * dt;
	p->col_add.g += p->col_add_delta.g * dt;
	p->col_add.b += p->col_add_delta.b * dt;
	p->col_add.a += p->col_add_delta.a * dt;

	if (et->cfg->mode_type == P2D_MODE_GRAVITY) {
		_update_mode_gravity(et, dt, p);
	} else if (et->cfg->mode_type == P2D_MODE_RADIUS) {
		_update_mode_radius(et, dt, p);
	} else if (et->cfg->mode_type == P2D_MODE_SPD_COS) {
		_update_mode_spd_cos(et, dt, p);
	}
}

void 
p2d_emitter_update(struct p2d_emitter* et, float dt) {
	if (et->active) {
		float rate = et->cfg->emission_time / et->cfg->count;
		et->emit_counter += dt;
		while (et->emit_counter > rate) {
			_add_particle(et);
			et->emit_counter -= rate;
		}
	} else {
		for (int i = 0; i < et->cfg->count; ++i) {
			_add_particle(et);
		}
		_stop(et);
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
			_remove_particle(et, curr);
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
		RENDER_FUNC(p->symbol->ud, p->position.x, p->position.y, p->angle, p->scale, &p->col_mul, &p->col_add, ud);
		p = p->next;
	}
}