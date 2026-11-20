/* ARM64 inline-hook relocator (tanpa library eksternal).
 * Menyalin 4 instruksi pertama fungsi target ke trampoline,
 * memperbaiki instruksi PC-relative, lalu lompat balik ke target+16. */
#pragma once
typedef unsigned int u32;
typedef unsigned long long u64;
typedef long long i64;

static i64 sext(u64 v, int bits) {
    u64 m = 1ULL << (bits - 1);
    v &= (1ULL << bits) - 1;
    return (i64)((v ^ m) - m);
}

#define EMIT(x) (dst[n++] = (u32)(x))
#define EMIT64(v) do { EMIT((u64)(v) & 0xffffffffULL); EMIT((u64)(v) >> 32); } while (0)

/* Instruksi yang mengakhiri fungsi (tidak boleh muncul di index 0..2). */
static int is_terminator(u32 ins) {
    if ((ins & 0xFC000000) == 0x14000000) return 1;            /* B */
    if ((ins & 0xFFFFFC1F) == 0xD61F0000) return 1;            /* BR */
    if ((ins & 0xFFFFFC1F) == 0xD65F0000) return 1;            /* RET */
    return 0;
}

/* src: alamat instruksi asli (juga dipakai sbg PC).
 * dst: buffer trampoline (min 64 word). Return jumlah word, atau -1 jika gagal. */
static int relocate4(const u32 *src, u64 pc0, u32 *dst) {
    int n = 0;
    for (int i = 0; i < 4; i++) {
        u32 ins = src[i];
        u64 pc = pc0 + (u64)i * 4;
        if (i < 3 && is_terminator(ins)) return -1;            /* fungsi < 16 byte */
        if ((ins & 0x9F000000) == 0x90000000) {                /* ADRP */
            u32 rd = ins & 31;
            i64 imm = sext(((ins >> 29) & 3) | (((u64)(ins >> 5) & 0x7FFFF) << 2), 21) * 4096;
            u64 v = (pc & ~0xFFFULL) + imm;
            EMIT(0x58000040 | rd); EMIT(0x14000003); EMIT64(v);
        } else if ((ins & 0x9F000000) == 0x10000000) {         /* ADR */
            u32 rd = ins & 31;
            u64 v = pc + sext(((ins >> 29) & 3) | (((u64)(ins >> 5) & 0x7FFFF) << 2), 21);
            EMIT(0x58000040 | rd); EMIT(0x14000003); EMIT64(v);
        } else if ((ins & 0xFC000000) == 0x14000000) {         /* B (index 3) */
            u64 v = pc + sext(ins & 0x3FFFFFF, 26) * 4;
            EMIT(0x58000051); EMIT(0xD65F0220); EMIT64(v);
            return n;                                          /* tidak perlu lompat balik */
        } else if ((ins & 0xFC000000) == 0x94000000) {         /* BL */
            u64 v = pc + sext(ins & 0x3FFFFFF, 26) * 4;
            EMIT(0x100000BE);                                  /* ADR X30, #20 */
            EMIT(0x58000051); EMIT(0xD65F0220); EMIT64(v);
        } else if ((ins & 0xBF000000) == 0x18000000 ||         /* LDR Wt/Xt, literal */
                   (ins & 0xFF000000) == 0x98000000) {         /* LDRSW literal */
            u32 rt = ins & 31;
            u64 v = pc + sext((ins >> 5) & 0x7FFFF, 19) * 4;
            u32 ld;
            if ((ins & 0xFF000000) == 0x98000000) ld = 0xB9800220;   /* LDRSW Xt,[X17] */
            else if (ins & 0x40000000) ld = 0xF9400220;              /* LDR Xt,[X17] */
            else ld = 0xB9400220;                                    /* LDR Wt,[X17] */
            EMIT(0x58000071); EMIT(ld | rt); EMIT(0x14000003); EMIT64(v);
        } else if ((ins & 0x7E000000) == 0x34000000 ||         /* CBZ/CBNZ */
                   (ins & 0xFF000010) == 0x54000000 ||         /* B.cond */
                   (ins & 0x7E000000) == 0x36000000) {         /* TBZ/TBNZ */
            u64 v;
            u32 head;
            if ((ins & 0x7E000000) == 0x36000000) {
                v = pc + sext((ins >> 5) & 0x3FFF, 14) * 4;
                head = (ins & 0xFFF8001F) | (2u << 5);
            } else {
                v = pc + sext((ins >> 5) & 0x7FFFF, 19) * 4;
                head = (ins & 0xFF00001F) | (2u << 5);
            }
            EMIT(head);                 /* kondisi benar -> +8 */
            EMIT(0x14000005);           /* kondisi salah -> lanjut (+20) */
            EMIT(0x58000051); EMIT(0xD65F0220); EMIT64(v);
        } else if ((ins & 0x3B000000) == 0x18000000) {         /* LDR SIMD literal */
            return -1;
        } else {
            EMIT(ins);
        }
    }
    EMIT(0x58000051); EMIT(0xD65F0220); EMIT64(pc0 + 16);       /* balik ke target+16 */
    return n;
}

/* Patch 16 byte: LDR X17,#8 ; RET X17 ; .quad hook  (RET tidak dicek BTI) */
static void make_jump(u32 *p, u64 hook) {
    p[0] = 0x58000051; p[1] = 0xD65F0220;
    p[2] = (u32)(hook & 0xffffffffULL); p[3] = (u32)(hook >> 32);
}

/* Ikuti tail-call "B" di awal fungsi (wrapper tipis). */
static const u32 *follow_b(const u32 *p) {
    for (int k = 0; k < 4 && (p[0] & 0xFC000000) == 0x14000000; k++)
        p = (const u32 *)((u64)p + sext(p[0] & 0x3FFFFFF, 26) * 4);
    return p;
}

/* Landing pad BTI (bti / bti c / bti j / bti jc) */
static int is_bti(u32 ins) { return (ins & 0xFFFFFF3F) == 0xD503241F; }
/* PACIASP / PACIBSP */
static int is_pac(u32 ins) { return ins == 0xD503233F || ins == 0xD503237F; }
#define BTI_C 0xD503245F
