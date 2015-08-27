#include "particle3d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct particle_system_3d* 
ps_create(int num, struct ps_cfg_3d* cfg) {
	int sz = sizeof(struct particle_system_3d) + num * (sizeof(struct particle_3d));
	struct particle_system_3d* ps = (struct particle_system_3d*)malloc(sz);
	memset(ps, 0, sz);
	ps->cfg = cfg;
	ps_init(ps, num);
	return ps;
}

// static inline void
// _resume(struct particle_system_3d* ps) {
// 	ps->active = true;
// }

static inline void
_pause(struct particle_system_3d* ps) {
	ps->active = false;
}

static inline void
_stop(struct particle_system_3d* ps) {
	_pause(ps);
	ps->emit_counter = 0;
}

static inline bool 
_is_full(struct particle_system_3d* ps) {
	return ps->last == ps->end;	
}

static inline bool
_is_empty(struct particle_system_3d* ps) {
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
_add(struct particle_system_3d* ps) {
	if (_is_full(ps) || !ps->cfg->symbol_count) {
		return;
	}

	struct particle_3d* p = ps->last;

	uint32_t RANDSEED = rand();

	p->cfg.symbol = (struct particle_symbol*)(ps->cfg->symbols + RANDSEED % ps->cfg->symbol_count);

	p->life = ps->cfg->life + ps->cfg->life_var * RANDOM_M11(&RANDSEED);
	p->cfg.lifetime = p->life;
	
	p->cfg.dir.x = ps->cfg->hori + ps->cfg->hori_var * RANDOM_M11(&RANDSEED);
	p->cfg.dir.y = ps->cfg->vert + ps->cfg->vert_var * RANDOM_M11(&RANDSEED);

	if (ps->cfg->is_start_radius_3d) {
		_trans_coords3d(ps->cfg->start_radius, p->cfg.dir.x, p->cfg.dir.y, &p->pos);
	} else {
		_trans_coords2d(ps->cfg->start_radius, p->cfg.dir.x, &p->pos);
	}

	float spd = ps->cfg->spd + ps->cfg->spd_var * RANDOM_M11(&RANDSEED);
	_trans_coords3d(spd, p->cfg.dir.x, p->cfg.dir.y, &p->spd);
	memcpy(&p->cfg.spd_dir, &p->spd, sizeof(p->spd));

	p->cfg.dis_region = ps->cfg->dis_region + ps->cfg->dis_region_var * RANDOM_M11(&RANDSEED);
	p->dis_curr_len = 0;
	float dis_angle = PI * RANDOM_M11(&RANDSEED);
	p->dis_dir.x = cosf(dis_angle);
	p->dis_dir.y = sinf(dis_angle);
	p->cfg.dis_spd = ps->cfg->dis_spd + ps->cfg->dis_spd_var * RANDOM_M11(&RANDSEED);

	p->cfg.linear_acc = ps->cfg->linear_acc + ps->cfg->linear_acc_var * RANDOM_M11(&RANDSEED);

	p->cfg.angular_spd = ps->cfg->angular_spd + ps->cfg->angular_spd_var * RANDOM_M11(&RANDSEED);

	p->angle = p->cfg.symbol->angle + p->cfg.symbol->angle_var * RANDOM_M11(&RANDSEED);

	if (p->cfg.symbol->bind_ps_cfg) {
		int num = ps->end - ps->start;
		p->bind_ps = ps_create(num, p->cfg.symbol->bind_ps_cfg);
	} else if (p->bind_ps) {
		free(p->bind_ps);
		p->bind_ps = NULL;
	}

	if (ps->add_func) {
		ps->add_func(p);
	}

	ps->last++;
}

static inline void
_remove(struct particle_system_3d* ps, struct particle_3d* p) {
	if (ps->remove_func) {
		ps->remove_func(p);
	}
	if (!_is_empty(ps)) {
		*p = *(--ps->last);
	}
}

void 
ps_init(struct particle_system_3d* ps, int num) {
	ps->last = ps->start = (struct particle_3d*)(ps + 1);
	ps->end = ps->last + num;

	ps->emit_counter = 0;
	ps->active = ps->loop = false;
}

static inline void
_update_speed(struct particle_system_3d* ps, float dt, struct particle_3d* p) {
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
_update_angle(struct particle_system_3d* ps, float dt, struct particle_3d* p) {
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
_update_position(struct particle_system_3d* ps, float dt, struct particle_3d* p) {
	for (int i = 0; i < 3; ++i) {
		p->pos.xyz[i] += p->spd.xyz[i] * dt;
	}
	if (p->pos.z < 0) {
		if (ps->cfg->bounce) {
			p->pos.z *= -0.2f;
			p->spd.x *= 0.4f;
			p->spd.y *= 0.4f;
			p->spd.z *= -0.2f;
		} else {
			p->pos.z = 0;
			memset(p->spd.xyz, 0, sizeof(p->spd));
		}
	}
}

void 
ps_update(struct particle_system_3d* ps, float dt) {
	if (ps->active) {
		if (ps->loop) {
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

// 			ps->life -= dt;
// 			if (ps->life < 0) {
// 				_stop(ps);
// 			}
		}
	}

	struct particle_3d* p = ps->start;
	while (p != ps->last) {
		if (p->bind_ps) {
			ps_update(p->bind_ps, dt);
		}

		p->life -= dt;

		if (p->life > 0) {
			_update_speed(ps, dt, p);
			_update_angle(ps, dt, p);
			_update_position(ps, dt, p);
			p++;
		} else {
			_remove(ps, p);
			if (p >= ps->last) {
				return;
			}
		}
	}
}