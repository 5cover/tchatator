/// @file
/// @author Raphaël
/// @brief Tchatator413 test - memlst
/// @date 1/02/2025

#include "memlst.h"
#include "tests.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef NDEBUG
#error the memlst test cannot be run with NDEBUG since it requires assertions to run.
#endif

static int gs_ncalls_testfree = 0;
static void *gs_last_freed_ptr = NULL;

static inline void testfree(void *ptr) {
    ++gs_ncalls_testfree;
    free(gs_last_freed_ptr = ptr);
}

#define test_expect_sigabrt(t, code)                            \
    do {                                                        \
        pid_t pid = fork();                                     \
        if (pid == -1) {                                        \
            perror("fork failed");                              \
            exit(1);                                            \
        }                                                       \
        if (pid == 0) {                                         \
            /* Child: silence assert output*/                   \
            int devnull = open("/dev/null", O_WRONLY);          \
            if (devnull != -1) {                                \
                dup2(devnull, STDERR_FILENO);                   \
                close(devnull);                                 \
            }                                                   \
            /* Run the code that might assert*/                 \
            code; /*NOLINT(bugprone-macro-parentheses)*/        \
            /* If it didn't abort, exit normally */             \
            _exit(0);                                           \
        }                                                       \
        /* Parent: wait for the child and check result */       \
        int status;                                             \
        waitpid(pid, &status, 0);                               \
        test_case(&(t),                                         \
            WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, \
            "status == %d", status);                            \
    } while (0)

struct test test_memlst(void) {
    struct test p_test = test_start("memlst");

    memlst_t *memlst = memlst_init();

    // test twice in case NULL is checked against duplicates -- which it shouldn't be.
    test_case(&p_test, memlst_add(&memlst, NULL, NULL) == NULL, "adding NULL returns false");
    test_case(&p_test, memlst_add(&memlst, NULL, NULL) == NULL, "adding NULL returns false");

    // Sample allocation
    void *ptr = malloc(19);
    assert(ptr);

    // First add
    test_case(&p_test, memlst_add(&memlst, testfree, ptr) == ptr, "first add");

    // Duplicate pointer: asserts
    test_expect_sigabrt(p_test, memlst_add(&memlst, testfree, ptr));

    // Collect of 1
    memlst_collect(&memlst);
    TEST_CASE_EQ_INT(&p_test, gs_ncalls_testfree, 1, );
    test_case(&p_test, gs_last_freed_ptr == ptr, "");

    // Collect of 0. Previous collect shoud have emptied the array. Same state.
    memlst_collect(&memlst);
    TEST_CASE_EQ_INT(&p_test, gs_ncalls_testfree, 1, );
    test_case(&p_test, gs_last_freed_ptr == ptr, "");

    ptr = malloc(100);
    void *ptr1 = malloc(50);

    test_case(&p_test, memlst_add(&memlst, testfree, ptr) == ptr, "");
    test_case(&p_test, memlst_add(&memlst, testfree, ptr1) == ptr1, "");

    // destruction: should collect of 2 too. order of cleanup is not specified.
    memlst_destroy(&memlst);
    TEST_CASE_EQ_INT(&p_test, gs_ncalls_testfree, 3, );
    test_case(&p_test, gs_last_freed_ptr == ptr || gs_last_freed_ptr == ptr1, "");

    return p_test;
}
