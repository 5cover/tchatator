#define MEMLST_IMPL

#include "memlst.h"
#include <assert.h>
#include "json-c.h"
#include "stb_ds.h"
#include <stdio.h>


struct memlst {
    void *ptr;
    dtor_fn dtor;
};

memlst_t *memlst_init() {
    return NULL;
}

void dtor_json_object(void *json_object) {
    json_object_put(json_object);
}

void memlst_destroy(memlst_t **memlst FILE_LINE_PARAMS) {
#ifdef MEMLST_TRACE
    fprintf(stderr, "%s:%d memlst_destroy(%p)\n", file, line, memlst);
#endif
    memlst_collect(memlst FILE_LINE_ARGS);
    arrfree(*memlst);
}

void *memlst_add(memlst_t *restrict *restrict memlst, dtor_fn dtor, void *restrict ptr FILE_LINE_PARAMS) {
#ifdef MEMLST_TRACE
    fprintf(stderr, "%s:%d memlst_add(%p, dtor=%p, ptr=%p)\n", file, line, memlst, dtor, ptr);
#endif
    if (!ptr) return NULL;

#ifndef NDEBUG
    for (ptrdiff_t i = 0; i < arrlen(*memlst); ++i) {
        assert((*memlst)[i].ptr != ptr);
    }
#endif

    arrput(*memlst, ((memlst_t) {
                        .ptr = ptr,
                        .dtor = dtor,
                    }));
    return ptr;
}

void memlst_collect(memlst_t **memlst FILE_LINE_PARAMS) {
#ifdef MEMLST_TRACE
    fprintf(stderr, "%s:%d memlst_collect(%p)\n", file, line, memlst);
#endif
    for (ptrdiff_t i = 0; i < arrlen(*memlst); ++i) {
#ifdef MEMLST_TRACE
        fprintf(stderr, "  clean %p\n", (*memlst)[i].ptr);
#endif
        (*memlst)[i].dtor((*memlst)[i].ptr);
    }
    arrsetlen(*memlst, 0);
}
