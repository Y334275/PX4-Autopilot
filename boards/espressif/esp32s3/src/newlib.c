// simple implementation for esp32s3 rom.newlib
#include "board_config.h"
#include <nuttx/kmalloc.h>
#include "xtensa_attr.h"
#include <nuttx/syslog/syslog.h>
#include "assert.h"
#include "debug.h"

void IRAM_ATTR *__getreent(void)
{
	return NULL;
}

void IRAM_ATTR *_calloc_r(void *, size_t n, size_t s)
{
	return kumm_calloc(n, s);
}
void IRAM_ATTR *_malloc_r(void *, size_t size)
{
	return kumm_malloc(size);
}
void IRAM_ATTR _free_r(void *, void *ptr)
{
	return kumm_free(ptr);
}
void IRAM_ATTR *_realloc_r(void *, void *ptr, size_t size)
{
	return kumm_realloc(ptr, size);
}

void __attribute__((noreturn)) __assert_func(const char *file, int line, const char *func, const char *expr)
{
	syslog_flush();
	_alert("Assert failed in %s, %s:%d (%s)",
	       func, file, line, expr);
	abort();
}

/**
 * @brief ESP32-S3 ROM code contains implementations of some of C library functions.
 * Whenever a function in ROM needs to use a syscall, it calls a pointer to the corresponding syscall
 * implementation defined in the following struct.
 *
 * The table itself, by default, is not allocated in RAM. There are two pointers, `syscall_table_ptr_pro` and
 * `syscall_table_ptr_app`, which can be set to point to the locations of syscall tables of CPU 0 (aka PRO CPU)
 * and CPU 1 (aka APP CPU). Location of these pointers in .bss segment of ROM code is defined in linker script.
 *
 * So, before using any of the C library functions (except for pure functions and memcpy/memset functions),
 * application must allocate syscall table structure for each CPU being used, and populate it with pointers
 * to actual implementations of corresponding syscalls.
 *
 */
struct syscall_stub_table {
	void *(*__getreent)(void);
	void *(*_malloc_r)(void *r, size_t);
	void (*_free_r)(void *r, void *);
	void *(*_realloc_r)(void *r, void *, size_t);
	void *(*_calloc_r)(void *r, size_t, size_t);
	void (*_abort)(void);
	int (*_system_r)(void *r, const char *);
	int (*_rename_r)(void *r, const char *, const char *);
	clock_t (*_times_r)(void *r, void *); // struct tms
	int (*_gettimeofday_r)(void *r, void *, void *); // struct timeval
	void (*_raise_r)(void *r);
	int (*_unlink_r)(void *r, const char *);
	int (*_link_r)(void *r, const char *, const char *);
	int (*_stat_r)(void *r, const char *, struct stat *);
	int (*_fstat_r)(void *r, int, struct stat *);
	void *(*_sbrk_r)(void *r, ptrdiff_t);
	int (*_getpid_r)(void *r);
	int (*_kill_r)(void *r, int, int);
	void (*_exit_r)(void *r, int);
	int (*_close_r)(void *r, int);
	int (*_open_r)(void *r, const char *, int, int);
	int (*_write_r)(void *r, int, const void *, int);
	int (*_lseek_r)(void *r, int, int, int);
	int (*_read_r)(void *r, int, void *, int);
	void (*_retarget_lock_init)(void * *lock);
	void (*_retarget_lock_init_recursive)(void * *lock);
	void (*_retarget_lock_close)(void *lock);
	void (*_retarget_lock_close_recursive)(void *lock);
	void (*_retarget_lock_acquire)(void *lock);
	void (*_retarget_lock_acquire_recursive)(void *lock);
	int (*_retarget_lock_try_acquire)(void *lock);
	int (*_retarget_lock_try_acquire_recursive)(void *lock);
	void (*_retarget_lock_release)(void *lock);
	void (*_retarget_lock_release_recursive)(void *lock);
	int (*_printf_float)(void *data, void *pdata, void *fp, int (*pfunc)(void *, void *, const char *, size_t len),
			     void *ap);
	int (*_scanf_float)(void *rptr, void *pdata, void *fp, void *ap);
	void (*__assert_func)(const char *file, int line, const char *func, const char *failedexpr) __attribute__((__noreturn__));
	void (*__sinit)(void *r);
	void (*_cleanup_r)(void *r);
};

extern struct syscall_stub_table *syscall_table_ptr;

static struct syscall_stub_table s_stub_table = {
	.__getreent = &__getreent,
	._malloc_r = &_malloc_r,
	._free_r = &_free_r,
	._realloc_r = &_realloc_r,
	._calloc_r = &_calloc_r,
	._abort = &abort,
	._system_r = (void *)abort,
	._rename_r = (void *)abort,
	._times_r = (void *)abort,
	._gettimeofday_r = (void *)abort,
	._raise_r = (void *)abort,
	._unlink_r = (void *)abort,
	._link_r = (void *)abort,
	._stat_r = (void *)abort,
	._fstat_r = (void *)abort,
	._sbrk_r = (void *)abort,
	._getpid_r = (void *)abort,
	._kill_r = (void *)abort,
	._exit_r = NULL,    // never called in ROM
	._close_r = (void *)abort,
	._open_r = (void *)abort,
	._write_r = (void *)abort,
	._lseek_r = (void *)abort,
	._read_r = (void *)abort,
	._retarget_lock_init = (void *)abort,
	._retarget_lock_init_recursive = (void *)abort,
	._retarget_lock_close = (void *)abort,
	._retarget_lock_close_recursive = (void *)abort,
	._retarget_lock_acquire = (void *)abort,
	._retarget_lock_acquire_recursive = (void *)abort,
	._retarget_lock_try_acquire = (void *)abort,
	._retarget_lock_try_acquire_recursive = (void *)abort,
	._retarget_lock_release = (void *)abort,
	._retarget_lock_release_recursive = (void *)abort,
#ifdef CONFIG_NEWLIB_NANO_FORMAT
	._printf_float = &_printf_float,
	._scanf_float = &_scanf_float,
#else
	._printf_float = NULL,
	._scanf_float = NULL,
#endif
	/* TODO IDF-2570 : mark that this assert failed in ROM, to avoid confusion between IDF & ROM
	   assertion failures (as function names & source file names will be similar)
	*/
	.__assert_func = &__assert_func,

	/* We don't expect either ROM code or IDF to ever call __sinit, so it's implemented as abort() for now.

	   esp_reent_init() does this job inside IDF.

	   Kept in the syscall table in case we find a need for it later.
	*/
	.__sinit = (void *)abort,
	._cleanup_r = (void *)abort,
};

void esp_newlib_init(void)
{
	syscall_table_ptr = &s_stub_table;
}
