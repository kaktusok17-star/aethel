#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <crypt.h>
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_TRIES 3
#define MAX_LINE  256

#define C_CYAN  "\033[36m"
#define C_BOLD  "\033[1m"
#define C_RESET "\033[0m"

static void msg(const char *s) {
    if (write(STDOUT_FILENO, s, strlen(s)) < 0) { /* ignore */ }
}

static void read_version(char *out, size_t n) {
    out[0] = '\0';
    FILE *f = fopen("/etc/aethel-release", "r");
    if (!f) { snprintf(out, n, "unknown"); return; }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VERSION=", 8) == 0) {
            char *v = line + 8;
            char *nl = strpbrk(v, "\r\n");
            if (nl) *nl = '\0';
            if (v[0] == '"') v++;
            size_t len = strlen(v);
            if (len > 0 && v[len-1] == '"') v[len-1] = '\0';
            snprintf(out, n, "%s", v);
            break;
        }
    }
    fclose(f);
    if (!out[0]) snprintf(out, n, "unknown");
}

static void print_banner(void) {
    char ver[64];
    read_version(ver, sizeof(ver));

    char buf[2048];
    snprintf(buf, sizeof(buf),
        "\n"
        C_CYAN
        "       _         _   _          _ \n"
        "      / \\   ___ | |_| |__   ___| |\n"
        "     / _ \\ / _ \\| __| '_ \\ / _ \\ |\n"
        "    / ___ \\  __/| |_| | | |  __/ |\n"
        "   /_/   \\_\\___| \\__|_| |_|\\___|_|\n"
        C_RESET
        "                              " C_BOLD "Linux " C_RESET "%s\n"
        "\n"
        "   Console login required.\n"
        "   Default user: " C_BOLD "root" C_RESET "\n"
        "   Live mode:    password is " C_BOLD "empty" C_RESET "\n"
        "\n",
        ver);
    msg(buf);
}

static int read_line(char *buf, size_t n) {
    size_t i = 0;
    while (i < n - 1) {
        char c;
        ssize_t r = read(STDIN_FILENO, &c, 1);
        if (r <= 0) { buf[0] = '\0'; return -1; }
        if (c == '\n' || c == '\r') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (int)i;
}

static int read_password(char *buf, size_t n) {
    struct termios old_t, new_t;
    if (tcgetattr(STDIN_FILENO, &old_t) != 0)
        return read_line(buf, n);

    new_t = old_t;
    new_t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_t);

    int rc = read_line(buf, n);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_t);
    msg("\n");
    return rc;
}

static int get_shadow_hash(const char *user, char *out, size_t n) {
    FILE *f = fopen("/etc/shadow", "r");
    if (!f) return -1;

    char line[512];
    size_t ulen = strlen(user);

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, user, ulen) == 0 && line[ulen] == ':') {
            char *p = line + ulen + 1;
            char *colon = strchr(p, ':');
            if (colon) *colon = '\0';
            snprintf(out, n, "%s", p);
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return -1;
}

static int verify_password(const char *password, const char *hash) {
    if (!hash[0]) return 0;  /* empty hash = no password required */
    if (hash[0] == '!' || hash[0] == '*') return -1;
    char *computed = crypt(password, hash);
    if (!computed) return -1;
    return strcmp(computed, hash) == 0 ? 0 : -1;
}

static void start_shell(struct passwd *pw) {
    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        msg("login: initgroups failed\n");
        return;
    }
    if (setgid(pw->pw_gid) != 0) {
        msg("login: setgid failed\n");
        return;
    }
    if (setuid(pw->pw_uid) != 0) {
        msg("login: setuid failed\n");
        return;
    }

    if (chdir(pw->pw_dir) != 0)
        if (chdir("/") != 0) { /* ignore */ }

    char home_env[512], user_env[128], shell_env[256];
    snprintf(home_env,  sizeof(home_env),  "HOME=%s",  pw->pw_dir);
    snprintf(user_env,  sizeof(user_env),  "USER=%s",  pw->pw_name);
    snprintf(shell_env, sizeof(shell_env), "SHELL=%s", pw->pw_shell);

    char *envp[] = {
        "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        "TERM=linux",
        home_env,
        user_env,
        shell_env,
        NULL
    };

    const char *shell = pw->pw_shell[0] ? pw->pw_shell : "/bin/sh";
    char *argv[] = { (char *)shell, "-l", NULL };

    execve(shell, argv, envp);
    msg("login: cannot start shell\n");
    _exit(127);
}

int main(void) {
    ioctl(STDIN_FILENO, TIOCSCTTY, 0);

    print_banner();

    for (int attempt = 0; attempt < MAX_TRIES; attempt++) {
        char user[MAX_LINE];
        char pass[MAX_LINE];

        msg(C_BOLD "login:" C_RESET " ");
        int n = read_line(user, sizeof(user));
        if (n < 0) return 1;
        if (n == 0) { attempt--; continue; }

        msg(C_BOLD "Password:" C_RESET " ");
        if (read_password(pass, sizeof(pass)) < 0) return 1;

        struct passwd *pw = getpwnam(user);
        if (!pw) {
            msg("Login incorrect\n");
            sleep(1);
            continue;
        }

        char hash[256];
        if (get_shadow_hash(user, hash, sizeof(hash)) != 0) {
            msg("Login incorrect\n");
            sleep(1);
            continue;
        }

        if (verify_password(pass, hash) != 0) {
            msg("Login incorrect\n");
            sleep(1);
            continue;
        }

        msg("\n");
        start_shell(pw);
        return 1;
    }

    msg("Too many failed login attempts.\n");
    sleep(3);
    return 1;
}