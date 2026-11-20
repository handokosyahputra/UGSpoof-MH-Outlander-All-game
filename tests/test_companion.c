/* Tes companion + parser config + override prop (jalan di PC, pakai file sungguhan). */
#ifndef MODDIR
#error "compile dengan -DMODDIR=\"/path/ke/fixture\""
#endif
#include "../src/spoof.c"
int __android_log_print(int a, const char *b, const char *c, ...) { (void)a; (void)b; (void)c; return 0; }
extern int socketpair(int, int, int, int *);
extern int printf(const char *, ...);
static int fail = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("GAGAL: %s\n", msg); fail = 1; } else printf("ok   : %s\n", msg); } while (0)

static u32 ask(const char *pkg) {
    int sv[2]; socketpair(1 /*AF_UNIX*/, 1 /*SOCK_STREAM*/, 0, sv);
    u32 l = (u32)slen(pkg), len = 0;
    write(sv[1], &l, 4); write(sv[1], pkg, l);
    zygisk_companion_entry(sv[0]);
    readfull(sv[1], &len, 4);
    memset(&C, 0, sizeof C);
    if (len) { readfull(sv[1], cfgbuf, len); cfgbuf[len] = 0; parse_cfg(cfgbuf); }
    close(sv[0]); close(sv[1]);
    return len;
}
static const char *build(const char *k) { for (int i = 0; i < C.nbuild; i++) if (seq(C.build[i].k, k)) return C.build[i].v; return "(none)"; }

int main(void) {
    void *ra = (void *)main;   /* pemanggil = binary tes (bukan /system) -> dianggap kode game */
    CHECK(ask("com.game.a") > 0, "package di packages= jadi target");
    CHECK(C.gl_renderer && seq(C.gl_renderer, "Adreno (TM) 750"), "GPU dari profil default");
    CHECK(seq(build("MODEL"), "NX769J"), "Build.MODEL profil default");
    CHECK(C.vk_on == 0, "override config (enable_vulkan=0) menimpa default");
    const char *v = prop_override("ro.mediatek.platform", ra);
    CHECK(v && !*v, "wildcard ro.mediatek.* disembunyikan");
    v = prop_override("ro.product.model", ra);
    CHECK(v && seq(v, "NX769J"), "propall ro.product.model");
    CHECK(prop_override("ro.build.id", ra) == 0, "prop lain tidak disentuh");

    CHECK(ask("com.game.b:remote") == 0, "companion hanya cocokkan nama tanpa :suffix (client yang memotong)");
    CHECK(ask("com.game.b") > 0, "package kedua di daftar");
    CHECK(ask("com.game.c") > 0 && seq(build("MODEL"), "SM-S928B"), "map.<pkg> memakai profil khusus");
    CHECK(ask("com.evil") > 0 && seq(build("MODEL"), "(none)"), "nama profil '../' ditolak");
    CHECK(ask("com.android.chrome") == 0, "app lain tidak disentuh");
    puts(fail ? "\nHASIL: ADA YANG GAGAL" : "\nHASIL: SEMUA LULUS");
    return fail;
}
