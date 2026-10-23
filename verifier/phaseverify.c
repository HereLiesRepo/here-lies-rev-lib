/* phaseverify -- Clifford+T circuits (.qc, the format of the T-count benchmark suites): cost and exact equivalence.
 *   phaseverify stat FILE.qc            qubits, gates, T-count, CNOT count, H count
 *   phaseverify equiv A.qc B.qc         EQUAL when the two unitaries agree up to one global phase, NOT_EQUAL otherwise
 *   phaseverify expand [--drop-t K] F   the circuit with every Toffoli and CCZ written as Clifford+T (7 T each); --drop-t
 *                                       omits the K-th T written, a planted cheater
 *   phaseverify selftest                the planted-cheater controls (exit 0 only when every one holds)
 *   phaseverify sample K A.qc B.qc      K columns only (up to 26 qubits): NOT_EQUAL is a proof, AGREES_ON_SAMPLE is not
 * Exactness: every amplitude is x / sqrt(2)^k with x in Z[w], w = e^(i pi/4), held as four 128-bit integers (coefficients
 * of 1, w, w^2, w^3) and one shared exponent k per state vector. H is (a +- b)/sqrt(2); T, S, Z multiply by w, w^2, w^4;
 * X and controlled gates permute. No floating point reaches a verdict. Equality up to global phase is a cross-product
 * test: U(x,y) * V(r) == V(x,y) * U(r) for one fixed reference entry r, over every entry of every column (both
 * circuits unitary, so the common ratio has modulus 1). Integer overflow is never wrapped: it ends the run REFUSED.
 * Width: every basis column is simulated, 2^n columns of 2^n entries; above MAXQ qubits the answer is REFUSED (too
 * wide), never a guess. C11, no dependencies. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MAXQ 22
#define MAXQS 26                                   /* sample mode: one column at a time, wider allowed */
#define MAXV 1024
#define MAXA 64
enum { G_H, G_X, G_Y, G_Z, G_S, G_SD, G_T, G_TD, G_TOF, G_ZC, G_SWAP };
typedef struct { int kind, na, a[MAXA]; } Gate;
typedef struct { int n; char name[MAXV][32]; Gate *g; int ng, cap; int has_i; size_t inmask; unsigned char isin[MAXV]; } Circ;   /* inmask: the first 62 inputs, for column masks; isin: all */
typedef __int128 Int;                              /* 128-bit coefficients (64-bit ones overflowed on grover_5) */
typedef struct { Int c[4]; } Zw;                  /* c0 + c1 w + c2 w^2 + c3 w^3 */

static int refused;                                /* overflow anywhere: the run ends REFUSED */
static long align_steps;                           /* comparisons whose two sides carried different sqrt2 powers */
static void die(const char *m) { fprintf(stderr, "phaseverify: %s\n", m); exit(2); }

static Int add(Int a, Int b) { Int r; if (__builtin_add_overflow(a, b, &r)) refused = 1; return r; }
static Int sub(Int a, Int b) { Int r; if (__builtin_sub_overflow(a, b, &r)) refused = 1; return r; }
static Int mul(Int a, Int b) { Int r; if (__builtin_mul_overflow(a, b, &r)) refused = 1; return r; }
/* multiply by w^j (j mod 8): a rotation of the coefficients with sign flips, since w^4 = -1 */
static Zw rotw(Zw x, int j) {
    j &= 7; Zw r = x;
    for (int s = 0; s < j; ++s) { Zw t; t.c[0] = sub(0, r.c[3]); t.c[1] = r.c[0]; t.c[2] = r.c[1]; t.c[3] = r.c[2]; r = t; }   /* checked negation */
    return r;
}
static Zw zmul(Zw a, Zw b) {
    Zw r = {{0, 0, 0, 0}};
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) {
        Int p = mul(a.c[i], b.c[j]); int k = i + j;
        if (k < 4) r.c[k] = add(r.c[k], p); else r.c[k - 4] = sub(r.c[k - 4], p);
    }
    return r;
}
/* times sqrt(2) = w - w^3: (p,q,r,s) -> (q-s, p+r, q+s, r-p) */
static Zw rt2(Zw x) { Zw y; y.c[0] = sub(x.c[1], x.c[3]); y.c[1] = add(x.c[0], x.c[2]); y.c[2] = add(x.c[1], x.c[3]); y.c[3] = sub(x.c[2], x.c[0]); return y; }
static int zeq(Zw a, Zw b) { return !memcmp(a.c, b.c, sizeof a.c); }
static int zzero(Zw a) { return !a.c[0] && !a.c[1] && !a.c[2] && !a.c[3]; }

/* ---------------- parser: .v names, optional .i/.o, BEGIN ... END, one gate per line ---------------- */
static int qubit(Circ *C, const char *nm) {
    for (int i = 0; i < C->n; ++i) if (!strcmp(C->name[i], nm)) return i;
    return -1;
}
static void push(Circ *C, Gate g) {
    if (C->ng == C->cap) { C->cap = C->cap ? 2 * C->cap : 256; C->g = realloc(C->g, sizeof(Gate) * (size_t)C->cap); if (!C->g) die("out of memory"); }
    C->g[C->ng++] = g;
}
static int parse(const char *path, Circ *C) {
    FILE *f = fopen(path, "r"); if (!f) { fprintf(stderr, "phaseverify: cannot open %s\n", path); return 0; }
    memset(C, 0, sizeof *C);
    char line[1 << 16]; int body = 0, lno = 0;
    while (fgets(line, sizeof line, f)) {
        ++lno; char *h = strchr(line, '#'); if (h) *h = 0;
        static char *tok[MAXV + 2]; int nt = 0;
        for (char *p = strtok(line, " \t\r\n,"); p && nt < MAXV + 2; p = strtok(NULL, " \t\r\n,")) tok[nt++] = p;
        if (nt > MAXA + 1 && tok[0][0] != '.') { fprintf(stderr, "phaseverify: %s:%d: gate wider than %d operands\n", path, lno, MAXA); fclose(f); return 0; }
        if (!nt) continue;
        if (!strcmp(tok[0], ".v")) { for (int i = 1; i < nt; ++i) { if (C->n >= MAXV) die("too many qubits declared"); snprintf(C->name[C->n++], 32, "%s", tok[i]); } continue; }
        if (!strcmp(tok[0], ".i")) { C->has_i = 1; for (int i = 1; i < nt; ++i) { int q = qubit(C, tok[i]);
                if (q < 0) { fprintf(stderr, "phaseverify: %s:%d: .i names unknown qubit '%s'\n", path, lno, tok[i]); fclose(f); return 0; }
                C->isin[q] = 1; if (q < 62) C->inmask |= (size_t)1 << q; } continue; }
        if (tok[0][0] == '.') continue;                          /* .o and the rest: no effect on the unitary */
        if (!strcmp(tok[0], "BEGIN")) { body = 1; continue; }
        if (!strcmp(tok[0], "END")) { body = 0; continue; }
        if (!body) continue;
        Gate g; memset(&g, 0, sizeof g); const char *k = tok[0];
        if (!strcmp(k, "H")) g.kind = G_H; else if (!strcmp(k, "X")) g.kind = G_X; else if (!strcmp(k, "Y")) g.kind = G_Y;
        else if (!strcmp(k, "Z") || !strcmp(k, "Zd")) g.kind = G_ZC;   /* Zd: Z dagger, the same operator */ else if (!strcmp(k, "S") || !strcmp(k, "P")) g.kind = G_S;
        else if (!strcmp(k, "S*") || !strcmp(k, "P*")) g.kind = G_SD; else if (!strcmp(k, "T")) g.kind = G_T;
        else if (!strcmp(k, "T*")) g.kind = G_TD; else if (!strcmp(k, "tof") || !strcmp(k, "cnot") || !strcmp(k, "CNOT")) g.kind = G_TOF;
        else if (!strcmp(k, "swap")) g.kind = G_SWAP;
        else { fprintf(stderr, "phaseverify: %s:%d: gate '%s' not supported (refusing rather than guessing)\n", path, lno, k); fclose(f); return 0; }
        for (int i = 1; i < nt; ++i) {
            int q = qubit(C, tok[i]);
            if (q < 0) { fprintf(stderr, "phaseverify: %s:%d: unknown qubit '%s'\n", path, lno, tok[i]); fclose(f); return 0; }
            int dup = 0; for (int j = 0; j < g.na; ++j) if (g.a[j] == q) dup = 1;
            if (dup && g.kind == G_ZC) continue;     /* Z a b a = Z a b: (-1)^(a b a) = (-1)^(a b) */
            if (dup) { fprintf(stderr, "phaseverify: %s:%d: qubit '%s' used twice\n", path, lno, tok[i]); fclose(f); return 0; }
            g.a[g.na++] = q;
        }
        int need1 = g.kind == G_H || g.kind == G_X || g.kind == G_Y || g.kind == G_S || g.kind == G_SD || g.kind == G_T || g.kind == G_TD;
        if (!g.na || (need1 && g.na != 1) || (g.kind == G_SWAP && g.na != 2)) { fprintf(stderr, "phaseverify: %s:%d: wrong operand count for '%s'\n", path, lno, k); fclose(f); return 0; }
        if (g.kind == G_ZC && g.na == 1) g.kind = G_Z;
        push(C, g);
    }
    fclose(f);
    if (!C->n) { fprintf(stderr, "phaseverify: %s: no .v line\n", path); return 0; }
    return 1;
}

/* ---------------- simulator: one state vector, entries x / sqrt(2)^k ---------------- */
typedef struct { Zw *v; int k; size_t N; } State;
/* sqrt2 divides x exactly when c0 + c2 and c1 + c3 are both even: read from the low bits, never from the sums
 * (which can overflow 128 bits) */
static int rt2_divides(const Zw *x) { return !((x->c[0] ^ x->c[2]) & 1) && !((x->c[1] ^ x->c[3]) & 1); }
static void reduce(State *s) {                             /* divide everything by sqrt(2) while every entry allows it */
    while (s->k > 0) {
        for (size_t i = 0; i < s->N; ++i) { if (!rt2_divides(&s->v[i])) return; }
        for (size_t i = 0; i < s->N; ++i) { Zw x = s->v[i], y;
            y.c[0] = sub(x.c[1], x.c[3]) / 2; y.c[1] = add(x.c[0], x.c[2]) / 2; y.c[2] = add(x.c[1], x.c[3]) / 2; y.c[3] = sub(x.c[2], x.c[0]) / 2;
            s->v[i] = y; }   /* inverse of rt2: q=(a+c)/2, s=(c-a)/2, p=(b-d)/2, r=(b+d)/2, reordered as (p,q,r,s) */
        s->k--;
    }
}
static void apply(const Gate *g, State *s) {
    size_t N = s->N;
    if (g->kind == G_H) {
        size_t b = (size_t)1 << g->a[0];
        for (size_t i = 0; i < N; ++i) if (!(i & b)) { Zw x = s->v[i], y = s->v[i | b], p, m;
            for (int t = 0; t < 4; ++t) { p.c[t] = add(x.c[t], y.c[t]); m.c[t] = sub(x.c[t], y.c[t]); }
            s->v[i] = p; s->v[i | b] = m; }
        s->k++; reduce(s); return;
    }
    if (g->kind == G_TOF || g->kind == G_X || g->kind == G_Y) {           /* last operand is the target */
        size_t cm = 0; for (int j = 0; j < g->na - 1; ++j) cm |= (size_t)1 << g->a[j];
        size_t b = (size_t)1 << g->a[g->na - 1];
        for (size_t i = 0; i < N; ++i) if ((i & cm) == cm && !(i & b)) {
            Zw x = s->v[i], y = s->v[i | b];
            if (g->kind == G_Y) { s->v[i] = rotw(y, 6); s->v[i | b] = rotw(x, 2); }   /* Y = [[0,-i],[i,0]] */
            else { s->v[i] = y; s->v[i | b] = x; }
        }
        return;
    }
    if (g->kind == G_SWAP) {
        size_t a = (size_t)1 << g->a[0], b = (size_t)1 << g->a[1];
        for (size_t i = 0; i < N; ++i) if ((i & a) && !(i & b)) { size_t j = (i & ~a) | b; Zw t = s->v[i]; s->v[i] = s->v[j]; s->v[j] = t; }
        return;
    }
    int rot = g->kind == G_T ? 1 : g->kind == G_TD ? 7 : g->kind == G_S ? 2 : g->kind == G_SD ? 6 : 4;  /* Z, multi-Z: w^4 */
    size_t m = 0; for (int j = 0; j < g->na; ++j) m |= (size_t)1 << g->a[j];
    for (size_t i = 0; i < N; ++i) if ((i & m) == m) s->v[i] = rotw(s->v[i], rot);
}
static void run(const Circ *C, size_t col, State *s) {
    memset(s->v, 0, sizeof(Zw) * s->N); s->k = 0; s->v[col].c[0] = 1;
    for (int g = 0; g < C->ng && !refused; ++g) apply(&C->g[g], s);
}

/* ---------------- equivalence ---------------- */
enum { EQUAL, NOT_EQUAL, REFUSED };
static const char *verdict_name[] = { "EQUAL", "NOT_EQUAL", "REFUSED" };
/* both qubit lists must name the same qubits in the same order: the unitary is indexed by position */
static int zero_ancillas;
static int sample_k;        /* sample K: only K columns (column 0 and K-1 pseudo-random ones); agreement there proves nothing */   /* --zero-ancillas: judge only inputs whose non-.i qubits start at 0 (both files must agree on .i) */
static int equiv(const Circ *A, const Circ *B, char *why, size_t wl) {
    refused = 0;
    if (A->n != B->n) { snprintf(why, wl, "qubit counts differ (%d vs %d)", A->n, B->n); return NOT_EQUAL; }
    for (int i = 0; i < A->n; ++i) if (strcmp(A->name[i], B->name[i])) { snprintf(why, wl, "qubit %d is '%s' in one and '%s' in the other", i, A->name[i], B->name[i]); return REFUSED; }
    if (zero_ancillas && (A->has_i != B->has_i || memcmp(A->isin, B->isin, (size_t)A->n))) { snprintf(why, wl, "--zero-ancillas: the two files declare different inputs (.i)"); return REFUSED; }
    if (A->n > (sample_k ? MAXQS : MAXQ)) { snprintf(why, wl, "%d qubits: above the cap of %d", A->n, sample_k ? MAXQS : MAXQ); return REFUSED; }
    size_t N = (size_t)1 << A->n;
    State a = { calloc(N, sizeof(Zw)), 0, N }, b = { calloc(N, sizeof(Zw)), 0, N };
    if (!a.v || !b.v) die("out of memory");
    int have_ref = 0; Zw ua = {{0}}, vb = {{0}}; int ka = 0, kb = 0;     /* reference entry U(r), V(r) and their exponents */
    int out = EQUAL;
    size_t judged = 0;
    uint64_t srng = 0x2545F4914F6CDD1DULL;
    for (size_t ci = 0; ci < (sample_k ? (size_t)sample_k : N) && out == EQUAL; ++ci) {
        size_t col = ci;
        if (sample_k) { srng ^= srng << 13; srng ^= srng >> 7; srng ^= srng << 17; col = ci == 0 ? 0 : (size_t)(srng % N); if (zero_ancillas && A->has_i) col &= A->inmask; }
        if (zero_ancillas && A->has_i && (col & ~A->inmask)) continue;
        ++judged;
        run(A, col, &a); run(B, col, &b);
        if (refused) { snprintf(why, wl, "integer overflow while simulating column %zu", col); out = REFUSED; break; }
        for (size_t i = 0; i < N; ++i) {
            Zw x = a.v[i], y = b.v[i];
            if (!have_ref) {
                if (zzero(x) != zzero(y)) { snprintf(why, wl, "column %zu row %zu: one is zero, the other is not", col, i); out = NOT_EQUAL; break; }
                if (!zzero(x)) { ua = x; vb = y; ka = a.k; kb = b.k; have_ref = 1; }
                continue;
            }
            /* U(x)/2^(a.k/2) * V(r)/2^(kb/2) == V(x)/2^(b.k/2) * U(r)/2^(ka/2): bring both sides to one sqrt(2) power */
            Zw l = zmul(x, vb), r = zmul(y, ua);
            int el = a.k + kb, er = b.k + ka;
            if (el != er) ++align_steps;
#ifndef PHASEVERIFY_BREAK_ALIGN   /* a planted break: compare without aligning the sqrt2 powers (see the note at selftest) */
            while (el < er && !refused) { l = rt2(l); ++el; }
            while (er < el && !refused) { r = rt2(r); ++er; }
#endif
            if (refused) { snprintf(why, wl, "integer overflow comparing column %zu", col); out = REFUSED; break; }
            if (!zeq(l, r)) { snprintf(why, wl, "column %zu row %zu: the amplitudes differ by more than a global phase", col, i); out = NOT_EQUAL; break; }
        }
    }
    if (out == EQUAL) snprintf(why, wl, "all %zu judged columns of %zu agree up to one global phase%s", judged, N, judged < N ? " (non-input qubits start at 0)" : "");
    free(a.v); free(b.v);
    return out;
}

static void stat(const Circ *C) {
    int t = 0, cx = 0, h = 0, three = 0, wider = 0;
    for (int g = 0; g < C->ng; ++g) { const Gate *G = &C->g[g];
        if (G->kind == G_T || G->kind == G_TD) ++t; else if (G->kind == G_H) ++h;
        else if (G->kind == G_TOF && G->na == 2) ++cx;
        else if ((G->kind == G_TOF || G->kind == G_ZC) && G->na == 3) ++three;
        else if ((G->kind == G_TOF || G->kind == G_ZC) && G->na > 3) ++wider; }
    printf("qubits=%d gates=%d T_explicit=%d toffoli_or_ccz=%d T_count=%d CNOT=%d H=%d%s\n", C->n, C->ng, t, three, t + 7 * three, cx, h,
           wider ? " (wider gates present: not counted)" : "");
}

/* ---------------- expand: Toffoli and CCZ into Clifford+T, 7 T gates each ----------------
 * CCZ(a,b,c) is the Nielsen-Chuang Toffoli core without its two H gates on the target; Toffoli is H c, CCZ, H c. The
 * selftest proves the 3-qubit identity; `equiv` on a whole benchmark against its expansion proves it in place.
 * --drop-t K omits the K-th T or T* written (1-based): a planted cheater that must come back NOT_EQUAL. */
static int t_written, drop_k;
static void emit_t(const char *g, const char *q) { ++t_written; if (t_written == drop_k) return; printf("%s %s\n", g, q); }
static void emit_ccz(const char *a, const char *b, const char *c) {
    printf("tof %s %s\n", b, c); emit_t("T*", c); printf("tof %s %s\n", a, c); emit_t("T", c);
    printf("tof %s %s\n", b, c); emit_t("T*", c); printf("tof %s %s\n", a, c); emit_t("T", b); emit_t("T", c);
    printf("tof %s %s\n", a, b); emit_t("T", a); emit_t("T*", b); printf("tof %s %s\n", a, b);
}
static void expand(const Circ *C) {
    static const char *nm[] = { "H", "X", "Y", "Z", "S", "S*", "T", "T*", "tof", "Z", "swap" };
    printf(".v"); for (int i = 0; i < C->n; ++i) printf(" %s", C->name[i]); printf("\n");
    if (C->has_i) { printf(".i"); for (int i = 0; i < C->n; ++i) if (C->isin[i]) printf(" %s", C->name[i]); printf("\n"); }   /* every declared input, past 62 too */
    printf("BEGIN\n");
    for (int g = 0; g < C->ng; ++g) { const Gate *G = &C->g[g];
        if ((G->kind == G_TOF || G->kind == G_ZC) && G->na == 3) {
            const char *a = C->name[G->a[0]], *b = C->name[G->a[1]], *c = C->name[G->a[2]];
            if (G->kind == G_TOF) printf("H %s\n", c);
            emit_ccz(a, b, c);
            if (G->kind == G_TOF) printf("H %s\n", c);
            continue;
        }
        if (G->kind == G_T || G->kind == G_TD) { emit_t(G->kind == G_T ? "T" : "T*", C->name[G->a[0]]); continue; }
        printf("%s", nm[G->kind]); for (int j = 0; j < G->na; ++j) printf(" %s", C->name[G->a[j]]); printf("\n");
    }
    printf("END\n");
}

/* ---------------- selftest: every control must give its expected verdict ---------------- */
static int write_tmp(char *path, const char *body) {
    snprintf(path, 64, "/tmp/phaseverify_XXXXXX"); int fd = mkstemp(path); if (fd < 0) return 0;
    FILE *f = fdopen(fd, "w"); fputs(body, f); fclose(f); return 1;
}
static int check(const char *name, const char *a, const char *b, int want) {
    char pa[64], pb[64], why[256]; Circ A, B;
    if (!write_tmp(pa, a) || !write_tmp(pb, b)) die("cannot write a temporary file");
    int ok = parse(pa, &A) && parse(pb, &B);
    int got = ok ? equiv(&A, &B, why, sizeof why) : REFUSED;
    if (!ok) snprintf(why, sizeof why, "parse failed");
    int pass = got == want;
    printf("%-34s want %-9s got %-9s %s  (%s)\n", name, verdict_name[want], verdict_name[got], pass ? "ok" : "FAIL", why);
    remove(pa); remove(pb);
    if (ok) { free(A.g); free(B.g); }
    return pass;
}
#define HDR3 ".v a b c\nBEGIN\n"
/* Toffoli (a, b controls; c target) in Clifford+T, 7 T gates (Nielsen and Chuang, Fig. 4.9) */
#define TOF7 "H c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT* c\ntof a c\nT b\nT c\nH c\ntof a b\nT a\nT* b\ntof a b\n"
static int selftest(void) {
    int all = 1;
    all &= check("identity: H H = nothing", HDR3 "H a\nH a\nEND\n", HDR3 "END\n", EQUAL);
    all &= check("T T = S", HDR3 "T a\nT a\nEND\n", HDR3 "S a\nEND\n", EQUAL);
    all &= check("S S = Z", HDR3 "S a\nS a\nEND\n", HDR3 "Z a\nEND\n", EQUAL);
    all &= check("HZH = X", HDR3 "H a\nZ a\nH a\nEND\n", HDR3 "X a\nEND\n", EQUAL);
    all &= check("global phase: XZXZ = -I", HDR3 "X a\nZ a\nX a\nZ a\nEND\n", HDR3 "END\n", EQUAL);
    all &= check("Toffoli = its 7-T circuit", HDR3 "tof a b c\nEND\n", HDR3 TOF7 "END\n", EQUAL);
    all &= check("CCZ = H tof H", HDR3 "Z a b c\nEND\n", HDR3 "H c\ntof a b c\nH c\nEND\n", EQUAL);
    all &= check("Y = i X Z", HDR3 "Y a\nEND\n", HDR3 "Z a\nX a\nEND\n", EQUAL);
    /* planted cheaters: each must be caught */
    all &= check("CHEAT relative phase: Z is not I", HDR3 "Z a\nEND\n", HDR3 "END\n", NOT_EQUAL);
    all &= check("CHEAT T is not S", HDR3 "T a\nEND\n", HDR3 "S a\nEND\n", NOT_EQUAL);
    all &= check("CHEAT T is not T*", HDR3 "T a\nEND\n", HDR3 "T* a\nEND\n", NOT_EQUAL);
    all &= check("CHEAT dropped T in Toffoli", HDR3 "tof a b c\nEND\n", HDR3 "H c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT* c\ntof a c\nT b\nH c\ntof a b\nT a\nT* b\ntof a b\nEND\n", NOT_EQUAL);
    all &= check("CHEAT T flipped to T* in Toffoli", HDR3 "tof a b c\nEND\n", HDR3 "H c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT c\ntof a c\nT b\nT c\nH c\ntof a b\nT a\nT* b\ntof a b\nEND\n", NOT_EQUAL);
    all &= check("CHEAT wrong target", HDR3 "tof a b c\nEND\n", HDR3 "tof a c b\nEND\n", NOT_EQUAL);
    all &= check("CHEAT dropped control", HDR3 "tof a b c\nEND\n", HDR3 "tof b c\nEND\n", NOT_EQUAL);
    /* The sqrt2-alignment path. Columns of one circuit can reduce to different powers of sqrt2: here column a=0 is
     * exact (k 0) and column a=1 carries 1/sqrt2 (k 1), so comparing against the identity reaches the alignment code;
     * the control asserts that it was reached. No control can make dropping the alignment change a verdict, and none
     * is claimed: reduce() leaves each column in lowest terms, so a skipped alignment can only accept a pair whose
     * numerators are proportional by one constant lambda across every column while the two circuits' power gaps dk
     * differ between columns; both being unitary, every column has norm 1, which forces |lambda| = sqrt2^dk for every
     * column, so dk is constant and nothing was skipped. Building with -DPHASEVERIFY_BREAK_ALIGN still passes the
     * selftest for that reason (measured), and the path stays covered by this control. */
    { long before = align_steps;
      int ok1 = check("CHEAT column-dependent sqrt2 powers", HDR3 "H b\nT b\ntof a b\nT* b\ntof a b\nH b\nEND\n", HDR3 "END\n", NOT_EQUAL);
      int reached = align_steps > before;
      printf("%-34s %s  (alignment comparisons made: %ld)\n", "  ...the alignment path was reached", reached ? "ok" : "FAIL", align_steps - before);
      all &= ok1 && reached; }
    all &= check("CHEAT relative-phase Toffoli", HDR3 "tof a b c\nEND\n", HDR3 "H c\nT c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT* c\nH c\nEND\n", NOT_EQUAL);
    /* checked arithmetic at the 128-bit edge (REPAIR-PHASEVERIFY-CHECKED-ARITHMETIC): negating the most negative
     * coefficient must refuse, not overflow; divisibility by sqrt2 must read the right parity when c0 + c2 overflows */
    { Int mn = (Int)((unsigned __int128)1 << 127), mx = ~mn;          /* INT128_MIN, INT128_MAX */
      Zw x = {{0, 0, 0, mn}}; refused = 0; (void)rotw(x, 1); int r1 = refused; refused = 0;
      Zw y = {{5, 0, 0, 3}}; Zw ry = rotw(y, 1); int r2 = !refused && ry.c[0] == -3 && ry.c[1] == 5;
      Zw e = {{mx, 0, 1, 0}}, o = {{mx, 0, 2, 0}};                       /* mx + 1 overflows: even; mx + 2 overflows: odd */
      int r3 = rt2_divides(&e) && !rt2_divides(&o);
      int pass = r1 && r2 && r3; all &= pass;
      printf("%-34s %s  (rotw(INT128_MIN) refused %d, ordinary rotation exact %d, overflowing parities right %d)\n", "checked 128-bit edge", pass ? "ok" : "FAIL", r1, r2, r3); }
    printf(all ? "selftest: every control holds\n" : "selftest: FAILED\n");
    return all ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--zero-ancillas")) { zero_ancillas = 1; ++argv; --argc; }
    if (argc == 2 && !strcmp(argv[1], "selftest")) return selftest();
    if (argc >= 3 && !strcmp(argv[1], "expand")) { Circ C; if (argc == 5 && !strcmp(argv[2], "--drop-t")) { drop_k = atoi(argv[3]); argv += 2; }
        if (!parse(argv[2], &C)) return 2;
        expand(&C);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "stat")) { Circ C; if (!parse(argv[2], &C)) return 2; stat(&C); return 0; }
    if (argc == 5 && !strcmp(argv[1], "sample")) {
        sample_k = atoi(argv[2]); if (sample_k < 1) sample_k = 1;
        Circ A, B; if (!parse(argv[3], &A) || !parse(argv[4], &B)) { printf("REFUSED (parse)\n"); return 2; }
        char why[256]; int v = equiv(&A, &B, why, sizeof why);
        if (v == EQUAL) printf("AGREES_ON_SAMPLE (%d columns agree up to one global phase; a sample, not a proof)\n", sample_k);
        else printf("%s (%s)\n", verdict_name[v], why);
        return v == EQUAL ? 4 : v == NOT_EQUAL ? 1 : 2;
    }
    if (argc == 4 && !strcmp(argv[1], "equiv")) {
        Circ A, B; if (!parse(argv[2], &A) || !parse(argv[3], &B)) { printf("REFUSED (parse)\n"); return 2; }
        char why[256]; int v = equiv(&A, &B, why, sizeof why);
        printf("%s (%s)\n", verdict_name[v], why);
        return v == EQUAL ? 0 : v == NOT_EQUAL ? 1 : 2;
    }
    fprintf(stderr, "usage: phaseverify [--zero-ancillas] stat F.qc | equiv A.qc B.qc | sample K A.qc B.qc | expand [--drop-t K] F.qc | selftest\n");
    return 2;
}
