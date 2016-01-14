#include "ps_3d_buffer.h"
#include "ps_3d.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_BUFFER_SZ 512
#define REMOVED_EMITTER_TIME -1

static void (*WRAP_RENDER_PARAMS_FUNC)(void* params, float* mat);
static void* RENDER_PARAMS = NULL;

struct buffer {
	struct p3d_sprite sprites[MAX_BUFFER_SZ];
	int sz;
};

static struct buffer* BUF;

void 
p3d_buffer_init(void* (*create_render_params_func)(),
				void (*wrap_render_params_func)(void* params, float* mat)) {
	RENDER_PARAMS = create_render_params_func();
	WRAP_RENDER_PARAMS_FUNC = wrap_render_params_func;

	BUF = (struct buffer*)malloc(sizeof(*BUF));
	if (!BUF) {
		return;
	}
	memset(BUF, 0, sizeof(*BUF));
}

struct p3d_sprite* 
p3d_buffer_add() {
	if (BUF->sz >= MAX_BUFFER_SZ) {
		return NULL;
	} else {
		return &BUF->sprites[BUF->sz++];
	}
}

void 
p3d_buffer_remove(struct p3d_sprite* spr) {
	spr->et->time = REMOVED_EMITTER_TIME;
}

void 
p3d_buffer_clear() {
	memset(BUF, 0, sizeof(*BUF));
}

bool 
p3d_buffer_update(float time) {
	bool dirty = false;
	for (int i = 0; i < BUF->sz; ) {
		struct p3d_sprite* spr = &BUF->sprites[i];
		struct p3d_emitter* et = spr->et;
		if (et->time == REMOVED_EMITTER_TIME || p3d_emitter_is_finished(et)) {
			spr->ud = NULL;
			BUF->sprites[i] = BUF->sprites[--BUF->sz];
		} else {
			assert(et->time <= time);
			if (et->time < time) {
				dirty = true;
				float dt = time - et->time;
				p3d_emitter_update(et, dt, spr->mat);
				et->time = time;
			}
			++i;
		}
	}
	return dirty;
}

void 
p3d_buffer_draw() {
	for (int i = 0; i < BUF->sz; ++i) {
		struct p3d_sprite* spr = &BUF->sprites[i];
		if (spr->local_mode_draw) {
			WRAP_RENDER_PARAMS_FUNC(RENDER_PARAMS, spr->mat);
			p3d_emitter_draw(spr->et, RENDER_PARAMS);
		} else {
			p3d_emitter_draw(spr->et, NULL);
		}
	}
}