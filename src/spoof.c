/* Universal Game Spoof (UGSpoof) - Zygisk module (API v4)
 * Spoof per-game: OpenGL ES strings, Vulkan device props, android.os.Build, native props.
 * Ditulis dari nol, freestanding (tanpa NDK). Target: arm64-v8a. */
#include "reloc.h"

typedef unsigned long size_t;
typedef long ssize_t;
typedef unsigned char u8;

#define EXPORT __attribute__((visibility("default")))
#define HIDDEN __attribute__((visibility("hidden")))

/* ---------- libc / libdl / liblog ---------- */
extern void *dlopen(const char *, int);
extern void *dlsym(void *, const char *);
typedef struct { const char *dli_fname; void *dli_fbase; const char *dli_sname; void *dli_saddr; } Dl_info;
extern int dladdr(const void *, Dl_info *);
extern void *mmap(void *, size_t, int, int, int, long);
extern int mprotect(void *, size_t, int);
extern int open(const char *, int, ...);
extern ssize_t read(int, void *, size_t);
extern ssize_t write(int, const void *, size_t);
extern int close(int);
extern unsigned long getauxval(unsigned long);
extern int __android_log_print(int, const char *, const char *, ...);

#define TAG "UGSpoof"
#define LOGI(...) __android_log_print(4, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(6, TAG, __VA_ARGS__)
#define RTLD_NOW 2
#define PROT_R 1
#define PROT_W 2
#define PROT_X 4
#define MAP_PRIVATE 2
#define MAP_ANON 0x20

/* ---------- util string ---------- */
HIDDEN void *memcpy(void *d, const void *s, size_t n) { u8 *a = d; const u8 *b = s; while (n--) *a++ = *b++; return d; }
HIDDEN void *memset(void *d, int c, size_t n) { u8 *a = d; while (n--) *a++ = (u8)c; return d; }
static size_t slen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static int seq(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return *a == *b; }
static int sstarts(const char *s, const char *p) { while (*p) if (*s++ != *p++) return 0; return 1; }
static int scontains(const char *s, const char *p) { for (; *s; s++) if (sstarts(s, p)) return 1; return 0; }
static void scpy(char *d, const char *s, size_t cap) { size_t i = 0; for (; s[i] && i + 1 < cap; i++) d[i] = s[i]; d[i] = 0; }
static u64 parse_num(const char *s) {
    u64 v = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        for (s += 2; *s; s++) {
            char c = *s; int d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else break;
            v = v * 16 + d;
        }
    } else for (; *s >= '0' && *s <= '9'; s++) v = v * 10 + (*s - '0');
    return v;
}

/* ---------- konfigurasi ---------- */
#define MAXKV 64
typedef struct { const char *k; const char *v; int all; } kv;
/* Key yang sama -> nilai belakangan menang (config.txt menimpa profil). */
static void upsert(kv *arr, int *n, const char *k, const char *v, int all) {
    for (int i = 0; i < *n; i++) if (seq(arr[i].k, k)) { arr[i].v = v; arr[i].all = all; return; }
    if (*n < MAXKV) { arr[*n].k = k; arr[*n].v = v; arr[*n].all = all; (*n)++; }
}
static char cfgbuf[20480];
static struct {
    const char *packages;
    const char *gl_vendor, *gl_renderer, *gl_version;
    u32 vk_vendor, vk_device, vk_drvver, vk_drvid;
    const char *vk_name, *vk_drvname, *vk_drvinfo;
    int gl_on, vk_on, build_on, prop_on, all_callers;
    kv build[MAXKV]; int nbuild;
    kv prop[MAXKV]; int nprop;
} C;

static void parse_cfg(char *p) {
    C.gl_on = C.vk_on = C.build_on = C.prop_on = 1;
    while (*p) {
        char *line = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        size_t L = slen(line);
        while (L && (line[L - 1] == '\r' || line[L - 1] == ' ')) line[--L] = 0;
        if (!line[0] || line[0] == '#') continue;
        char *eq = line; while (*eq && *eq != '=') eq++;
        if (!*eq) continue;
        *eq = 0; const char *k = line, *v = eq + 1;
        if (seq(k, "packages")) C.packages = v;
        else if (seq(k, "gl_vendor")) C.gl_vendor = v;
        else if (seq(k, "gl_renderer")) C.gl_renderer = v;
        else if (seq(k, "gl_version")) C.gl_version = v;
        else if (seq(k, "vk_vendor_id")) C.vk_vendor = (u32)parse_num(v);
        else if (seq(k, "vk_device_id")) C.vk_device = (u32)parse_num(v);
        else if (seq(k, "vk_driver_version")) C.vk_drvver = (u32)parse_num(v);
        else if (seq(k, "vk_driver_id")) C.vk_drvid = (u32)parse_num(v);
        else if (seq(k, "vk_device_name")) C.vk_name = v;
        else if (seq(k, "vk_driver_name")) C.vk_drvname = v;
        else if (seq(k, "vk_driver_info")) C.vk_drvinfo = v;
        else if (seq(k, "enable_gl")) C.gl_on = (int)parse_num(v);
        else if (seq(k, "enable_vulkan")) C.vk_on = (int)parse_num(v);
        else if (seq(k, "enable_build")) C.build_on = (int)parse_num(v);
        else if (seq(k, "enable_props")) C.prop_on = (int)parse_num(v);
        else if (seq(k, "spoof_system_callers")) C.all_callers = (int)parse_num(v);
        else if (sstarts(k, "build.")) upsert(C.build, &C.nbuild, k + 6, v, 0);
        else if (sstarts(k, "propall.")) upsert(C.prop, &C.nprop, k + 8, v, 1);
        else if (sstarts(k, "prop.")) upsert(C.prop, &C.nprop, k + 5, v, 0);
    }
}

/* Apakah "name" (tanpa ":suffix") ada di daftar koma "list". */
static int list_has(const char *list, const char *name) {
    if (!list || !*list) return 0;
    const char *p = list;
    size_t nl = slen(name);
    while (*p && *p != '\n') {
        while (*p == ',' || *p == ' ') p++;
        const char *s = p; while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
        size_t n = (size_t)(p - s);
        while (n && s[n - 1] == ' ') n--;
        if (n && n == nl) { size_t i = 0; while (i < n && name[i] == s[i]) i++; if (i == n) return 1; }
        if (*p == '\r') p++;
    }
    return 0;
}

/* ---------- caller filter ---------- */
/* GL/VK: jangan spoof ke renderer sistem (UI/hwui/ANGLE/loader) supaya UI tidak rusak. */
static int gpu_caller_ok(void *ra) {
    if (C.all_callers) return 1;
    Dl_info i;
    if (!dladdr(ra, &i) || !i.dli_fname) return 1;
    const char *f = i.dli_fname;
    if (scontains(f, "libhwui") || scontains(f, "libskia") || scontains(f, "libgui") ||
        scontains(f, "libvulkan") || scontains(f, "libEGL") || scontains(f, "libGLES") ||
        scontains(f, "/vendor/") || scontains(f, "libsurfaceflinger")) return 0;
    return 1;
}
/* Props: hanya untuk kode game (di luar partisi sistem) agar driver/HAL tetap normal. */
static int prop_caller_ok(void *ra) {
    Dl_info i;
    if (!dladdr(ra, &i) || !i.dli_fname) return 1;
    const char *f = i.dli_fname;
    if (sstarts(f, "/system") || sstarts(f, "/apex") || sstarts(f, "/vendor") ||
        sstarts(f, "/product") || sstarts(f, "/odm")) return 0;
    return 1;
}

/* ---------- inline hook ---------- */
static void flush_icache(void *start, size_t len) {
#if defined(__aarch64__)
    u64 ctr; __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    u64 dl = 4ULL << ((ctr >> 16) & 0xF), il = 4ULL << (ctr & 0xF);
    u64 s = (u64)start, e = s + len, a;
    for (a = s & ~(dl - 1); a < e; a += dl) __asm__ volatile("dc cvau, %0" ::"r"(a) : "memory");
    __asm__ volatile("dsb ish" ::: "memory");
    for (a = s & ~(il - 1); a < e; a += il) __asm__ volatile("ic ivau, %0" ::"r"(a) : "memory");
    __asm__ volatile("dsb ish\n\tisb" ::: "memory");
#else
    (void)start; (void)len;
#endif
}

static int hook(void *target, void *repl, void **orig, const char *what) {
    if (!target) { LOGE("%s: simbol tidak ditemukan", what); return 0; }
    u32 *t = (u32 *)follow_b((const u32 *)target);
    u64 pg = getauxval(6 /*AT_PAGESZ*/); if (!pg) pg = 4096;
    u32 *tr = mmap(0, pg, PROT_R | PROT_W, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (tr == (void *)-1) { LOGE("%s: mmap gagal", what); return 0; }
    /* CPU ARMv8.5+ (BTI/PAC): landing pad di instruksi pertama wajib dipertahankan. */
    int start = 0, k = 0, pac = 0;
    if (is_bti(t[0])) start = 1;
    else if (is_pac(t[0])) { start = 1; pac = 1; tr[k++] = t[0]; } /* PACIxSP pindah ke trampoline */
    u32 *site = t + start;
    int n = relocate4(site, (u64)site, tr + k);
    if (n < 0) { LOGE("%s: prolog tidak bisa direlokasi", what); return 0; }
    n += k;
    if (mprotect(tr, pg, PROT_R | PROT_X)) { LOGE("%s: mprotect tramp gagal", what); return 0; }
    flush_icache(tr, (size_t)n * 4);
    u64 a = (u64)t & ~(pg - 1), e = ((u64)t + 20 + pg - 1) & ~(pg - 1);
    if (mprotect((void *)a, e - a, PROT_R | PROT_W | PROT_X)) { LOGE("%s: mprotect target gagal", what); return 0; }
    if (orig) *orig = tr;
    make_jump(site, (u64)repl);
    if (pac) t[0] = BTI_C;   /* tetap landing pad, tapi tanpa menandatangani LR */
    mprotect((void *)a, e - a, PROT_R | PROT_X);
    flush_icache(t, 20);
    LOGI("hook OK: %s @%p", what, t);
    return 1;
}

/* ---------- OpenGL ES ---------- */
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
typedef const u8 *(*glGetString_t)(u32);
static glGetString_t o_gl[3];
static const u8 *gl_common(int idx, u32 name, void *ra) {
    const u8 *r = o_gl[idx](name);
    if (!gpu_caller_ok(ra)) return r;
    if (name == GL_VENDOR && C.gl_vendor && *C.gl_vendor) return (const u8 *)C.gl_vendor;
    if (name == GL_RENDERER && C.gl_renderer && *C.gl_renderer) return (const u8 *)C.gl_renderer;
    if (name == GL_VERSION && C.gl_version && *C.gl_version && r) return (const u8 *)C.gl_version;
    return r;
}
static const u8 *h_gl0(u32 n) { return gl_common(0, n, __builtin_return_address(0)); }
static const u8 *h_gl1(u32 n) { return gl_common(1, n, __builtin_return_address(0)); }
static const u8 *h_gl2(u32 n) { return gl_common(2, n, __builtin_return_address(0)); }

/* ---------- Vulkan ---------- */
#define ST_DRIVER_PROPS 1000196000u
#define ST_VK12_PROPS 52u
typedef void (*gpdp_t)(void *, void *);
typedef void *(*gipa_t)(void *, const char *);
static gipa_t o_gipa;
static gpdp_t o_gpdp, o_gpdp2;         /* trampoline export */
static gpdp_t r_gpdp, r_gpdp2, r_gpdp2k; /* dari vkGetInstanceProcAddr */

static void patch_props(u8 *p) {
    u32 *w = (u32 *)p;
    if (C.vk_drvver) w[1] = C.vk_drvver;
    if (C.vk_vendor) w[2] = C.vk_vendor;
    if (C.vk_device) w[3] = C.vk_device;
    if (C.vk_name && *C.vk_name) scpy((char *)p + 20, C.vk_name, 256);
}
static void patch_props2(u8 *p2) {
    patch_props(p2 + 16);
    u8 *n = *(u8 **)(p2 + 8);
    for (int guard = 0; n && guard < 64; guard++) {
        u32 st = *(u32 *)n;
        if (st == ST_DRIVER_PROPS || st == ST_VK12_PROPS) {
            if (C.vk_drvid) *(u32 *)(n + 16) = C.vk_drvid;
            if (C.vk_drvname && *C.vk_drvname) scpy((char *)n + 20, C.vk_drvname, 256);
            if (C.vk_drvinfo && *C.vk_drvinfo) scpy((char *)n + 276, C.vk_drvinfo, 256);
        }
        n = *(u8 **)(n + 8);
    }
}
static void h_gpdp(void *pd, void *pr) { o_gpdp(pd, pr); if (pr && gpu_caller_ok(__builtin_return_address(0))) patch_props(pr); }
static void h_gpdp2(void *pd, void *pr) { o_gpdp2(pd, pr); if (pr && gpu_caller_ok(__builtin_return_address(0))) patch_props2(pr); }
static void w_gpdp(void *pd, void *pr) { r_gpdp(pd, pr); if (pr) patch_props(pr); }
static void w_gpdp2(void *pd, void *pr) { r_gpdp2(pd, pr); if (pr) patch_props2(pr); }
static void w_gpdp2k(void *pd, void *pr) { r_gpdp2k(pd, pr); if (pr) patch_props2(pr); }
static void *h_gipa(void *inst, const char *name) {
    void *ra = __builtin_return_address(0);
    void *r = o_gipa(inst, name);
    if (!r || !name || !gpu_caller_ok(ra)) return r;
    if (seq(name, "vkGetInstanceProcAddr")) return (void *)h_gipa;
    if (seq(name, "vkGetPhysicalDeviceProperties")) { if ((void *)r != (void *)w_gpdp) r_gpdp = r; return (void *)w_gpdp; }
    if (seq(name, "vkGetPhysicalDeviceProperties2")) { if ((void *)r != (void *)w_gpdp2) r_gpdp2 = r; return (void *)w_gpdp2; }
    if (seq(name, "vkGetPhysicalDeviceProperties2KHR")) { if ((void *)r != (void *)w_gpdp2k) r_gpdp2k = r; return (void *)w_gpdp2k; }
    return r;
}

/* ---------- native system properties ---------- */
typedef int (*spget_t)(const char *, char *);
typedef void (*sprcb_cb)(void *, const char *, const char *, u32);
typedef void (*sprcb_t)(const void *, sprcb_cb, void *);
static spget_t o_spget;
static sprcb_t o_sprcb;
/* prop.X    : hanya untuk kode game (aman untuk prop yang dipakai driver/HAL)
 * propall.X : semua pemanggil di proses game (identitas: model, brand, dll)
 * Key berakhiran '*' = cocokkan awalan, mis. prop.ro.mediatek.*= */
static int key_match(const char *pat, const char *name) {
    size_t L = slen(pat);
    if (L && pat[L - 1] == '*') { for (size_t i = 0; i + 1 < L; i++) if (pat[i] != name[i]) return 0; return 1; }
    return seq(pat, name);
}
/* Cek caller (dladdr) hanya kalau benar-benar perlu, supaya baca prop tetap cepat. */
static const char *prop_override(const char *name, void *ra) {
    const kv *best = 0;
    for (int i = 0; i < C.nprop; i++)
        if (key_match(C.prop[i].k, name)) {
            best = &C.prop[i];
            if (seq(C.prop[i].k, name)) break;        /* exact match prioritas */
        }
    if (!best) return 0;
    if (best->all || prop_caller_ok(ra)) return best->v;
    return 0;
}
static int h_spget(const char *name, char *value) {
    int r = o_spget(name, value);
    if (name && value) {
        const char *v = prop_override(name, __builtin_return_address(0));
        if (v) { scpy(value, v, 92); r = (int)slen(value); }
    }
    return r;
}
struct cbctx { sprcb_cb cb; void *cookie; void *ra; };
static void my_cb(void *c, const char *name, const char *value, u32 serial) {
    struct cbctx *x = c;
    const char *v = name ? prop_override(name, x->ra) : 0;
    x->cb(x->cookie, name, v ? v : value, serial);
}
static void h_sprcb(const void *pi, sprcb_cb cb, void *cookie) {
    if (!cb) { o_sprcb(pi, cb, cookie); return; }
    struct cbctx x = { cb, cookie, __builtin_return_address(0) };
    o_sprcb(pi, my_cb, &x);
}

/* ---------- JNI (android.os.Build) ---------- */
typedef void *jobject;
typedef void JNIEnv;
#define JF(env, i) ((*(void ***)(env))[i])
static void spoof_build(JNIEnv *env) {
    jobject (*FindClass)(JNIEnv *, const char *) = JF(env, 6);
    void (*ExceptionClear)(JNIEnv *) = JF(env, 17);
    void (*DeleteLocalRef)(JNIEnv *, jobject) = JF(env, 23);
    void *(*GetStaticFieldID)(JNIEnv *, jobject, const char *, const char *) = JF(env, 144);
    void (*SetStaticObjectField)(JNIEnv *, jobject, void *, jobject) = JF(env, 154);
    jobject (*NewStringUTF)(JNIEnv *, const char *) = JF(env, 167);
    u8 (*ExceptionCheck)(JNIEnv *) = JF(env, 228);

    jobject cls = FindClass(env, "android/os/Build");
    if (!cls || ExceptionCheck(env)) { ExceptionClear(env); return; }
    for (int i = 0; i < C.nbuild; i++) {
        void *fid = GetStaticFieldID(env, cls, C.build[i].k, "Ljava/lang/String;");
        if (!fid || ExceptionCheck(env)) { ExceptionClear(env); continue; }
        jobject s = NewStringUTF(env, C.build[i].v);
        SetStaticObjectField(env, cls, fid, s);
        DeleteLocalRef(env, s);
    }
    DeleteLocalRef(env, cls);
}

/* ---------- Zygisk API v4 ---------- */
struct module_abi;
typedef struct api_table {
    void *impl;
    _Bool (*registerModule)(struct api_table *, struct module_abi *);
    void *hookJniNativeMethods, *pltHookRegister, *exemptFd, *pltHookCommit;
    int (*connectCompanion)(void *);
    void (*setOption)(void *, int);
    int (*getModuleDir)(void *);
    u32 (*getFlags)(void *);
} api_table;
typedef struct module_abi {
    long api_version;
    void *impl;
    void (*preAppSpecialize)(void *, void *);
    void (*postAppSpecialize)(void *, const void *);
    void (*preServerSpecialize)(void *, void *);
    void (*postServerSpecialize)(void *, const void *);
} module_abi;

static api_table *g_api;
static JNIEnv *g_env;
static int g_target;
static char g_proc[256];

static int readfull(int fd, void *b, size_t n) {
    u8 *p = b;
    while (n) { ssize_t r = read(fd, p, n); if (r <= 0) return 0; p += r; n -= (size_t)r; }
    return 1;
}

static void pre_app(void *self, void *a) {
    (void)self;
    void **args = (void **)a;
    jobject *pnice = (jobject *)args[7];
    g_target = 0;
    if (pnice && *pnice) {
        const char *(*GetStringUTFChars)(JNIEnv *, jobject, u8 *) = JF(g_env, 169);
        void (*ReleaseStringUTFChars)(JNIEnv *, jobject, const char *) = JF(g_env, 170);
        const char *s = GetStringUTFChars(g_env, *pnice, 0);
        if (s) { scpy(g_proc, s, sizeof g_proc); ReleaseStringUTFChars(g_env, *pnice, s); }
    }
    if (g_proc[0] && !scontains(g_proc, "_zygote")) {
        int fd = g_api->connectCompanion(g_api->impl);
        if (fd >= 0) {
            char base[256]; scpy(base, g_proc, sizeof base);
            for (char *q = base; *q; q++) if (*q == ':') { *q = 0; break; }
            u32 nl = (u32)slen(base), len = 0;
            write(fd, &nl, 4); write(fd, base, nl);
            if (readfull(fd, &len, 4) && len > 0 && len < sizeof cfgbuf - 1 && readfull(fd, cfgbuf, len)) {
                cfgbuf[len] = 0;
                parse_cfg(cfgbuf);
                g_target = 1;
            }
            close(fd);
        }
    }
    if (!g_target) g_api->setOption(g_api->impl, 1 /* DLCLOSE_MODULE_LIBRARY */);
}

static void post_app(void *self, const void *a) {
    (void)self; (void)a;
    if (!g_target) return;
    LOGI("target: %s", g_proc);
    if (C.build_on) spoof_build(g_env);
    if (C.gl_on) {
        const char *libs[3] = { "libGLESv2.so", "libGLESv3.so", "libGLESv1_CM.so" };
        const u8 *(*hs[3])(u32) = { h_gl0, h_gl1, h_gl2 };
        for (int i = 0; i < 3; i++) {
            void *h = dlopen(libs[i], RTLD_NOW);
            if (h) hook(dlsym(h, "glGetString"), (void *)hs[i], (void **)&o_gl[i], libs[i]);
        }
    }
    if (C.vk_on) {
        void *h = dlopen("libvulkan.so", RTLD_NOW);
        if (h) {
            hook(dlsym(h, "vkGetInstanceProcAddr"), (void *)h_gipa, (void **)&o_gipa, "vkGetInstanceProcAddr");
            void *p1 = dlsym(h, "vkGetPhysicalDeviceProperties");
            void *p2 = dlsym(h, "vkGetPhysicalDeviceProperties2");
            if (hook(p1, (void *)h_gpdp, (void **)&o_gpdp, "vkGetPhysicalDeviceProperties")) r_gpdp = o_gpdp;
            if (hook(p2, (void *)h_gpdp2, (void **)&o_gpdp2, "vkGetPhysicalDeviceProperties2")) r_gpdp2k = r_gpdp2 = o_gpdp2;
        }
    }
    if (C.prop_on && C.nprop) {
        void *h = dlopen("libc.so", RTLD_NOW);
        if (h) {
            hook(dlsym(h, "__system_property_get"), (void *)h_spget, (void **)&o_spget, "__system_property_get");
            hook(dlsym(h, "__system_property_read_callback"), (void *)h_sprcb, (void **)&o_sprcb, "__system_property_read_callback");
        }
    }
}

static void pre_server(void *s, void *a) { (void)s; (void)a; g_api->setOption(g_api->impl, 1); }
static void post_server(void *s, const void *a) { (void)s; (void)a; }

static module_abi g_abi = { 4, 0, pre_app, post_app, pre_server, post_server };

EXPORT void zygisk_module_entry(api_table *table, JNIEnv *env) {
    g_api = table; g_env = env;
    g_abi.impl = &g_abi;
    table->registerModule(table, &g_abi);
}

/* ---------- Companion (root) ----------
 * Terima nama package, tentukan target & profil, kirim: isi profil + config.txt.
 * Non-target -> kirim 0 (modul langsung lepas dari proses itu). */
#ifndef MODDIR
#define MODDIR "/data/adb/modules/ugspoof"
#endif
static u32 slurp(const char *path, char *buf, u32 cap) {
    u32 len = 0; ssize_t r;
    int fd = open(path, 0);
    if (fd < 0) return 0;
    while (len < cap && (r = read(fd, buf + len, cap - len)) > 0) len += (u32)r;
    close(fd);
    return len;
}
/* Cari nilai "key=" di awal baris. Return pointer ke nilai (berakhir di \n). */
static const char *find_val(const char *buf, const char *key) {
    const char *p = buf;
    while (*p) {
        if (sstarts(p, key)) return p + slen(key);
        while (*p && *p != '\n') p++;
        if (*p) p++;
    }
    return 0;
}
static void copy_line(char *d, const char *s, size_t cap) {
    size_t i = 0;
    for (; s && s[i] && s[i] != '\n' && s[i] != '\r' && i + 1 < cap; i++) d[i] = s[i];
    while (i && d[i - 1] == ' ') i--;
    d[i] = 0;
}
EXPORT void zygisk_companion_entry(int client) {
    static char cfg[8192], prof[8192], out[16384];
    char name[256], key[300], pname[64], path[160];
    u32 nl = 0, zero = 0;
    if (!readfull(client, &nl, 4) || nl == 0 || nl >= sizeof name || !readfull(client, name, nl)) { write(client, &zero, 4); return; }
    name[nl] = 0;
    u32 cl = slurp(MODDIR "/config.txt", cfg, sizeof cfg - 1); cfg[cl] = 0;

    /* map.<package>=<profil> menang, lalu packages= memakai profile= */
    scpy(key, "map.", sizeof key); scpy(key + 4, name, sizeof key - 4);
    size_t kl = slen(key); key[kl] = '='; key[kl + 1] = 0;
    const char *mv = find_val(cfg, key);
    if (mv) copy_line(pname, mv, sizeof pname);
    else if (list_has(find_val(cfg, "packages="), name)) copy_line(pname, find_val(cfg, "profile="), sizeof pname);
    else { write(client, &zero, 4); return; }

    /* nama profil hanya [a-z0-9_-] (anti path traversal) */
    for (char *q = pname; *q; q++)
        if (!((*q >= 'a' && *q <= 'z') || (*q >= '0' && *q <= '9') || *q == '_' || *q == '-')) { pname[0] = 0; break; }
    u32 pl = 0;
    if (pname[0]) {
        scpy(path, MODDIR "/profiles/", sizeof path);
        size_t L = slen(path); scpy(path + L, pname, sizeof path - L);
        L = slen(path); scpy(path + L, ".conf", sizeof path - L);
        pl = slurp(path, prof, sizeof prof - 1);
    }
    u32 len = 0;
    memcpy(out, prof, pl); len += pl;
    out[len++] = '\n';
    memcpy(out + len, cfg, cl); len += cl;
    write(client, &len, 4);
    write(client, out, len);
}
