# Memlist

A simple arena-like memory aggergator but with support for custom destructors.

## API

- memlst *memlst_init() : intialize an empty memolist
- bool memlst_add(memlst*, void*, fn_destructor_t destructor) : adds a row to the allocation list and returns true. if pointer is null, does nothing and returns false.
- void memlst_collect(memlst*) : run an memlist's destrutors and clear its alllocation list
- void memlst_destroy(memlst*) : frees the memlist memory itself. the memlist handle is invalid after calling this function. calls memlst_collect too.

## Tests

### Nothing registered -> nothing runs

### Add the same pointer twice triggers an assertion failure

### Adding memlst itself as a pointer triggers an assertion failure

### Add 1 pointer -> destructor called

### Add 2 pointers -> destructor called

### Add NULL -> returns false
