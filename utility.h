#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_utility_h
#define particle_utility_h

#include <stdint.h>
#include <math.h>

#define PI 3.1415926

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

struct ps_color4f {
	float r;
	float g;
	float b;
	float a;
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
ps_color_sub(const struct ps_color4f* start, const struct ps_color4f* end, 
			 struct ps_color4f* ret) {
	ret->r = end->r - start->r;
	ret->g = end->g - start->g;
	ret->b = end->b - start->b;
	ret->a = end->a - start->a;
}

static inline void
ps_color_mul(struct ps_color4f* ori, float mul) {
	ori->r *= mul;
	ori->g *= mul;
	ori->b *= mul;
	ori->a *= mul;
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

#endif // particle_utility_h

#ifdef __cplusplus
}
#endif