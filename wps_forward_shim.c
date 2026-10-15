/*
 * Conservative workaround for duplicate windows in integrated (prome_fushion)
 * mode, preloaded by wps_runner.
 *
 * Every `flatpak run` gets its own PID namespace. A newly started
 * `wpsoffice /prometheus` correctly forwards its request to the running
 * instance through the startup socket in
 * ~/.local/share/Kingsoft/daemon/wpsoffice_starup_object/, but later checks
 * the running instance's PID from a lock file in /tmp, cannot see it, takes
 * over as a second main instance and opens another window.
 *
 * This library ends such a process right after WPS itself has completed the
 * exchange on a startup socket, before it gets to that check. It knows nothing
 * about the message format: WPS builds and sends the request. It only acts
 * when the exchange looks exactly like the observed one (two sends and two
 * receives, then close). In every other case it does nothing, so WPS behaves
 * as it would without it.
 *
 * Safeguards:
 *  - hooks are active only in `.../office6/wpsoffice /prometheus`;
 *  - WPS processes remove this library from LD_PRELOAD so their children
 *    don't inherit it;
 *  - hooks switch off for good once the startup connections are done, or
 *    ACTIVE_SECONDS after start, so a running main instance is never touched.
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define WPS_DIR "/app/extra/opt/kingsoft/wps-office/office6/"
#define SOCKET_DIR "/.local/share/Kingsoft/daemon/wpsoffice_starup_object/"
#define SHIM_NAME "wps_forward_shim.so"
#define ACTIVE_SECONDS 10
#define MAX_FD 1024

static int active;
static time_t deadline;
static int tracked_count;
static struct {
    unsigned char tracked, sends, recvs;
} fds[MAX_FD];

static int (*real_connect)(int, const struct sockaddr *, socklen_t);
static ssize_t (*real_send)(int, const void *, size_t, int);
static ssize_t (*real_sendto)(int, const void *, size_t, int,
                              const struct sockaddr *, socklen_t);
static ssize_t (*real_recv)(int, void *, size_t, int);
static ssize_t (*real_recvfrom)(int, void *, size_t, int, struct sockaddr *,
                                socklen_t *);
static int (*real_close)(int);

static time_t now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec;
}

/* Drop this library from LD_PRELOAD, keeping any other entries. */
static void remove_from_preload(void)
{
    const char *value = getenv("LD_PRELOAD");
    if (!value)
        return;
    char *copy = strdup(value);
    char *out = calloc(1, strlen(value) + 1);
    if (!copy || !out)
        goto done;
    for (char *save, *tok = strtok_r(copy, ": ", &save); tok;
         tok = strtok_r(NULL, ": ", &save)) {
        const char *base = strrchr(tok, '/');
        if (strcmp(base ? base + 1 : tok, SHIM_NAME) == 0)
            continue;
        if (*out)
            strcat(out, ":");
        strcat(out, tok);
    }
    if (*out)
        setenv("LD_PRELOAD", out, 1);
    else
        unsetenv("LD_PRELOAD");
done:
    free(copy);
    free(out);
}

static int is_active(void)
{
    if (active && now() > deadline)
        active = 0;
    return active;
}

static int is_tracked(int fd)
{
    return fd >= 0 && fd < MAX_FD && fds[fd].tracked && is_active();
}

static int is_startup_socket(const struct sockaddr *addr, socklen_t len)
{
    size_t off = offsetof(struct sockaddr_un, sun_path);
    if (!addr || addr->sa_family != AF_UNIX || len <= off)
        return 0;
    return memmem((const char *)addr + off, len - off, SOCKET_DIR,
                  strlen(SOCKET_DIR)) != NULL;
}

__attribute__((constructor))
static void init(int argc, char **argv)
{
    char exe[4096];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len <= 0)
        return;
    exe[len] = '\0';
    if (strncmp(exe, WPS_DIR, strlen(WPS_DIR)) != 0)
        return;

    remove_from_preload();

    if (strcmp(exe, WPS_DIR "wpsoffice") != 0 || argc < 2 ||
        strcmp(argv[1], "/prometheus") != 0)
        return;

    real_connect = dlsym(RTLD_NEXT, "connect");
    real_send = dlsym(RTLD_NEXT, "send");
    real_sendto = dlsym(RTLD_NEXT, "sendto");
    real_recv = dlsym(RTLD_NEXT, "recv");
    real_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
    real_close = dlsym(RTLD_NEXT, "close");
    if (!real_connect || !real_send || !real_sendto || !real_recv ||
        !real_recvfrom || !real_close)
        return;

    deadline = now() + ACTIVE_SECONDS;
    active = 1;
}

/* Processes where init() bailed out resolve lazily and only pass through */
#define RESOLVE(name)                                    \
    do {                                                 \
        if (!real_##name)                                \
            real_##name = dlsym(RTLD_NEXT, #name);       \
    } while (0)

int connect(int fd, const struct sockaddr *addr, socklen_t len)
{
    RESOLVE(connect);
    int ret = real_connect(fd, addr, len);
    if (ret == 0 && fd >= 0 && fd < MAX_FD && is_active() &&
        is_startup_socket(addr, len)) {
        if (!fds[fd].tracked)
            tracked_count++;
        fds[fd].tracked = 1;
        fds[fd].sends = fds[fd].recvs = 0;
    }
    return ret;
}

static void count(int fd, ssize_t ret, int is_send)
{
    if (ret <= 0 || !is_tracked(fd))
        return;
    unsigned char *n = is_send ? &fds[fd].sends : &fds[fd].recvs;
    if (*n < 255)
        (*n)++;
}

ssize_t send(int fd, const void *buf, size_t len, int flags)
{
    RESOLVE(send);
    ssize_t ret = real_send(fd, buf, len, flags);
    count(fd, ret, 1);
    return ret;
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *addr, socklen_t addrlen)
{
    RESOLVE(sendto);
    ssize_t ret = real_sendto(fd, buf, len, flags, addr, addrlen);
    count(fd, ret, 1);
    return ret;
}

ssize_t recv(int fd, void *buf, size_t len, int flags)
{
    RESOLVE(recv);
    ssize_t ret = real_recv(fd, buf, len, flags);
    count(fd, ret, 0);
    return ret;
}

ssize_t recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *addr, socklen_t *addrlen)
{
    RESOLVE(recvfrom);
    ssize_t ret = real_recvfrom(fd, buf, len, flags, addr, addrlen);
    count(fd, ret, 0);
    return ret;
}

int close(int fd)
{
    RESOLVE(close);
    if (is_tracked(fd)) {
        /* Handshake and request each sent and acknowledged */
        if (fds[fd].sends == 2 && fds[fd].recvs == 2)
            _exit(0);
        fds[fd].tracked = 0;
        /* Startup connections are done; never act again in this process */
        if (--tracked_count == 0)
            active = 0;
    }
    return real_close(fd);
}
