#include "ps_3d_buffer.h"
#include "ps_3d.h"

#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SZ 512
#define REMOVED_EMITTER_TIME -1

struct buffer {
	struct p3d_emitter* data[MAX_BUFFER_SZ];
	int sz;
};

static struct buffer* BUF;

void 
p3d_buffer_init() {
	BUF = (struct buffer*)malloc(sizeof(*BUF));
	if (!BUF) {
		return;
	}
	memset(BUF, 0, sizeof(*BUF));
}

void 
p3d_buffer_add(struct p3d_emitter* et) {
	if (BUF->sz >= MAX_BUFFER_SZ) {
		return;
	} else {
		BUF->data[BUF->sz++] = et;
	}
}

void 
p3d_buffer_remove(struct p3d_emitter* et) {
	et->time = REMOVED_EMITTER_TIME;
}

void 
p3d_buffer_clear() {
	BUF->sz = 0;
}

void 
p3d_buffer_update(float dt) {
	for (int i = 0; i < BUF->sz; ) {
		struct p3d_emitter* et = BUF->data[i];
		if (p3d_emitter_is_finished(et) || et->time == REMOVED_EMITTER_TIME) {
			BUF->data[i] = BUF->data[--BUF->sz];
		} else {
			p3d_emitter_update(et, dt, NULL);
			++i;
		}
	}
}

void 
p3d_buffer_draw() {
	for (int i = 0; i < BUF->sz; ++i) {
		p3d_emitter_draw(BUF->data[i], NULL);
	}
}