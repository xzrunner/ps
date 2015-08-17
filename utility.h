#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_utility_h
#define particle_utility_h

#include <stdint.h>
#include <math.h>

#define PI 3.1415926

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

float ps_vec3_len(struct ps_vec3* vec) {
	return sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
}

void ps_vec3_normalize(struct ps_vec3* vec) {
	float len = ps_vec3_len(vec);
	vec->x /= len;
	vec->y /= len;
	vec->z /= len;
}

void ps_vec3_projection(const struct ps_vec3* pos3, struct ps_vec2* pos2) {
	float gx = pos3->x * 0.01f,
		gy = pos3->y * 0.01f;

	pos2->x = (gx - gy) * 36;
	pos2->y = (gx + gy) * 26 + pos3->z * 0.5f;
}

float RANDOM_M11(unsigned int* seed) {
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