/// @file
/// @author Raphaël
/// @brief Memory list - Interface
/// @date 13/04/2025

#ifndef MEMLST_H
#define MEMLST_H

#include <stdbool.h>

typedef struct memlst memlst_t;

/// @brief Create a new, empty memory list.
/// @return A new memory list.
memlst_t *memlst_init(void);

void dtor_json_object(void *json_object);

/// @brief Destroy a memory list. Free all allocated memory, including memory for the memory list itself.
/// @param memlst A memory list.
/// @remark @p memlst points to invalid memory after calling this function.
void memlst_destroy(memlst_t **memlst);

/// @brief A destructor function for memory lists.
typedef void (*fn_dtor_t)(void *);

/// @brief Add an allocation to a memory list.
/// @param memlst A memory list.
/// @param dtor The destructor function to add on cleanup.
/// @param ptr The pointer to add.
/// @return @p ptr, or @c NULL or the system is out of memoory.
/// @remark if @p ptr is @c NULL, this is a no-op and other parameters can be @c NULL.
void *memlst_add(memlst_t *restrict *restrict memlst, fn_dtor_t dtor, void *restrict ptr);

/// @brief Empty a memory list's allocations and runs destructors.
/// @param memlst A memory list.
void memlst_collect(memlst_t **memlst);

#endif // MEMLST_H
