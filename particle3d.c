#include "particle3d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void 
_ps_init(struct p3d_particle_system* ps, int num) {
	ps->last = ps->start = (struct p3d_particle*)(ps + 1);
	ps->end = ps->start + num;

	ps->emit_counter = 0;
	ps->particle_count = 0;

	ps->active = false;
}

struct p3d_particle_system* 
p3d_create(int num, struct p3d_ps_config* cfg) {
	int sz = SIZEOF_P3D_PARTICLE_SYSTEM + num * SIZEOF_P3D_PARTICLE;
	struct p3d_particle_system* ps = (struct p3d_particle_system*)malloc(sz);
	memset(ps, 0, sz);
	ps->cfg = cfg;
	_ps_init(ps, num);
	return ps;
}

void 
p3d_release(struct p3d_particle_system* ps)
{
	free(ps);
}

struct p3d_particle_system* 
p3d_create_with_mem(void* mem, int num, struct p3d_ps_config* cfg) {
	int sz = SIZEOF_P3D_PARTICLE_SYSTEM + num * SIZEOF_P3D_PARTICLE;
	struct p3d_particle_system* ps = (struct p3d_particle_system*)mem;
	memset(ps, 0, sz);
	ps->cfg = cfg;
	_ps_init(ps, num);
	return ps;
}

// static inline void
// _resume(struct particle_system_3d* ps) {
// 	ps->active = true;
// }

static inline void
_pause(struct p3d_particle_system* ps) {
	ps->active = false;
}

static inline void
_stop(struct p3d_particle_system* ps) {
	_pause(ps);
	ps->emit_counter = 0;
}

static inline bool 
_is_full(struct p3d_particle_system* ps) {
	return ps->last == ps->end;	
}

static inline bool
_is_empty(struct p3d_particle_system* ps) {
	return ps->start == ps->last;
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
_trans_coords2d(float r, float hori, struct ps_vec3* pos) {
	pos->x = r * cosf(hori);
	pos->y = r * sinf(hori);
	pos->z = 0;
}

static inline void
_add(struct p3d_particle_system* ps) {
	if (_is_full(ps) || !ps->cfg->symbol_count) {
		return;
	}

	struct p3d_particle* p = ps->last;

	uint32_t RANDSEED = rand();

	p->cfg.symbol = (struct p3d_symbol*)(ps->cfg->symbols + RANDSEED % ps->cfg->symbol_count);

	p->life = ps->cfg->life + ps->cfg->life_var * ps_random_m11(&RANDSEED);
	p->cfg.lifetime = p->life;
	
	p->cfg.dir.x = ps->cfg->hori + ps->cfg->hori_var * ps_random_m11(&RANDSEED);
	p->cfg.dir.y = ps->cfg->vert + ps->cfg->vert_var * ps_random_m11(&RANDSEED);

	if (ps->cfg->is_start_radius_3d) {
		_trans_coords3d(ps->cfg->start_radius, p->cfg.dir.x, p->cfg.dir.y, &p->pos);
	} else {
		_trans_coords2d(ps->cfg->start_radius, p->cfg.dir.x, &p->pos);
	}

	float spd = ps->cfg->spd + ps->cfg->spd_var * ps_random_m11(&RANDSEED);
	_trans_coords3d(spd, p->cfg.dir.x, p->cfg.dir.y, &p->spd);
	memcpy(&p->cfg.spd_dir, &p->spd, sizeof(p->spd));

	p->cfg.dis_region = ps->cfg->dis_region + ps->cfg->dis_region_var * ps_random_m11(&RANDSEED);
	p->dis_curr_len = 0;
	float dis_angle = PI * ps_random_m11(&RANDSEED);
	p->dis_dir.x = cosf(dis_angle);
	p->dis_dir.y = sinf(dis_angle);
	p->cfg.dis_spd = ps->cfg->dis_spd + ps->cfg->dis_spd_var * ps_random_m11(&RANDSEED);

	p->cfg.linear_acc = ps->cfg->linear_acc + ps->cfg->linear_acc_var * ps_random_m11(&RANDSEED);

	p->cfg.angular_spd = ps->cfg->angular_spd + ps->cfg->angular_spd_var * ps_random_m11(&RANDSEED);

	p->angle = p->cfg.symbol->angle + p->cfg.symbol->angle_var * ps_random_m11(&RANDSEED);

	if (p->cfg.symbol->bind_ps_cfg) {
		int num = ps->end - ps->start;
		p->bind_ps = p3d_create(num, p->cfg.symbol->bind_ps_cfg);
	} else if (p->bind_ps) {
		free(p->bind_ps);
		p->bind_ps = NULL;
	}

	if (ps->add_func) {
		ps->add_func(p, ps->ud);
	}

	ps->last++;
}

static inline void
_remove(struct p3d_particle_system* ps, struct p3d_particle* p) {
	if (ps->remove_func) {
		ps->remove_func(p, ps->ud);
	}
	if (!_is_empty(ps)) {
		*p = *(--ps->last);
	}
}

static inline void
_update_speed(struct p3d_particle_system* ps, float dt, struct p3d_particle* p) {
	// gravity
	p->spd.z -= ps->cfg->gravity * dt;

	// normal acceleration
	float velocity = ps_vec3_len(&p->spd);
	float linear_acc = p->cfg.linear_acc * dt;
	for (int i = 0; i < 3; ++i) {
		p->spd.xyz[i] += linear_acc * p->spd.xyz[i] / velocity;
	}

	// disturbance
	if (p->spd.z != 0 && p->pos.z > 0) {
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
}

static inline void
_update_angle(struct p3d_particle_system* ps, float dt, struct p3d_particle* p) {
	if (ps->cfg->orient_to_movement) {
		struct ps_vec2 pos_old, pos_new;
		ps_vec3_projection(&p->pos, &pos_old);
		
		struct ps_vec3 pos;
		for (int i = 0; i < 3; ++i) {
			pos.xyz[i] = p->pos.xyz[i] + p->spd.xyz[i] * dt;
		}
		ps_vec3_projection(&pos, &pos_new);

		p->angle = atan2f(pos_new.y - pos_old.y, pos_new.x - pos_old.x) - PI * 0.5f;
	} else {
		if (p->pos.z > 0.1f) {
			p->angle += p->cfg.angular_spd * dt;
		}
	}
}

static inline void
_update_with_ground(struct p3d_particle_system* ps, struct p3d_particle* p) {
	if (p->pos.z >= 0) {
		return;
	}

	switch (ps->cfg->ground) {
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
_update_position(struct p3d_particle_system* ps, float dt, struct p3d_particle* p) {
	for (int i = 0; i < 3; ++i) {
		p->pos.xyz[i] += p->spd.xyz[i] * dt;
	}
	_update_with_ground(ps, p);
}

void 
p3d_update(struct p3d_particle_system* ps, float dt) {
	if (ps->active && (ps->cfg->loop || ps->particle_count < ps->cfg->count)) {
		float rate = ps->cfg->emission_time / ps->cfg->count;
		ps->emit_counter += dt;
		while (ps->emit_counter > rate) {
			++ps->particle_count;
			_add(ps);
			ps->emit_counter -= rate;
		}
	}

	struct p3d_particle* p = ps->start;
	while (p != ps->last) {
		if (p->bind_ps) {
			p3d_update(p->bind_ps, dt);
		}

		p->life -= dt;

		if (p->life > 0) {
			_update_speed(ps, dt, p);
			_update_angle(ps, dt, p);
			_update_position(ps, dt, p);
			++p;
		} else {
			_remove(ps, p);
			if (p >= ps->last) {
				return;
			}
		}
	}
}