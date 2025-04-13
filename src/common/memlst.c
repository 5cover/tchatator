#include "memlst.h"
#include <assert.h>
#include <stb_ds.h>

struct memlst {
    void *ptr;
    fn_destructor_t dtor;
};

memlst_t *memlst_init() {
    return NULL;
}

void memlst_destroy(memlst_t **memlst) {
    memlst_collect(memlst);
    arrfree(*memlst);
}

void *memlst_add(memlst_t *restrict *restrict memlst, void *restrict ptr, fn_destructor_t dtor) {
    if (!ptr) return NULL;

#ifndef NDEBUG
        for (ptrdiff_t i = 0; i < arrlen(*memlst); ++i) {
            assert((*memlst)[i].ptr != ptr);
        }
#endif // NDEBUG

    arrput(*memlst, ((memlst_t) {
                        .ptr = ptr,
                        .dtor = dtor,
                    }));
    return ptr;
}

void memlst_collect(memlst_t **memlst) {
    for (ptrdiff_t i = 0; i < arrlen(*memlst); ++i) {
        (*memlst)[i].dtor((*memlst)[i].ptr);
    }
    arrsetlen(*memlst, 0);
}
