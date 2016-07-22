#include "ps_3d_buffer.h"
#include "ps_3d.h"
#include "ps_3d_sprite.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct list {
	struct p3d_sprite *head, *tail;
};

static struct list L;

static void (*UPDATE_SRT_FUNC)(void* params, float x, float y, float scale);
static void (*REMOVE_FUNC)(struct p3d_sprite*);

void 
p3d_buffer_init(void (*update_srt_func)(void* params, float x, float y, float scale),
				void (*remove_func)(struct p3d_sprite*)) {
	UPDATE_SRT_FUNC = update_srt_func;
	REMOVE_FUNC = remove_func;

	L.head = L.tail = NULL;
}

void 
p3d_buffer_insert(struct p3d_sprite* spr) {
	if (!spr->draw_params) {
		p3d_sprite_create_draw_params(spr);
	}
	spr->next = NULL;
	if (!L.head) {
		assert(!L.tail);
		L.head = L.tail = spr;
	} else {
		L.tail->next = spr;
		L.tail = spr;
	}
}

static inline void
_remove(struct p3d_sprite* curr, struct p3d_sprite* prev) {
	if (curr == L.head) {
		L.head = curr->next;
	}
	if (curr == L.tail) {
		L.tail = prev;
	}
	if (prev) {
		prev->next = curr->next;
	}

	REMOVE_FUNC(curr);
}

void 
p3d_buffer_remove(struct p3d_sprite* spr) {
	struct p3d_sprite* prev = NULL;
	struct p3d_sprite* curr = L.head;
	while (curr && curr != spr) {
		prev = curr;
		curr = curr->next;
	}

	if (curr == spr) {
		_remove(curr, prev);
	}
}

void 
p3d_buffer_clear() {
	struct p3d_sprite* curr = L.head;
	while (curr) {
		struct p3d_sprite* next = curr->next;
		p3d_sprite_release(curr);
		REMOVE_FUNC(curr);
		curr = next;
	}
	L.head = L.tail = NULL;
}

bool 
p3d_buffer_update(float time) {
	bool dirty = false;
	struct p3d_sprite* curr = L.head;
	struct p3d_sprite* prev = NULL;
	while (curr) {
		struct p3d_sprite* next = curr->next;
		struct p3d_emitter* et = curr->et;
		if (!et || p3d_emitter_is_finished(et)) {
			_remove(curr, prev);
			p3d_sprite_release(curr);
		} else {
			if (et->time == 0) {
				et->time = time;
			} else {
				assert(et->time <= time);
				if (et->time < time) {
					dirty = true;
					float dt = time - et->time;
					p3d_emitter_update(et, dt, curr->mat);
					et->time = time;
				}
			}
			prev = curr;
		}
		curr = next;
	}

	return dirty;
}

void 
p3d_buffer_draw(float x, float y, float scale) {
	struct p3d_sprite* curr = L.head;
	while (curr) {
		if (curr->draw_params) {
			UPDATE_SRT_FUNC(curr->draw_params, x, y, scale);
			p3d_emitter_draw(curr->et, curr->draw_params);
		}
		curr = curr->next;
	}
}

bool 
p3d_buffer_query(struct p3d_sprite* spr) {
	struct p3d_sprite* curr = L.head;
	while (curr) {
		if (curr == spr) {
			return true;
		}
		curr = curr->next;
	}
	return false;
}