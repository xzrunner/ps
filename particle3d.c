#include "particle3d.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FREELIST_CAP 10000

struct particle_buf {
	struct p3d_particle* head;

	int cap;
	int used;
};

// static emitter_buf {
// 
// };

static struct p3d_particle* FREELIST = NULL;

static int PARTICLE_SIZE = 0;

static void (*RENDER_FUNC)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud);
static void (*ADD_FUNC)(struct p3d_particle*, void* ud);
static void (*REMOVE_FUNC)(struct p3d_particle*, void* ud);

static inline struct p3d_particle*
_fetch_particle() {
	struct p3d_particle* p = FREELIST;
	if (!p) {
		printf("err! no free: _add \n");
	} else {
		FREELIST = p->next;
		++PARTICLE_SIZE;
	}
	return p;
}

static inline void
_return_particle(struct p3d_particle* p) {
	--PARTICLE_SIZE;
	p->next = FREELIST;
	FREELIST = p;
}

void 
p3d_init() {
	int sz = sizeof(struct p3d_particle) * FREELIST_CAP;
	struct p3d_particle* p = (struct p3d_particle*)malloc(sz);
	if (!p) {
		printf("malloc err: p3d_init !\n");
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
p3d_regist_cb(void (*render_func)(void* symbol, float x, float y, float angle, float scale, struct ps_color4f* mul_col, struct ps_color4f* add_col, const void* ud),
			  void (*add_func)(struct p3d_particle*, void* ud),
			  void (*remove_func)(struct p3d_particle*, void* ud)) {
	RENDER_FUNC = render_func;
	ADD_FUNC = add_func;
	REMOVE_FUNC = remove_func;
}

struct p3d_emitter* 
p3d_emitter_create(struct p3d_emitter_cfg* cfg) {
	struct p3d_emitter* et = (struct p3d_emitter*)malloc(SIZEOF_P3D_PARTICLE_SYSTEM);
	memset(et, 0, SIZEOF_P3D_PARTICLE_SYSTEM);

	et->active = false;
	et->loop = true;

	et->cfg = cfg;

	return et;
}

void 
p3d_emitter_release(struct p3d_emitter* et) {
	p3d_emitter_clear(et);
	free(et);
}

void 
p3d_emitter_clear(struct p3d_emitter* et) {
	struct p3d_particle* p = et->head;
	while (p) {
		struct p3d_particle* next = p->next;
		_return_particle(p);
		p = next;
	}

	et->head = et->tail = NULL;

	et->emit_counter = 0;
	et->particle_count = 0;
}

// static inline void
// _resume(struct particle_system_3d* et) {
// 	et->active = true;
// }

static inline void
_pause(struct p3d_emitter* et) {
	et->active = false;
}

static inline void
_stop(struct p3d_emitter* et) {
	_pause(et);
	et->emit_counter = 0;
}

static inline void
_trans_coords3d(float r, float hori, float vert, struct ps_vec3* pos) {
	float dxy = r * cosf(vert);
	float dz = r * sinf(vert);
	pos->x = dxy * cosf(hori);
	pos->y = dxy * sinf(hori);
	pos->z = dz;
}

static inline void
_trans_coords2d(float r, float h, float hori, struct ps_vec3* pos) {
	pos->x = r * cosf(hori);
	pos->y = r * sinf(hori);
	pos->z = h;
}

static inline void
_init_particle(struct p3d_emitter* et, struct p3d_particle* p) {
	uint32_t RANDSEED = rand();

	p->cfg.symbol = (struct p3d_symbol*)(et->cfg->symbols + RANDSEED % et->cfg->symbol_count);

	p->life = et->cfg->life + et->cfg->life_var * ps_random_m11(&RANDSEED);
	p->cfg.lifetime = p->life;
	
	p->cfg.dir.x = et->cfg->hori + et->cfg->hori_var * ps_random_m11(&RANDSEED);
	p->cfg.dir.y = et->cfg->vert + et->cfg->vert_var * ps_random_m11(&RANDSEED);

	_trans_coords2d(et->cfg->start_radius, et->cfg->start_height, p->cfg.dir.x, &p->pos);

	float spd = et->cfg->spd + et->cfg->spd_var * ps_random_m11(&RANDSEED);
	_trans_coords3d(spd, p->cfg.dir.x, p->cfg.dir.y, &p->spd);
	memcpy(&p->cfg.spd_dir, &p->spd, sizeof(p->spd));

	p->cfg.dis_region = et->cfg->dis_region + et->cfg->dis_region_var * ps_random_m11(&RANDSEED);
	p->dis_curr_len = 0;
	float dis_angle = PI * ps_random_m11(&RANDSEED);
	p->dis_dir.x = cosf(dis_angle);
	p->dis_dir.y = sinf(dis_angle);
	p->cfg.dis_spd = et->cfg->dis_spd + et->cfg->dis_spd_var * ps_random_m11(&RANDSEED);

	p->cfg.linear_acc = et->cfg->linear_acc + et->cfg->linear_acc_var * ps_random_m11(&RANDSEED);

	p->cfg.angular_spd = et->cfg->angular_spd + et->cfg->angular_spd_var * ps_random_m11(&RANDSEED);

	p->angle = p->cfg.symbol->angle + p->cfg.symbol->angle_var * ps_random_m11(&RANDSEED);

// 	// todo bind_ps
// 	if (p->cfg.symbol->bind_ps_cfg) {
// 		int num = et->end - et->start;
// 		p->bind_ps = p3d_create(num, p->cfg.symbol->bind_ps_cfg);
// 	} else if (p->bind_ps) {
// 		free(p->bind_ps);
// 		p->bind_ps = NULL;
// 	}
}

static inline void
_add_particle(struct p3d_emitter* et) {
	if (!et->cfg->symbol_count) {
		return;
	}

	struct p3d_particle* p = _fetch_particle();
	if (!p) {
		return;
	}

	_init_particle(et, p);

	if (ADD_FUNC) {
		ADD_FUNC(p, et->ud);
	}

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
_remove_particle(struct p3d_emitter* et, struct p3d_particle* p) {
	_return_particle(p);

	if (REMOVE_FUNC) {
		REMOVE_FUNC(p, et->ud);
	}
}

static inline void
_update_disturbance_speed(struct p3d_emitter* et, float dt, struct p3d_particle* p) {
	// stop disturbance after touch the ground
	if (et->cfg->ground != P3D_NO_GROUND && 
		fabs(p->pos.z) < 1) {
		return;
	}

	struct ps_vec3 dis_dir;
	dis_dir.x = p->dis_dir.x;
	dis_dir.y = p->dis_dir.y;
	dis_dir.z = - (p->cfg.spd_dir.x * dis_dir.x + p->cfg.spd_dir.y * dis_dir.y) / p->cfg.spd_dir.z;
	ps_vec3_normalize(&dis_dir);

	float s = p->cfg.dis_spd * dt;
	for (int i = 0; i < 3; ++i) {
		p->spd.xyz[i] += dis_dir.xyz[i] * s;
	}
	p->dis_curr_len += s;
	if (p->dis_curr_len > p->cfg.dis_region) {
		p->dis_dir.x = -p->dis_dir.x;
		p->dis_dir.y = -p->dis_dir.y;
		p->dis_curr_len = -p->cfg.dis_region;
	}
}

static inline void
_update_speed(struct p3d_emitter* et, float dt, struct p3d_particle* p) {
	// gravity
	p->spd.z -= et->cfg->gravity * dt;

	// normal acceleration
	float velocity = ps_vec3_len(&p->spd);
	float linear_acc = p->cfg.linear_acc * dt;
	for (int i = 0; i < 3; ++i) {
		p->spd.xyz[i] += linear_acc * p->spd.xyz[i] / velocity;
	}

	_update_disturbance_speed(et, dt, p);
}

static inline void
_update_angle(struct p3d_emitter* et, float dt, struct p3d_particle* p) {
	if (et->cfg->orient_to_movement) {
		struct ps_vec2 pos_old, pos_new;
		ps_vec3_projection(&p->pos, &pos_old);
		
		struct ps_vec3 pos;
		for (int i = 0; i < 3; ++i) {
			pos.xyz[i] = p->pos.xyz[i] + p->spd.xyz[i] * dt;
		}
		ps_vec3_projection(&pos, &pos_new);

		// stop update angle when move slowly
		if (fabs(pos_new.x - pos_old.x) < 1 &&
			fabs(pos_new.y - pos_old.y) < 1) {
			return;
		}

		p->angle = atan2f(pos_new.y - pos_old.y, pos_new.x - pos_old.x) - PI * 0.5f;
	} else {
		// stop update angle after touch the ground
		if (et->cfg->ground != P3D_NO_GROUND &&
			fabs(p->pos.z) < 1) {
			return;
		}

		p->angle += p->cfg.angular_spd * dt;
	}
}

static inline void
_update_with_ground(struct p3d_emitter* et, struct p3d_particle* p) {
	if (p->pos.z >= 0) {
		return;
	}

	switch (et->cfg->ground) {
	case P3D_NO_GROUND:
		break;
	case P3D_GROUND_WITH_BOUNCE:
		p->pos.z *= -0.2f;
		p->spd.x *= 0.4f;
		p->spd.y *= 0.4f;
		p->spd.z *= -0.2f;
		break;
	case P3D_GROUND_WITHOUT_BOUNCE:
		p->pos.z = 0;
		memset(p->spd.xyz, 0, sizeof(p->spd));
		break;
	}
}

static inline void
_update_position(struct p3d_emitter* et, float dt, struct p3d_particle* p) {
	for (int i = 0; i < 3; ++i) {
		p->pos.xyz[i] += p->spd.xyz[i] * dt;
	}
	_update_with_ground(et, p);
}

void 
p3d_emitter_update(struct p3d_emitter* et, float dt) {
	if (et->active && (et->loop || et->particle_count < et->cfg->count)) {
		float rate = et->cfg->emission_time / et->cfg->count;
		et->emit_counter += dt;
		while (et->emit_counter > rate) {
			++et->particle_count;
			_add_particle(et);
			et->emit_counter -= rate;
		}
	}

	struct p3d_particle* prev = NULL;
	struct p3d_particle* curr = et->head;
	while (curr) {
		et->tail = curr;

		if (curr->bind_ps) {
			p3d_emitter_update(curr->bind_ps, dt);
		}

		curr->life -= dt;
		if (curr->life > 0) {
			_update_speed(et, dt, curr);
			_update_angle(et, dt, curr);
			_update_position(et, dt, curr);
			prev = curr;
			curr = curr->next;
		} else {
			struct p3d_particle* next = curr->next;
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
p3d_emitter_draw(struct p3d_emitter* et, const void* ud) {
	struct ps_vec2 pos;
	struct ps_color4f mul_col;

	struct p3d_particle* p = et->head;
	while (p) {
		float proc = (p->cfg.lifetime - p->life) / p->cfg.lifetime;

		ps_vec3_projection(&p->pos, &pos);

		float scale = proc * (p->cfg.symbol->scale_end - p->cfg.symbol->scale_start) + p->cfg.symbol->scale_start;

		mul_col = p->cfg.symbol->col_mul;
		if (p->life < et->cfg->fadeout_time) {
			mul_col.a *= p->life / et->cfg->fadeout_time;
		}
		float alpha = proc * (p->cfg.symbol->alpha_end - p->cfg.symbol->alpha_start) + p->cfg.symbol->alpha_start;
		mul_col.a *= alpha;

//		RENDER_FUNC(p->cfg.symbol->ud, pos.x + p->init_pos.x, pos.y + p->init_pos.y, p->angle, scale, &mul_col, &p->cfg.symbol->col_add, ud);
		RENDER_FUNC(p->cfg.symbol->ud, pos.x, pos.y, p->angle, scale, &mul_col, &p->cfg.symbol->col_add, ud);
		p = p->next;
	}
}

void 
p3d_draw() {
	
}