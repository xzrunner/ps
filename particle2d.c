#include "particle2d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static void (*RENDER_FUNC)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud);

void 
p2d_init(void (*render_func)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud)) {
	RENDER_FUNC = render_func;	
}

static inline void 
_ps_init(struct p2d_particle_system* ps, int num) {
	ps->last = ps->start = (struct p2d_particle*)(ps + 1);
	ps->end = ps->last + num;

	ps->emit_counter = 0;
	ps->is_active = ps->is_loop = false;
}

struct p2d_particle_system* 
p2d_create(int num, struct p2d_ps_config* cfg) {
	int sz = SIZEOF_P2D_PARTICLE_SYSTEM + num * SIZEOF_P2D_PARTICLE;
	struct p2d_particle_system* ps = (struct p2d_particle_system*)malloc(sz);
	memset(ps, 0, sz);
	ps->cfg = cfg;
	_ps_init(ps, num);
	return ps;
}

void 
p2d_release(struct p2d_particle_system* ps) {
	free(ps);
}

struct p2d_particle_system* 
p2d_create_with_mem(void* mem, int num, struct p2d_ps_config* cfg) {
	int sz = SIZEOF_P2D_PARTICLE_SYSTEM + num * SIZEOF_P2D_PARTICLE;
	struct p2d_particle_system* ps = (struct p2d_particle_system*)mem;
	memset(ps, 0, sz);
	ps->cfg = cfg;
	_ps_init(ps, num);
	return ps;
}

static inline void
_pause(struct p2d_particle_system* ps) {
	ps->is_active = false;
}

static inline void
_stop(struct p2d_particle_system* ps) {
	_pause(ps);
	ps->emit_counter = 0;
}

static inline bool 
_is_full(struct p2d_particle_system* ps) {
	return ps->last == ps->end;	
}

static inline bool
_is_empty(struct p2d_particle_system* ps) {
	return ps->start == ps->last;
}

static inline void
_init_mode_gravity(struct p2d_particle_system* ps, struct p2d_particle* p, uint32_t* rand) {
	float dir = ps->cfg->direction + ps->cfg->direction_var * ps_random_m11(rand);
	float speed = ps->cfg->mode.A.speed + ps->cfg->mode.A.speed_var * ps_random_m11(rand);
	p->mode.A.speed.x = cosf(dir) * speed;
	p->mode.A.speed.y = sinf(dir) * speed;

	p->mode.A.tangential_accel = ps->cfg->mode.A.tangential_accel + ps->cfg->mode.A.tangential_accel_var * ps_random_m11(rand);

	p->mode.A.radial_accel = ps->cfg->mode.A.radial_accel + ps->cfg->mode.A.radial_accel_var * ps_random_m11(rand);

	if (ps->cfg->mode.A.rotation_is_dir) {
		p->angle = atan2f(p->mode.A.speed.y, p->mode.A.speed.x);
	}
}

static inline void
_init_mode_radius(struct p2d_particle_system* ps, struct p2d_particle* p, uint32_t* rand) {
	float dir = ps->cfg->direction + ps->cfg->direction_var * ps_random_m11(rand);
	p->mode.B.direction = dir;
	p->mode.B.direction_delta = ps->cfg->mode.B.direction_delta + ps->cfg->mode.B.direction_delta_var * ps_random_m11(rand);

	float start_radius = ps->cfg->mode.B.start_radius + ps->cfg->mode.B.start_radius_var * ps_random_m11(rand);
	float end_radius = ps->cfg->mode.B.end_radius + ps->cfg->mode.B.end_radius_var * ps_random_m11(rand);
	p->mode.B.radius = start_radius;
	p->mode.B.radius_delta = (end_radius - start_radius) / p->life;
}

static inline void
_init_mode_spd_cos(struct p2d_particle_system* ps, struct p2d_particle* p, uint32_t* rand) {
	float dir = ps->cfg->direction + ps->cfg->direction_var * ps_random_m11(rand);
	p->mode.C.lifetime = p->life;

	float speed = ps->cfg->mode.C.speed + ps->cfg->mode.C.speed_var * ps_random_m11(rand);
	p->mode.C.speed.x = cosf(dir) * speed;
	p->mode.C.speed.y = sinf(dir) * speed;

	p->mode.C.cos_amplitude = ps->cfg->mode.C.cos_amplitude + ps->cfg->mode.C.cos_amplitude_var * ps_random_m11(rand);
	p->mode.C.cos_frequency = ps->cfg->mode.C.cos_frequency + ps->cfg->mode.C.cos_frequency_var * ps_random_m11(rand);
}

static inline void
_init_particle(struct p2d_particle_system* ps, struct p2d_particle* p) {
	uint32_t RANDSEED = rand();

	p->symbol = (struct p2d_symbol*)(ps->cfg->symbols + RANDSEED % ps->cfg->symbol_count);

	p->life = ps->cfg->life + ps->cfg->life_var * ps_random_m11(&RANDSEED);

	p->position.x = ps->cfg->position.x + ps->cfg->position_var.x * ps_random_m11(&RANDSEED);
	p->position.y = ps->cfg->position.y + ps->cfg->position_var.y * ps_random_m11(&RANDSEED);
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

	if (ps->cfg->mode_type == P2D_MODE_GRAVITY) {
		_init_mode_gravity(ps, p, &RANDSEED);
	} else if (ps->cfg->mode_type == P2D_MODE_RADIUS) {
		_init_mode_radius(ps, p, &RANDSEED);
	} else if (ps->cfg->mode_type == P2D_MODE_SPD_COS) {
		_init_mode_spd_cos(ps, p, &RANDSEED);
	}
}

static inline void
_add(struct p2d_particle_system* ps) {
	if (!_is_full(ps) && ps->cfg->symbol_count) {
		_init_particle(ps, 	ps->last++);
	}
}

static inline void
_remove(struct p2d_particle_system* ps, struct p2d_particle* p) {
	if (!_is_empty(ps)) {
		*p = *(--ps->last);
	}
}

static inline void
_update_mode_gravity(struct p2d_particle_system* ps, float dt, struct p2d_particle* p) {
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
	accel.x = radial.x + tangential.x + ps->cfg->mode.A.gravity.x;
	accel.y = radial.y + tangential.y + ps->cfg->mode.A.gravity.y;
	p->mode.A.speed.x += accel.x * dt;
	p->mode.A.speed.y += accel.y * dt;
	p->position.x += p->mode.A.speed.x * dt;
	p->position.y += p->mode.A.speed.y * dt;
}

static inline void
_update_mode_radius(struct p2d_particle_system* ps, float dt, struct p2d_particle* p) {
	p->mode.B.direction += p->mode.B.direction_delta * dt;
	p->mode.B.radius += p->mode.B.radius_delta * dt;
	p->position.x = -cosf(p->mode.B.direction) * p->mode.B.radius;
	p->position.y = -sinf(p->mode.B.direction) * p->mode.B.radius;
}

static inline void
_update_mode_spd_cos(struct p2d_particle_system* ps, float dt, struct p2d_particle* p) {
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
_update(struct p2d_particle_system* ps, float dt, struct p2d_particle* p) {
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

	if (ps->cfg->mode_type == P2D_MODE_GRAVITY) {
		_update_mode_gravity(ps, dt, p);
	} else if (ps->cfg->mode_type == P2D_MODE_RADIUS) {
		_update_mode_radius(ps, dt, p);
	} else if (ps->cfg->mode_type == P2D_MODE_SPD_COS) {
		_update_mode_spd_cos(ps, dt, p);
	}
}

void 
p2d_update(struct p2d_particle_system* ps, float dt) {
	if (ps->is_active) {
		float rate = ps->cfg->emission_time / ps->cfg->count;
		ps->emit_counter += dt;
		while (ps->emit_counter > rate) {
			_add(ps);
			ps->emit_counter -= rate;
		}
	} else {
		for (int i = 0; i < ps->cfg->count; ++i) {
			_add(ps);
		}
		_stop(ps);
	}

	struct p2d_particle* p = ps->start;
	while (p != ps->last) {
		p->life -= dt;
		if (p->life > 0) {
			_update(ps, dt, p);
			++p;
		} else {
			_remove(ps, p);
			if (p >= ps->last) {
				return;
			}
		}
	}
}

void 
p2d_draw(struct p2d_particle_system* ps, const void* ud) {
	struct p2d_particle* p = ps->start;
	while (p != ps->last) {
		RENDER_FUNC(p->symbol->ud, p->position.x, p->position.y, p->angle, p->scale, &p->col_mul, &p->col_add, ud);
		++p;
	}
}