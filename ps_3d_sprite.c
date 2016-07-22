#include "ps_3d_sprite.h"
#include "ps_3d.h"
#include "ps_array.h"

#include <logger.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_SPRITE_SZ	1000

//#define SPR_LOG

static struct p3d_sprite* SPRITE_ARRAY = NULL;

static void (*CREATE_DRAW_PARAMS_FUNC)(struct p3d_sprite*);
static void (*RELEASE_DRAW_PARAMS_FUNC)(struct p3d_sprite*);

static struct p3d_sprite* CREATED_BUF[MAX_SPRITE_SZ];
static int CREATED_COUNT = 0;

void 
p3d_sprite_init(void (*create_draw_params_func)(struct p3d_sprite* spr),
				void (*release_draw_params_func)(struct p3d_sprite* spr)) {
	CREATE_DRAW_PARAMS_FUNC = create_draw_params_func;
	RELEASE_DRAW_PARAMS_FUNC = release_draw_params_func;

	int sz = sizeof(struct p3d_sprite) * MAX_SPRITE_SZ;
	SPRITE_ARRAY = (struct p3d_sprite*)malloc(sz);
	if (!SPRITE_ARRAY) {
		LOGW("malloc err: p3d_sprite_init");
		return;
	}
	memset(SPRITE_ARRAY, 0, sz);
	PS_ARRAY_INIT(SPRITE_ARRAY, MAX_SPRITE_SZ);
}

#ifdef SPR_LOG
static int count = 0;
#endif // SPR_LOG

struct p3d_sprite* 
p3d_sprite_create() {
	struct p3d_sprite* spr;
	PS_ARRAY_ALLOC(SPRITE_ARRAY, spr);
	if (!spr) {
		return NULL;
	}
#ifdef SPR_LOG
	++count;
	LOGD("add %d %p\n", count, spr);
#endif // EMITTER_LOG
	memset(spr, 0, sizeof(struct p3d_sprite));
	if (CREATED_COUNT < MAX_SPRITE_SZ) {
		CREATED_BUF[CREATED_COUNT++] = spr;
	}
	return spr;
}

void 
p3d_sprite_release(struct p3d_sprite* spr) {
	assert(spr->et);
	p3d_emitter_release(spr->et);
	spr->et = NULL;

	if (spr->draw_params) {
		RELEASE_DRAW_PARAMS_FUNC(spr);
	}
#ifdef SPR_LOG
	--count;
	LOGD("del %d %p\n", count, spr);
#endif // EMITTER_LOG

	*(spr->ptr_self) = NULL;
	PS_ARRAY_FREE(SPRITE_ARRAY, spr);
}

void 
p3d_sprite_clear() {
	for (int i = 0; i < CREATED_COUNT; ++i) {
		struct p3d_sprite* spr = CREATED_BUF[i];
		if (spr->et) {
			p3d_sprite_release(spr);
		}
	}
	CREATED_COUNT = 0;
}

void 
p3d_sprite_create_draw_params(struct p3d_sprite* spr) {
	assert(!spr->draw_params);
	CREATE_DRAW_PARAMS_FUNC(spr);
}