/* Tes relocator ARM64 (jalan di PC). Cek hasil relokasi untuk pola prolog umum. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/reloc.h"
static int fail = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("GAGAL: %s\n", msg); fail = 1; } else printf("ok   : %s\n", msg); } while (0)
static u64 lit(const u32 *w, int i) { return (u64)w[i] | ((u64)w[i + 1] << 32); }

int main(void) {
    u32 out[64];
    /* 1) Stub glGetString Android: mrs; ldr; cbz x16,+12; ldr; br */
    u32 gl[] = {0xd53bd050, 0xf9400e10, 0xb4000070, 0xf9415610, 0xd61f0200};
    u64 pc = 0x7000001000ULL;
    int n = relocate4(gl, pc, out);
    CHECK(n == 13, "glGetString: panjang trampoline 13 word");
    CHECK(out[2] == 0xb4000050, "CBZ ditulis ulang ke +8");
    CHECK(out[3] == 0x14000005, "jalur kondisi-salah lompat +20");
    CHECK(lit(out, 6) == pc + 0x14, "target CBZ absolut = asli+0x14");
    CHECK(out[8] == gl[3], "instruksi ke-4 disalin apa adanya");
    CHECK(out[10] == 0xD65F0220, "lompat balik pakai RET X17 (aman BTI)");
    CHECK(lit(out, 11) == pc + 16, "lompat balik ke target+16");

    /* 2) paciasp; adrp x8; bl +0x100; ldr */
    u32 f2[] = {0xd503233f, 0xf0000008, 0x94000040, 0xf9400900};
    pc = 0x7000002004ULL;
    n = relocate4(f2, pc, out);
    CHECK(n == 15, "adrp+bl: panjang trampoline 15 word");
    CHECK(out[1] == 0x58000048, "ADRP jadi LDR X8, literal");
    CHECK(lit(out, 3) == 0x7000005000ULL, "nilai halaman ADRP benar");
    CHECK(out[5] == 0x100000BE, "BL: ADR X30 menunjuk instruksi berikutnya");
    CHECK(lit(out, 8) == pc + 8 + 0x100, "target BL absolut benar");

    /* 3) fungsi terlalu pendek ditolak */
    u32 f3[] = {0x14000010, 0, 0, 0};
    CHECK(relocate4(f3, 0x1000, out) == -1, "fungsi < 16 byte ditolak");
    u32 f4[] = {0xd503201f, 0xd65f03c0, 0, 0};
    CHECK(relocate4(f4, 0x1000, out) == -1, "RET di awal ditolak");

    /* 4) deteksi landing pad */
    CHECK(is_bti(0xD503245F) && is_bti(0xD503241F) && !is_bti(0xD503201F), "deteksi BTI");
    CHECK(is_pac(0xD503233F) && !is_pac(0xD503245F), "deteksi PACIASP");

    /* 5) follow_b mengikuti tail-call */
    u32 chain[8] = {0x14000002, 0, 0xd503201f};
    CHECK(follow_b(chain) == &chain[2], "follow_b mengikuti B");

    if (getenv("DISASM")) {
        FILE *f = fopen("/tmp/ugs_t.txt", "w");
        n = relocate4(gl, 0x7000001000ULL, out);
        for (int i = 0; i < n; i++) fprintf(f, "0x%02x,0x%02x,0x%02x,0x%02x\n", out[i] & 255, (out[i] >> 8) & 255, (out[i] >> 16) & 255, out[i] >> 24);
        fclose(f);
        if (system("llvm-mc --disassemble -triple=aarch64 /tmp/ugs_t.txt") != 0) puts("(llvm-mc tidak ada)");
    }
    puts(fail ? "\nHASIL: ADA YANG GAGAL" : "\nHASIL: SEMUA LULUS");
    return fail;
}
