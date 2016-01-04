#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_3d_buffer_h
#define particle_system_3d_buffer_h

void p3d_buffer_init();

void p3d_buffer_add(struct p3d_emitter*);
void p3d_buffer_remove(struct p3d_emitter*);
void p3d_buffer_clear();

void p3d_buffer_update(float dt);
void p3d_buffer_draw();

#endif // particle_system_3d_buffer_h

#ifdef __cplusplus
}
#endif