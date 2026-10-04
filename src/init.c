#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

/* Что просили сделать: 0=ничего, 1=halt, 2=poweroff, 3=reboot */
static volatile sig_atomic_t g_sigchld  = 0;
static volatile sig_atomic_t g_action   = 0;

static void on_sigchld(int sig) { (void)sig; g_sigchld = 1; }
static void on_sigint (int sig) { (void)sig; g_action = 3; }  /* Ctrl+Alt+Del */
static void on_sigterm(int sig) { (void)sig; g_action = 3; }  /* reboot */
static void on_sigusr1(int sig) { (void)sig; g_action = 1; }  /* halt */
static void on_sigusr2(int sig) { (void)sig; g_action = 2; }  /* poweroff */

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

    char buf[512];
    snprintf(buf, sizeof(buf),
        "\n"
        "=========================================\n"
        "     Welcome to Aethel Linux %s\n"
        "=========================================\n"
        "\n", ver);
    msg(buf);
}

static void try_mount(const char *src, const char *tgt, const char *type) {
    if (mount(src, tgt, type, 0, NULL) != 0 && errno != EBUSY) {
        char buf[256];
        snprintf(buf, sizeof(buf), "init: mount %s failed: %s\n",
                 tgt, strerror(errno));
        msg(buf);
    }
}

static void mount_all(void) {
    try_mount("proc",     "/proc", "proc");
    try_mount("sysfs",    "/sys",  "sysfs");
    try_mount("devtmpfs", "/dev",  "devtmpfs");
    mkdir("/tmp", 01777);
    mkdir("/run", 0755);
    try_mount("tmpfs", "/tmp", "tmpfs");
    try_mount("tmpfs", "/run", "tmpfs");
}

static void set_hostname(void) {
    char host[256];
    host[0] = '\0';

    FILE *f = fopen("/etc/hostname", "r");
    if (f) {
        if (fgets(host, sizeof(host), f)) {
            char *p = strpbrk(host, "\r\n");
            if (p) *p = '\0';
        }
        fclose(f);
    }

    if (!host[0]) snprintf(host, sizeof(host), "aethel");

    if (sethostname(host, strlen(host)) != 0) {
        msg("init: sethostname failed\n");
    }
}

/* Запустить login или first-boot setup на tty1 */
static pid_t spawn_login(void) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        setsid();
        int fd = open("/dev/tty1", O_RDWR);
        if (fd < 0) fd = open("/dev/console", O_RDWR);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2) close(fd);
        }
        ioctl(0, TIOCSCTTY, 0);

        char *envp[] = {
            "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
            "HOME=/root",
            "TERM=linux",
            "SHELL=/bin/sh",
            NULL
        };

        /* First boot → run aethel-setup. Otherwise → login. */
        struct stat st;
        if (stat("/etc/aethel/.configured", &st) != 0) {
            char *setup_argv[] = { "/bin/sh", "/usr/bin/aethel-setup", NULL };
            execve("/bin/sh", setup_argv, envp);
        } else {
            char *login_argv[] = { "/usr/bin/aethel-login", NULL };
            execve("/usr/bin/aethel-login", login_argv, envp);
        }

        /* Fallback: root shell */
        msg("init: no login/setup found, starting root shell\n");
        char *sh_argv[] = { "/bin/sh", "-l", NULL };
        execve("/bin/sh", sh_argv, envp);
        _exit(127);
    }
    return pid;
}

static void do_reboot(int action) {
    sync();
    sleep(1);
    sync();

    if (action == 1) {
        msg("\ninit: halting system...\n");
        reboot(RB_HALT_SYSTEM);
    } else if (action == 2) {
        msg("\ninit: powering off...\n");
        reboot(RB_POWER_OFF);
    } else {
        msg("\ninit: rebooting...\n");
        reboot(RB_AUTOBOOT);
    }

    msg("init: reboot() failed, trying ACPI poweroff...\n");
    reboot(RB_POWER_OFF);
    _exit(0);
}

int main(void) {
    if (getpid() != 1) {
        fprintf(stderr, "init: must run as PID 1 (got pid %d)\n", getpid());
        return 1;
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    sa.sa_handler = on_sigchld; sigaction(SIGCHLD, &sa, NULL);
    sa.sa_handler = on_sigint;  sigaction(SIGINT,  &sa, NULL);
    sa.sa_handler = on_sigterm; sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = on_sigusr1; sigaction(SIGUSR1, &sa, NULL);
    sa.sa_handler = on_sigusr2; sigaction(SIGUSR2, &sa, NULL);

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    mount_all();
    set_hostname();
    print_banner();

    pid_t shell = spawn_login();
    if (shell < 0) {
        msg("init: cannot spawn login\n");
        return 1;
    }

    while (1) {
        pause();

        if (g_action) {
            int act = g_action;
            g_action = 0;
            do_reboot(act);
        }

        if (g_sigchld) {
            g_sigchld = 0;
            int status;
            pid_t p;
            while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
                if (p == shell) {
                    msg("\ninit: session ended, restarting login...\n");
                    shell = spawn_login();
                }
            }
        }
    }

    return 0;
}