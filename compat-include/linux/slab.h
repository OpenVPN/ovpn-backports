#ifndef _NET_OVPN_LINUX_SLAB_H_
#define _NET_OVPN_LINUX_SLAB_H_

#include <linux/version.h>
#include_next <linux/slab.h>

#ifndef __alloc_objs
#define __alloc_objs(KMALLOC, GFP, TYPE, COUNT)				\
({									\
	const size_t __obj_size = size_mul(sizeof(TYPE), COUNT);	\
	(TYPE *)KMALLOC(__obj_size, GFP);				\
})
#endif /* __alloc_objs */

#ifndef __alloc_flex
#define __alloc_flex(KMALLOC, GFP, TYPE, FAM, COUNT)			\
({									\
	const size_t __count = (COUNT);					\
	const size_t __obj_size = struct_size_t(TYPE, FAM, __count);	\
	TYPE *__obj_ptr = KMALLOC(__obj_size, GFP);			\
	if (__obj_ptr)							\
		__set_flex_counter(__obj_ptr->FAM, __count);		\
	__obj_ptr;							\
})
#endif /* __alloc_flex */

#ifndef kmalloc_obj
#define kmalloc_obj(VAR_OR_TYPE, ...) \
	__alloc_objs(kmalloc, default_gfp(__VA_ARGS__), typeof(VAR_OR_TYPE), 1)
#endif /* kmalloc_obj */

#ifndef kmalloc_objs
#define kmalloc_objs(VAR_OR_TYPE, COUNT, ...) \
	__alloc_objs(kmalloc, default_gfp(__VA_ARGS__), typeof(VAR_OR_TYPE), COUNT)
#endif /* kmalloc_objs */

#ifndef kmalloc_flex
#define kmalloc_flex(VAR_OR_TYPE, FAM, COUNT, ...) \
	__alloc_flex(kmalloc, default_gfp(__VA_ARGS__), typeof(VAR_OR_TYPE), FAM, COUNT)
#endif /* kmalloc_flex */

#ifndef kzalloc_obj
#define kzalloc_obj(P, ...) \
	__alloc_objs(kzalloc, default_gfp(__VA_ARGS__), typeof(P), 1)
#endif /* kzalloc_obj */

#ifndef kzalloc_objs
#define kzalloc_objs(P, COUNT, ...) \
	__alloc_objs(kzalloc, default_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif /* kzalloc_objs */

#ifndef kzalloc_flex
#define kzalloc_flex(P, FAM, COUNT, ...)		\
	__alloc_flex(kzalloc, default_gfp(__VA_ARGS__), typeof(P), FAM, COUNT)
#endif /* kzalloc_flex */

#ifndef kvmalloc_obj
#define kvmalloc_obj(P, ...) \
	__alloc_objs(kvmalloc, default_gfp(__VA_ARGS__), typeof(P), 1)
#endif /* kvmalloc_obj */

#ifndef kvmalloc_objs
#define kvmalloc_objs(P, COUNT, ...) \
	__alloc_objs(kvmalloc, default_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif /* kvmalloc_objs */

#ifndef kvmalloc_flex
#define kvmalloc_flex(P, FAM, COUNT, ...) \
	__alloc_flex(kvmalloc, default_gfp(__VA_ARGS__), typeof(P), FAM, COUNT)
#endif /* kvmalloc_flex */

#ifndef kvzalloc_obj
#define kvzalloc_obj(P, ...) \
	__alloc_objs(kvzalloc, default_gfp(__VA_ARGS__), typeof(P), 1)
#endif /* kvzalloc_obj */

#ifndef kvzalloc_objs
#define kvzalloc_objs(P, COUNT, ...) \
	__alloc_objs(kvzalloc, default_gfp(__VA_ARGS__), typeof(P), COUNT)
#endif /* kvzalloc_objs */

#ifndef kvzalloc_flex
#define kvzalloc_flex(P, FAM, COUNT, ...) \
	__alloc_flex(kvzalloc, default_gfp(__VA_ARGS__), typeof(P), FAM, COUNT)
#endif /* kvzalloc_flex */

#endif /* _NET_OVPN_LINUX_SLAB_H_ */
