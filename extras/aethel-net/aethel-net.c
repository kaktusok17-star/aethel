#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define CONF_FILE   "/etc/aethel/network.conf"
#define SCRIPT_FILE "/etc/aethel/udhcpc.script"
#define RESOLV_FILE "/etc/resolv.conf"
#define STATE_FILE  "/run/aethel-net.state"

typedef struct {
    char iface[64];
    int  dhcp;
    char ip[64];
    char gateway[64];
    char dns[128];
} NetConf;

static void msg(const char *s) {
    fputs(s, stdout);
    fputc('\n', stdout);
}

static int run(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static void trim(char *s) {
    char *p = s + strlen(s);
    while (p > s && (p[-1] == ' ' || p[-1] == '\t' ||
                     p[-1] == '\n' || p[-1] == '\r'))
        *--p = '\0';
    p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

static int read_conf(NetConf *c) {
    memset(c, 0, sizeof(*c));
    snprintf(c->iface, sizeof(c->iface), "eth0");
    c->dhcp = 1;

    FILE *f = fopen(CONF_FILE, "r");
    if (!f) return 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (!*line || *line == '#') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *k = line, *v = eq + 1;
        trim(k);
        trim(v);

        if      (!strcmp(k, "interface")) snprintf(c->iface, sizeof(c->iface), "%s", v);
        else if (!strcmp(k, "dhcp"))      c->dhcp = (!strcmp(v, "yes") || !strcmp(v, "1") || !strcmp(v, "true"));
        else if (!strcmp(k, "ip"))        snprintf(c->ip, sizeof(c->ip), "%s", v);
        else if (!strcmp(k, "gateway"))   snprintf(c->gateway, sizeof(c->gateway), "%s", v);
        else if (!strcmp(k, "dns"))       snprintf(c->dns, sizeof(c->dns), "%s", v);
    }
    fclose(f);
    return 0;
}

static void save_state(const char *iface, const char *status) {
    FILE *f = fopen(STATE_FILE, "w");
    if (!f) return;
    fprintf(f, "iface=%s\nstatus=%s\n", iface, status);
    fclose(f);
}

static void cmd_up(NetConf *c) {
    char buf[256];
    snprintf(buf, sizeof(buf), "aethel-net: bringing up %s", c->iface);
    msg(buf);

    char *up_argv[] = { "ip", "link", "set", c->iface, "up", NULL };
    if (run(up_argv) != 0) {
        msg("aethel-net: failed to bring interface up");
        return;
    }

    if (c->dhcp) {
        msg("aethel-net: requesting DHCP...");
        char *udhcpc[] = {
            "udhcpc", "-i", c->iface, "-n", "-q", "-t", "5",
            "-s", SCRIPT_FILE, NULL
        };
        if (run(udhcpc) != 0) {
            msg("aethel-net: DHCP failed");
            save_state(c->iface, "error");
            return;
        }
        msg("aethel-net: DHCP ok");
    } else {
        if (c->ip[0]) {
            char *add[] = { "ip", "addr", "add", c->ip, "dev", c->iface, NULL };
            if (run(add) != 0)
                msg("aethel-net: ip addr add failed");
        }
        if (c->gateway[0]) {
            char *route[] = { "ip", "route", "add", "default", "via",
                              c->gateway, "dev", c->iface, NULL };
            if (run(route) != 0)
                msg("aethel-net: route add failed");
        }
        if (c->dns[0]) {
            FILE *f = fopen(RESOLV_FILE, "w");
            if (f) {
                fprintf(f, "nameserver %s\n", c->dns);
                fclose(f);
            }
        }
    }

    save_state(c->iface, "up");
    msg("aethel-net: interface is up");
}

static void cmd_down(NetConf *c) {
    char *argv[] = { "ip", "link", "set", c->iface, "down", NULL };
    run(argv);
    save_state(c->iface, "down");
    msg("aethel-net: interface is down");
}

static void cmd_status(NetConf *c) {
    char buf[256];

    snprintf(buf, sizeof(buf), "interface: %s", c->iface);
    msg(buf);
    snprintf(buf, sizeof(buf), "mode:      %s", c->dhcp ? "dhcp" : "static");
    msg(buf);

    if (c->dhcp == 0) {
        snprintf(buf, sizeof(buf), "ip:        %s", c->ip);
        msg(buf);
        snprintf(buf, sizeof(buf), "gateway:   %s", c->gateway);
        msg(buf);
        snprintf(buf, sizeof(buf), "dns:       %s", c->dns);
        msg(buf);
    }

    msg("");
    msg("--- ip addr ---");
    char *addr[] = { "ip", "-o", "addr", "show", c->iface, NULL };
    run(addr);

    msg("--- routes ---");
    char *route[] = { "ip", "route", NULL };
    run(route);

    msg("--- resolv.conf ---");
    FILE *f = fopen(RESOLV_FILE, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) fputs(line, stdout);
        fclose(f);
    } else {
        msg("(no resolv.conf)");
    }
}

static void usage(void) {
    printf(
        "aethel-net — network manager for Aethel Linux\n"
        "\n"
        "Usage:\n"
        "  aethel-net up       bring up interface\n"
        "  aethel-net down     bring down interface\n"
        "  aethel-net status   show network status\n"
        "\n"
        "Config file: " CONF_FILE "\n"
    );
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 2; }

    NetConf c;
    read_conf(&c);

    if      (!strcmp(argv[1], "up"))     cmd_up(&c);
    else if (!strcmp(argv[1], "down"))   cmd_down(&c);
    else if (!strcmp(argv[1], "status")) cmd_status(&c);
    else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) usage();
    else { usage(); return 2; }

    return 0;
}