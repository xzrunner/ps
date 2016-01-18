#include "ps_3d_buffer.h"
#include "ps_3d.h"
#include "ps_3d_sprite.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

struct list {
	struct p3d_sprite *head, *tail;
};

static struct list L;

void 
p3d_buffer_init(void (*create_draw_params_func)(),
				void (*release_draw_params_func)()) {
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
	p3d_sprite_release(curr);
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
		curr = next;
	}
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
		} else {
			assert(et->time <= time);
			if (et->time < time) {
				dirty = true;
				float dt = time - et->time;
				p3d_emitter_update(et, dt, curr->mat);
				et->time = time;
			}
		}
		curr = next;
	}
	return dirty;
}

void 
p3d_buffer_draw() {
	struct p3d_sprite* curr = L.head;
	while (curr) {
		if (curr->local_mode_draw) {
			p3d_emitter_draw(curr->et, curr->draw_params);
		} else {
			p3d_emitter_draw(curr->et, NULL);
		}
		curr = curr->next;
	}
}