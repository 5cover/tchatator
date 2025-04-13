#include "memlst.h"
#include <assert.h>
#include <json-c.h>
#include <stb_ds.h>

struct memlst {
    void *ptr;
    fn_dtor_t dtor;
};

memlst_t *memlst_init() {
    return NULL;
}

void dtor_json_object(void *json_object) {
    json_object_put(json_object);
}

void memlst_destroy(memlst_t **memlst) {
    memlst_collect(memlst);
    arrfree(*memlst);
}

void *memlst_add(memlst_t *restrict *restrict memlst, fn_dtor_t dtor, void *restrict ptr) {
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
