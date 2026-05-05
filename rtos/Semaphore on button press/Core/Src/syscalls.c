/* syscalls.c — minimal stubs to silence newlib-nano linker warnings
 * TODO: These are required when using --specs=nano.specs without a full OS.
 * _write will be properly implemented.
 */
#include <sys/stat.h>
#include <errno.h>

int _write(int fd, char *ptr, int len)
{
    (void)fd; (void)ptr;
    return len;   /* silence the warning */
}

int _read(int fd, char *ptr, int len)
{
    (void)fd; (void)ptr; (void)len;
    return 0;
}

int _close(int fd)
{
    (void)fd;
    return -1;
}

int _lseek(int fd, int ptr, int dir)
{
    (void)fd; (void)ptr; (void)dir;
    return 0;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	errno = EINVAL;
	return -1;
}
