#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_utility_h
#define particle_system_utility_h

#include <stdint.h>
#include <math.h>

#define PTR_SIZE_DIFF (8 - sizeof(void *))
#define SIZEOF_POINTER 8

struct ps_vec2 {
	union {
		struct {
			float x, y;
		};

		float xy[2];
	};
};

struct ps_vec3 {
	union {
		struct {
			float x, y, z;
		};

		float xyz[3];
	};
};

struct ps_color
{
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;			
		};

		uint8_t rgba[4];
	};
};

static inline void 
ps_vec2_normalize(struct ps_vec2* p) {
	float len2 = p->x * p->x + p->y * p->y;
	if (len2 == 0) {
		p->x = 1;
		p->y = 0;
	} else {
		float len = sqrtf(len2);
		p->x /= len;
		p->y /= len;
	}
}

static inline float 
ps_vec3_len(struct ps_vec3* p) {
	return sqrtf(p->x * p->x + p->y * p->y + p->z * p->z);
}

static inline void 
ps_vec3_normalize(struct ps_vec3* p) {
	float len2 = p->x * p->x + p->y * p->y + p->z * p->z;
	if (len2 == 0) {
		p->x = 1;
		p->y = p->z = 0;
	} else {
		float len = sqrtf(len2);
		p->x /= len;
		p->y /= len;
		p->z /= len;
	}
}

static inline void 
ps_vec3_projection(const struct ps_vec3* pos3, struct ps_vec2* pos2) {
	float gx = pos3->x * 0.01f,
		gy = pos3->y * 0.01f;

	pos2->x = (gx - gy) * 36;
	pos2->y = (gx + gy) * 26 + pos3->z * 0.5f;
}

static inline void
ps_color_sub(const struct ps_color* start, const struct ps_color* end, 
			 struct ps_color* ret) {
	ret->r = end->r - start->r;
	ret->g = end->g - start->g;
	ret->b = end->b - start->b;
	ret->a = end->a - start->a;
}

static inline void
ps_color_mul(struct ps_color* ori, float mul) {
	ori->r = (int)(ori->r * mul + 0.5f);
	ori->g = (int)(ori->g * mul + 0.5f);
	ori->b = (int)(ori->b * mul + 0.5f);
	ori->a = (int)(ori->a * mul + 0.5f);
}

static inline float 
ps_random_m11(unsigned int* seed) {
	*seed = *seed * 134775813 + 1;
	union {
		uint32_t d;
		float f;
	} u;
	u.d = (((uint32_t)(*seed) & 0x7fff) << 8) | 0x40000000;
	return u.f - 3.0f;
}

#endif // particle_system_utility_h

#ifdef __cplusplus
}
#endif