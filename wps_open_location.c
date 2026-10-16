/*
 * Stand-in for the file managers that WPS starts for "Open File Location".
 *
 * WPS (ksolite::openFilePath) picks a file manager by XDG_CURRENT_DESKTOP and
 * starts it directly, e.g. `nautilus --select FILE`, `dolphin --select FILE`,
 * `peony --show-items FILE`, `kylin-explorer --select FILE` or
 * `dde-file-manager --show-item FILE`. None of these exist in the sandbox, so
 * WPS ends up exec'ing an empty path and nothing happens. This program is
 * installed under all of those names and asks the OpenURI portal to show the
 * file in the host file manager instead.
 *
 * The file is opened with O_RDONLY rather than O_PATH: the portal treats an
 * O_PATH fd to a file the user can write as writable and then rejects the
 * read-only OpenDirectory request with response 2.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

#define PORTAL_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_PATH "/org/freedesktop/portal/desktop"
#define RESPONSE_TIMEOUT_SECONDS 30

static GMainLoop *loop;
static guint response = G_MAXUINT;

static void on_response(GDBusConnection *bus, const gchar *sender,
                        const gchar *path, const gchar *iface,
                        const gchar *signal, GVariant *params, gpointer data)
{
    (void)bus, (void)sender, (void)path, (void)iface, (void)signal, (void)data;

    g_variant_get(params, "(u@a{sv})", &response, NULL);
    g_main_loop_quit(loop);
}

static gboolean on_timeout(gpointer data)
{
    (void)data;

    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

/* The path is the first argument that isn't an option; WPS passes one. */
static const char *find_target(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0)
            return i + 1 < argc ? argv[i + 1] : NULL;
        if (argv[i][0] != '-')
            return argv[i];
    }
    return NULL;
}

int main(int argc, char **argv)
{
    g_autofree char *prog = g_path_get_basename(argv[0]);
    const char *arg = find_target(argc, argv);
    if (!arg) {
        fprintf(stderr, "%s: no file given\n", prog);
        return 2;
    }

    g_autoptr(GFile) file = g_file_new_for_commandline_arg(arg);
    g_autofree char *path = g_file_get_path(file);
    if (!path) {
        fprintf(stderr, "%s: not a local file: %s\n", prog, arg);
        return 1;
    }

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "%s: cannot open %s: %s\n", prog, path,
                g_strerror(errno));
        return 1;
    }

    g_autoptr(GError) error = NULL;
    g_autoptr(GDBusConnection) bus =
        g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!bus) {
        fprintf(stderr, "%s: %s\n", prog, error->message);
        return 1;
    }

    /* Subscribe before calling so a fast response isn't missed. */
    loop = g_main_loop_new(NULL, FALSE);
    g_dbus_connection_signal_subscribe(
        bus, PORTAL_NAME, "org.freedesktop.portal.Request", "Response", NULL,
        NULL, G_DBUS_SIGNAL_FLAGS_NONE, on_response, NULL, NULL);

    g_autoptr(GUnixFDList) fds = g_unix_fd_list_new_from_array(&fd, 1);
    g_autoptr(GVariant) reply = g_dbus_connection_call_with_unix_fd_list_sync(
        bus, PORTAL_NAME, PORTAL_PATH, "org.freedesktop.portal.OpenURI",
        "OpenDirectory", g_variant_new("(sha{sv})", "", 0, NULL),
        G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, fds, NULL, NULL,
        &error);
    if (!reply) {
        fprintf(stderr, "%s: OpenDirectory failed: %s\n", prog,
                error->message);
        return 1;
    }

    /* Stay connected until the portal is done, then report its answer. */
    g_timeout_add_seconds(RESPONSE_TIMEOUT_SECONDS, on_timeout, NULL);
    g_main_loop_run(loop);

    if (response != 0) {
        fprintf(stderr, "%s: portal response %d for %s\n", prog,
                response == G_MAXUINT ? -1 : (int)response, path);
        return 1;
    }
    return 0;
}
