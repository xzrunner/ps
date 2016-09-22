#include "ps_3d.h"
#include "ps_array.h"

#include <logger.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_PARTICLE_SZ 10000
#define MAX_EMITTER_SZ	2048

#define PI 3.1415926

//#define EMITTER_LOG

static struct p3d_particle*	PARTICLE_ARRAY		= NULL;
static struct p3d_particle*	PARTICLE_ARRAY_HEAD	= NULL;
static struct p3d_emitter*	EMITTER_ARRAY		= NULL;
static struct p3d_emitter*	EMITTER_ARRAY_HEAD	= NULL;

static void (*BLEND_BEGIN_FUNC)(int blend);
static void (*BLEND_END_FUNC)();
static void (*RENDER_FUNC)(void* sym, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, const void* ud);
static void (*ADD_FUNC)(struct p3d_particle*, void* ud);
static void (*REMOVE_FUNC)(struct p3d_particle*, void* ud);

void 
p3d_init() {
    if (!PARTICLE_ARRAY_HEAD) {
        int sz = sizeof(struct p3d_particle) * MAX_PARTICLE_SZ;
        PARTICLE_ARRAY_HEAD = (struct p3d_particle*)malloc(sz);
        if (!PARTICLE_ARRAY_HEAD) {
            LOGW("%s", "malloc err: p3d_init particle");
            return;
        }
    }
    if (!EMITTER_ARRAY_HEAD) {
        int sz = sizeof(struct p3d_emitter) * MAX_EMITTER_SZ;
        EMITTER_ARRAY_HEAD = (struct p3d_emitter*)malloc(sz);
        if (!EMITTER_ARRAY_HEAD) {
            LOGW("%s", "malloc err: p3d_init emitter");
            return;
        }
    }
    
	p3d_clear();
}

void 
p3d_regist_cb(void (*blend_begin_func)(int blend),
			  void (*blend_end_func)(),
			  void (*render_func)(void* sym, float* mat, float x, float y, float angle, float scale, struct ps_color* mul_col, struct ps_color* add_col, const void* ud),
			  void (*add_func)(struct p3d_particle*, void* ud),
			  void (*remove_func)(struct p3d_particle*, void* ud)) {
	BLEND_BEGIN_FUNC = blend_begin_func;
	BLEND_END_FUNC = blend_end_func;
	RENDER_FUNC = render_func;
	ADD_FUNC = add_func;
	REMOVE_FUNC = remove_func;
}

#ifdef EMITTER_LOG
static int et_count = 0;
#endif // EMITTER_LOG

void 
p3d_clear() {
	int sz = sizeof(struct p3d_particle) * MAX_PARTICLE_SZ;
	memset(PARTICLE_ARRAY_HEAD, 0, sz);
	PS_ARRAY_INIT(PARTICLE_ARRAY_HEAD, MAX_PARTICLE_SZ);
    PARTICLE_ARRAY = PARTICLE_ARRAY_HEAD;

	sz = sizeof(struct p3d_emitter) * MAX_EMITTER_SZ;
	memset(EMITTER_ARRAY_HEAD, 0, sz);
	PS_ARRAY_INIT(EMITTER_ARRAY_HEAD, MAX_EMITTER_SZ);
    EMITTER_ARRAY = EMITTER_ARRAY_HEAD;

#ifdef EMITTER_LOG
	et_count = 0;
#endif // EMITTER_LOG
}

struct p3d_emitter* 
p3d_emitter_create(const struct p3d_emitter_cfg* cfg) {
	struct p3d_emitter* et;	
	PS_ARRAY_ALLOC(EMITTER_ARRAY, et);
	if (!et) {
		return NULL;
	}
#ifdef EMITTER_LOG
	++et_count;
	LOGD("++ add %d %p \n", et_count, et);
#endif // EMITTER_LOG
	memset(et, 0, sizeof(struct p3d_emitter));
	et->loop = true;
	et->cfg = cfg;
	return et;
}

void 
p3d_emitter_release(struct p3d_emitter* et) {
#ifdef EMITTER_LOG
	--et_count;
	LOGD("-- del %d %p\n", et_count, et);
#endif // EMITTER_LOG

	p3d_emitter_clear(et);
	PS_ARRAY_FREE(EMITTER_ARRAY, et);
}

void 
p3d_emitter_clear(struct p3d_emitter* et) {
	struct p3d_particle* p = et->head;
	while (p) {
		struct p3d_particle* next = p->next;
		PS_ARRAY_FREE(PARTICLE_ARRAY, p);
		p = next;
	}

	et->head = et->tail = NULL;

	et->emit_counter = 0;
	et->particle_count = 0;
}

void 
p3d_emitter_stop(struct p3d_emitter* et) {
	et->active = false;
}

void 
p3d_emitter_start(struct p3d_emitter* et) {
	et->particle_count = 0;
	et->emit_counter = 0;
	et->active = true;
	et->static_mode_finished = false;
}

void 
p3d_emitter_pause(struct p3d_emitter* et) {
	et->active = false;
}

void 
p3d_emitter_resume(struct p3d_emitter* et) {
	et->active = true;
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
_init_particle(struct p3d_emitter* et, struct p3d_particle* p, struct p3d_symbol* sym) {
	uint32_t RANDSEED = rand();

	if (sym) {
		p->cfg.sym = sym;
	} else {
		p->cfg.sym = (struct p3d_symbol*)(et->cfg->syms + RANDSEED % et->cfg->sym_count);
	}

	p->life = et->cfg->life + et->cfg->life_var * ps_random_m11(&RANDSEED);
	p->cfg.lifetime = p->life;
	
	p->cfg.dir.x = et->cfg->hori + et->cfg->hori_var * ps_random_m11(&RANDSEED);
	p->cfg.dir.y = et->cfg->vert + et->cfg->vert_var * ps_random_m11(&RANDSEED);

	_trans_coords2d(et->cfg->start_radius, et->cfg->start_height, p->cfg.dir.x, &p->pos);

	float radial_spd = et->cfg->radial_spd + et->cfg->radial_spd_var * ps_random_m11(&RANDSEED);
	_trans_coords3d(radial_spd, p->cfg.dir.x, p->cfg.dir.y, &p->spd);
	memcpy(&p->cfg.spd_dir, &p->spd, sizeof(p->spd));

	p->cfg.dis_region = et->cfg->dis_region + et->cfg->dis_region_var * ps_random_m11(&RANDSEED);
	p->dis_curr_len = 0;
	float dis_angle = PI * ps_random_m11(&RANDSEED);
	p->dis_dir.x = cosf(dis_angle);
	p->dis_dir.y = sinf(dis_angle);
	p->cfg.dis_spd = et->cfg->dis_spd + et->cfg->dis_spd_var * ps_random_m11(&RANDSEED);

	p->cfg.linear_acc = et->cfg->linear_acc + et->cfg->linear_acc_var * ps_random_m11(&RANDSEED);

	p->cfg.tangential_spd = et->cfg->tangential_spd + et->cfg->tangential_spd_var * ps_random_m11(&RANDSEED);

	p->cfg.angular_spd = et->cfg->angular_spd + et->cfg->angular_spd_var * ps_random_m11(&RANDSEED);

	p->angle = p->cfg.sym->angle + p->cfg.sym->angle_var * ps_random_m11(&RANDSEED);

// 	// todo bind_ps
// 	if (p->cfg.sym->bind_ps_cfg) {
// 		int num = et->end - et->start;
// 		p->bind_ps = p3d_create(num, p->cfg.sym->bind_ps_cfg);
// 	} else if (p->bind_ps) {
// 		free(p->bind_ps);
// 		p->bind_ps = NULL;
// 	}
}

static void
_add_particle(struct p3d_emitter* et, float* mat, struct p3d_symbol* sym) {
	struct p3d_particle* p;
	PS_ARRAY_ALLOC(PARTICLE_ARRAY, p);
	if (!p) {
		return;
	}

	if (mat) {
		memcpy(p->mat, mat, sizeof(p->mat));
	} else {
		memset(p->mat, 0, sizeof(p->mat));
		p->mat[0] = p->mat[3] = 1024;
	}
	_init_particle(et, p, sym);

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

static void
_add_particle_static(struct p3d_emitter* et, float* mat) {
	for (int i = 0; i < et->cfg->sym_count; ++i) {
		struct p3d_symbol* sym = &et->cfg->syms[i];
		for (int j = 0; j < sym->count; ++j) {
			_add_particle(et, mat, sym);
		}
	}
}

static inline void
_add_particle_random(struct p3d_emitter* et, float* mat) {
	if (!et->cfg->sym_count) {
		return;
	}

	_add_particle(et, mat, NULL);
}

static inline void
_remove_particle(struct p3d_emitter* et, struct p3d_particle* p) {
	PS_ARRAY_FREE(PARTICLE_ARRAY, p);
	if (REMOVE_FUNC) {
		REMOVE_FUNC(p, et->ud);
	}
}

static inline void
_update_disturbance_speed(struct p3d_emitter* et, float dt, struct p3d_particle* p) {
	if (p->cfg.dis_spd == 0) {
		return;
	}

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

	// tangential speed
	struct ps_vec2 tan_spd;
	tan_spd.x = -p->pos.y;
	tan_spd.y =  p->pos.x;
	ps_vec2_normalize(&tan_spd);
	tan_spd.x *= p->cfg.tangential_spd;
	tan_spd.y *= p->cfg.tangential_spd;
	p->spd.x += tan_spd.x;
	p->spd.y += tan_spd.y;

	// normal acceleration
	float velocity = ps_vec3_len(&p->spd);
	if (velocity != 0) {
		float linear_acc = p->cfg.linear_acc * dt;
		for (int i = 0; i < 3; ++i) {
			float v = p->spd.xyz[i];
			float dv = linear_acc * p->spd.xyz[i] / velocity;
			if (dv * v < 0 && fabs(dv) > fabs(v)) {
				p->spd.xyz[i] = 0;
			} else {
				p->spd.xyz[i] += dv;
			}
		}
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
p3d_emitter_update(struct p3d_emitter* et, float dt, float* mat) {
	if (et->active) {
		float rate = et->cfg->emission_time / et->cfg->count;
		et->emit_counter += dt;
		if (et->loop) {
			while (et->emit_counter > rate) {
				_add_particle_random(et, mat);
				et->emit_counter -= rate;
			}
		} else {
			if (!et->cfg->static_mode) {
				while (et->emit_counter > rate && et->particle_count < et->cfg->count) {
					_add_particle_random(et, mat);
					++et->particle_count;
					et->emit_counter -= rate;
				}
			} else {
				if (!et->static_mode_finished) {
					_add_particle_static(et, mat);
					et->static_mode_finished = true;
				}
			}
		}
	}

	struct p3d_particle* prev = NULL;
	struct p3d_particle* curr = et->head;
	while (curr) {
		et->tail = curr;

		if (curr->bind_ps) {
			p3d_emitter_update(curr->bind_ps, dt, mat);
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

static inline void
_color_lerp(struct ps_color* begin, struct ps_color* end, struct ps_color* lerp, float proc) {
	lerp->r = proc * (end->r - begin->r) + begin->r;
	lerp->g = proc * (end->g - begin->g) + begin->g;
	lerp->b = proc * (end->b - begin->b) + begin->b;
	lerp->a = proc * (end->a - begin->a) + begin->a;
}

void 
p3d_emitter_draw(struct p3d_emitter* et, const void* ud) {
	struct ps_vec2 pos;
	struct ps_color mul_col, add_col;

	BLEND_BEGIN_FUNC(et->cfg->blend);

	struct p3d_particle* p = et->head;
	while (p) {
		float proc = (p->cfg.lifetime - p->life) / p->cfg.lifetime;

		ps_vec3_projection(&p->pos, &pos);

		float scale = proc * (p->cfg.sym->scale_end - p->cfg.sym->scale_start) + p->cfg.sym->scale_start;

		_color_lerp(&p->cfg.sym->mul_col_begin, &p->cfg.sym->mul_col_end, &mul_col, proc);
		_color_lerp(&p->cfg.sym->add_col_begin, &p->cfg.sym->add_col_end, &add_col, proc);
		if (p->life < et->cfg->fadeout_time) {
			mul_col.a *= p->life / et->cfg->fadeout_time;
		}

		RENDER_FUNC(p->cfg.sym->ud, p->mat, pos.x, pos.y, p->angle, scale, &mul_col, &add_col, ud);

		p = p->next;
	}

	BLEND_END_FUNC();
}

bool 
p3d_emitter_is_finished(struct p3d_emitter* et) {
	return !et->loop && et->particle_count >= et->cfg->count && !et->head;
}