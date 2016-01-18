#include "ps_3d_sprite.h"
#include "ps_array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_SPRITE_SZ	1000

static struct p3d_sprite* SPRITE_ARRAY = NULL;

static void* (*CREATE_DRAW_PARAMS_FUNC)();
static void (*RELEASE_DRAW_PARAMS_FUNC)(void* params);

void 
p3d_sprite_init(void* (*create_draw_params_func)(),
				void (*release_draw_params_func)(void* params)) {
	CREATE_DRAW_PARAMS_FUNC = create_draw_params_func;
	RELEASE_DRAW_PARAMS_FUNC = release_draw_params_func;

	int sz = sizeof(struct p3d_sprite) * MAX_SPRITE_SZ;
	SPRITE_ARRAY = (struct p3d_sprite*)malloc(sz);
	if (!SPRITE_ARRAY) {
		printf("malloc err: p3d_init !\n");
		return;
	}
	memset(SPRITE_ARRAY, 0, sz);
	PS_ARRAY_INIT(SPRITE_ARRAY, MAX_SPRITE_SZ);
}

//static int count = 0;

struct p3d_sprite* 
p3d_sprite_create() {
	struct p3d_sprite* spr;
	PS_ARRAY_ALLOC(SPRITE_ARRAY, spr);
	if (!spr) {
		return NULL;
	}
	//++count;
	//printf("add %d %p\n", count, spr);
	memset(spr, 0, sizeof(struct p3d_sprite));
	return spr;
}

void 
p3d_sprite_release(struct p3d_sprite* spr) {
	if (spr->draw_params) {
		RELEASE_DRAW_PARAMS_FUNC(spr->draw_params);
		spr->draw_params = NULL;
	}
	//--count;
	//printf("del %d %p\n", count, spr);

	*(spr->ptr_self) = NULL;
	PS_ARRAY_FREE(SPRITE_ARRAY, spr);
}

void 
p3d_sprite_create_draw_params(struct p3d_sprite* spr) {
	assert(!spr->draw_params);
	spr->draw_params = CREATE_DRAW_PARAMS_FUNC();
}