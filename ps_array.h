#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_system_array_h
#define particle_system_array_h

/*
#define PS_ARRAY_INIT(buffer, type, n) { \
	type* array = (type*)(buffer); \
	for (int i = 0; i < (n) - 1; ++i) { \
		array[i].next = &array[i + 1]; \
	} \
	array[(n) - 1].next = NULL; \
}
*/

#define PS_ARRAY_INIT(array, n) { \
	for (int i = 0; i < (n) - 1; ++i) { \
		array[i].next = &array[i + 1]; \
	} \
	array[(n) - 1].next = NULL; \
}

#define PS_ARRAY_INIT_WITH_INDEX(array, n) { \
	for (int i = 0; i < (n) - 1; ++i) { \
		array[i].next = &array[i + 1]; \
        array[i].index = i; \
	} \
	array[(n) - 1].next = NULL; \
}

#define PS_ARRAY_ALLOC(array, node) { \
	(node) = (array); \
	if (node) { \
		(array) = (node)->next; \
	} \
}

#define PS_ARRAY_FREE(array, node) { \
	(node)->next = (array); \
	(array) = (node); \
}

#endif // particle_system_array_h

#ifdef __cplusplus
}
#endif