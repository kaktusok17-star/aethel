#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_BLUE    "\033[34m"
#define C_CYAN    "\033[36m"
#define C_MAGENTA "\033[35m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"

static void strip_nl(char *s) {
    char *p = strpbrk(s, "\r\n");
    if (p) *p = '\0';
}

static void read_first_line(const char *path, char *out, size_t n) {
    out[0] = '\0';
    FILE *f = fopen(path, "r");
    if (!f) return;
    if (!fgets(out, (int)n, f)) out[0] = '\0';
    fclose(f);
    strip_nl(out);
}

static long meminfo_kb(const char *key) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char line[256];
    size_t klen = strlen(key);
    long val = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, klen) == 0 && line[klen] == ':') {
            sscanf(line + klen + 1, " %ld", &val);
            break;
        }
    }
    fclose(f);
    return val;
}

static void fmt_uptime(char *out, size_t n) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) { snprintf(out, n, "unknown"); return; }
    double up = 0;
    int ok = fscanf(f, "%lf", &up);
    fclose(f);
    if (ok != 1) { snprintf(out, n, "unknown"); return; }

    int t = (int)up;
    int d = t / 86400;
    int h = (t % 86400) / 3600;
    int m = (t % 3600) / 60;

    if (d > 0)      snprintf(out, n, "%dd %dh %dm", d, h, m);
    else if (h > 0) snprintf(out, n, "%dh %dm", h, m);
    else            snprintf(out, n, "%dm", m);
}

static void cpu_model(char *out, size_t n) {
    out[0] = '\0';
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) { snprintf(out, n, "unknown"); return; }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *c = strchr(line, ':');
            if (c) {
                c += 2;
                strip_nl(c);
                snprintf(out, n, "%s", c);
                break;
            }
        }
    }
    fclose(f);
    if (!out[0]) snprintf(out, n, "unknown");
}

static int cpu_count(void) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) return 0;
    char line[256];
    int c = 0;
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, "processor", 9) == 0) c++;
    fclose(f);
    return c;
}

static void read_version(char *out, size_t n) {
    out[0] = '\0';
    FILE *f = fopen("/etc/aethel-release", "r");
    if (!f) { snprintf(out, n, "unknown"); return; }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VERSION=", 8) == 0) {
            char *v = line + 8;
            strip_nl(v);
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

static const char *LOGO[] = {
    "       _         _   _          _ ",
    "      / \\   ___ | |_| |__   ___| |",
    "     / _ \\ / _ \\| __| '_ \\ / _ \\ |",
    "    / ___ \\  __/| |_| | | |  __/ |",
    "   /_/   \\_\\___| \\__|_| |_|\\___|_|",
    NULL
};

int main(void) {
    char version[64];
    char uptime[64];
    char cpu[256];
    char kernel[64];
    char hostname[256];

    read_version(version, sizeof(version));
    fmt_uptime(uptime, sizeof(uptime));
    cpu_model(cpu, sizeof(cpu));
    int ncpu = cpu_count();

    struct utsname u;
    if (uname(&u) == 0)
        snprintf(kernel, sizeof(kernel), "%s", u.release);
    else
        snprintf(kernel, sizeof(kernel), "unknown");

    if (gethostname(hostname, sizeof(hostname)) != 0)
        snprintf(hostname, sizeof(hostname), "aethel");
    hostname[sizeof(hostname)-1] = '\0';

    long total_kb = meminfo_kb("MemTotal");
    long avail_kb = meminfo_kb("MemAvailable");
    long used_kb  = total_kb - avail_kb;
    long total_mb = total_kb / 1024;
    long used_mb  = used_kb  / 1024;

    const char *shell = getenv("SHELL");
    if (!shell) shell = "/bin/sh";

    const char *term = getenv("TERM");
    if (!term) term = "console";

    const char *tty = ttyname(0);
    if (!tty) tty = "unknown";
    if (strncmp(tty, "/dev/", 5) == 0) tty += 5;

    char info[12][256];
    int n_info = 0;

    snprintf(info[n_info++], 256, "%s%sAethel Linux %s%s", C_BOLD, C_CYAN, version, C_RESET);
    snprintf(info[n_info++], 256, "%s─────────────────────%s", C_BLUE, C_RESET);
    snprintf(info[n_info++], 256, "%sHost:%s     %s", C_BOLD, C_RESET, hostname);
    snprintf(info[n_info++], 256, "%sKernel:%s   %s", C_BOLD, C_RESET, kernel);
    snprintf(info[n_info++], 256, "%sUptime:%s   %s", C_BOLD, C_RESET, uptime);
    snprintf(info[n_info++], 256, "%sShell:%s    %s", C_BOLD, C_RESET, shell);
    snprintf(info[n_info++], 256, "%sTerminal:%s %s", C_BOLD, C_RESET, tty);
    if (ncpu > 1)
        snprintf(info[n_info++], 256, "%sCPU:%s      %s (%d cores)", C_BOLD, C_RESET, cpu, ncpu);
    else
        snprintf(info[n_info++], 256, "%sCPU:%s      %s", C_BOLD, C_RESET, cpu);
    snprintf(info[n_info++], 256, "%sMemory:%s   %ld MB / %ld MB", C_BOLD, C_RESET, used_mb, total_mb);
    snprintf(info[n_info++], 256, "%sPackages:%s gris 1.0.0", C_BOLD, C_RESET);

    int n_logo = 0;
    while (LOGO[n_logo]) n_logo++;
    int rows = n_logo > n_info ? n_logo : n_info;

    printf("\n");
    for (int i = 0; i < rows; i++) {
        if (i < n_logo)
            printf("%s%s%s", C_MAGENTA, LOGO[i], C_RESET);
        else
            printf("%-34s", "");
        if (i < n_info)
            printf("  %s", info[i]);
        printf("\n");
    }
    printf("\n");

    return 0;
}