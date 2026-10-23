/* revverify -- RevLib .real circuits: cost and exact equivalence. No optimizer in here.
 *   revverify stat FILE.real            lines, gates, quantum cost (RevKit's table), gate kinds
 *   revverify equiv A.real B.real       every non-garbage output equal for every free-input assignment
 *   revverify miter A B OUT.cnf         a CNF that is UNSAT exactly when A and B agree (an independent encoder; solve
 *                                       it with any DIMACS solver, e.g. kissat, and check the proof with drat-trim)
 *   revverify qc2real FILE.qc           a .qc of X and tof gates only (target last) as a .real with every line an input
 *                                       and an output, no constants, no garbage: such a circuit has no phases, so equal
 *                                       permutations on every line is equality of unitaries. Any other gate is refused
 *                                       (exit 3), so a phase gate never passes as classical.
 *   revverify --intended equiv A B      the same, reading .garbage the way the file means it (see the parser); the
 *                                       reference A decides the reading for both, and a reading an EMPTY circuit
 *                                       would pass is never chosen
 *                                       (exhaustive up to 26 free inputs; above that 1,048,576 random assignments,
 *                                       printed as sampled: evidence, not proof)
 * Gates: t (multiple-control Toffoli), f (multiple-control Fredkin), p / pi (Peres), v / v+ (controlled sqrt(NOT),
 * simulated over Z4; a control left in a V state is unjudgeable, never equal).
 * Semantics: .constants fixes lines; a line is garbage only when .garbage marks it AND its .outputs name is g (some RevLib
 * files list .garbage reversed). Extracted verbatim from revopt (the parser, cost table and simulator). C11, no deps. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MAXL 8192
#define MAXC 64
enum { G_T, G_F, G_P, G_PI, G_V, G_VP };
static int g_intended;   /* --intended: see the parser */
typedef struct { int kind, nc, c[MAXC], t[2]; } Gate;
typedef struct {
    int n; char name[MAXL][32];
    char cons[MAXL], garb[MAXL];  /* '-' or '0'/'1'; '-' or '1' */
    char inname[MAXL][32], outname[MAXL][32]; int nin, nout;
    Gate *g; int ng, capg;
    int ncv;                       /* has v or v+ */
    char *head; size_t headlen;    /* the file up to and including .begin, re-emitted verbatim */
} Circ;

static void die(const char *m) { fprintf(stderr, "revopt: %s\n", m); exit(2); }
static int line_of(const Circ *C, const char *nm) { for (int i = 0; i < C->n; ++i) if (!strcmp(C->name[i], nm)) return i; return -1; }

static void apply(const Gate *G, uint64_t **st, int W);
/* A garbage reading is TRIVIAL when an empty circuit meets it: every compared line ends where it started, on 256 random
 * inputs (constants held). No benchmark function is the identity on its compared outputs, so --intended never picks a
 * trivial reading (alu_32_383: reversed, its 32 result lines would be garbage). Circuits with V gates: not simulated. */
static int garbage_trivial(const Circ *C, const char *garb) {
    if (C->ncv) return 0;
    enum { TW = 4 };
    uint64_t **st = malloc(sizeof(uint64_t *) * (size_t)C->n), *in = malloc(8 * (size_t)C->n * TW), x = 0x9E3779B97F4A7C15ULL;
    if (!st || !in) die("out of memory");
    for (int i = 0; i < C->n; ++i) {
        st[i] = malloc(8 * TW); if (!st[i]) die("out of memory");
        for (int w = 0; w < TW; ++w) { x ^= x << 13; x ^= x >> 7; x ^= x << 17;
            st[i][w] = in[(size_t)i * TW + w] = C->cons[i] == '0' ? 0 : C->cons[i] == '1' ? ~0ULL : x; }
    }
    for (int g = 0; g < C->ng; ++g) apply(&C->g[g], st, TW);
    int triv = 1;
    for (int i = 0; i < C->n && triv; ++i) if (garb[i] != '1') for (int w = 0; w < TW; ++w) if (st[i][w] != in[(size_t)i * TW + w]) { triv = 0; break; }
    for (int i = 0; i < C->n; ++i) free(st[i]);
    free(st); free(in);
    return triv;
}
static int parse(const char *path, Circ *C) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    memset(C, 0, sizeof(*C));
    static char ln[1 << 20]; int body = 0, indef = 0;
    while (fgets(ln, sizeof ln, f)) {
        char *s = ln; while (isspace((unsigned char)*s)) ++s;
        char *h = strchr(s, '#'); if (h) *h = 0;
        size_t L = strlen(s); while (L && isspace((unsigned char)s[L - 1])) s[--L] = 0;
        if (!*s) continue;
        if (!strncmp(s, ".define", 7)) { indef = 1; continue; }
        if (!strncmp(s, ".enddefine", 10)) { indef = 0; continue; }
        if (indef) continue;
        if (*s == '.') {
            char key[64]; int off = 0; sscanf(s, "%63s%n", key, &off); char *rest = s + off;
            if (!body) { /* the header is re-emitted verbatim: copy it before strtok cuts the line */
                size_t l2 = strlen(s);
                C->head = realloc(C->head, C->headlen + l2 + 2);
                memcpy(C->head + C->headlen, s, l2); C->headlen += l2; C->head[C->headlen++] = '\n'; C->head[C->headlen] = 0;
            }
            if (!strcmp(key, ".variables")) { char *tok = strtok(rest, " \t"); while (tok) { if (C->n >= MAXL) die("too many lines"); snprintf(C->name[C->n++], 32, "%s", tok); tok = strtok(NULL, " \t"); } }
            else if (!strcmp(key, ".inputs") || !strcmp(key, ".outputs")) {
                int isin = key[2] == 'n', k = 0; char *tok = strtok(rest, " \t");
                while (tok && k < MAXL) { snprintf(isin ? C->inname[k] : C->outname[k], 32, "%s", tok); ++k; tok = strtok(NULL, " \t"); }
                if (isin) C->nin = k; else C->nout = k;
            }
            else if (!strcmp(key, ".constants")) { while (isspace((unsigned char)*rest)) ++rest; for (int i = 0; i < C->n && rest[i]; ++i) C->cons[i] = rest[i]; }
            else if (!strcmp(key, ".garbage")) { while (isspace((unsigned char)*rest)) ++rest; for (int i = 0; i < C->n && rest[i]; ++i) C->garb[i] = rest[i]; }
            else if (!strcmp(key, ".begin")) body = 1;
            else if (!strcmp(key, ".end")) body = 0;
            continue;
        }
        if (!body) continue;
        char op[16]; int off = 0; sscanf(s, "%15s%n", op, &off); char *rest = s + off;
        Gate G; memset(&G, 0, sizeof G);
        int ops[MAXC + 2], no = 0;
        char *tok = strtok(rest, " \t");
        while (tok) { int l = line_of(C, tok); if (l < 0) { fclose(f); fprintf(stderr, "revopt: %s: unknown line %s\n", path, tok); return -1; } if (no >= MAXC + 2) die("gate too wide"); ops[no++] = l; tok = strtok(NULL, " \t"); }
        if ((op[0] == 't' || op[0] == 'v') && no - 1 > MAXC) die("gate with more controls than MAXC");
        if (op[0] == 't') { G.kind = G_T; G.nc = no - 1; for (int i = 0; i < no - 1; ++i) G.c[i] = ops[i]; G.t[0] = ops[no - 1]; }
        else if (op[0] == 'f') { G.kind = G_F; G.nc = no - 2; for (int i = 0; i < no - 2; ++i) G.c[i] = ops[i]; G.t[0] = ops[no - 2]; G.t[1] = ops[no - 1]; }
        else if (!strcmp(op, "p") || !strcmp(op, "p3")) { G.kind = G_P; G.nc = 2; G.c[0] = ops[0]; G.c[1] = ops[1]; G.t[0] = ops[2]; }
        else if (!strcmp(op, "pi") || !strcmp(op, "pi3")) { G.kind = G_PI; G.nc = 2; G.c[0] = ops[0]; G.c[1] = ops[1]; G.t[0] = ops[2]; }
        else if (!strncmp(op, "v+", 2)) { G.kind = G_VP; G.nc = no - 1; for (int i = 0; i < no - 1; ++i) G.c[i] = ops[i]; G.t[0] = ops[no - 1]; C->ncv = 1; }
        else if (op[0] == 'v') { G.kind = G_V; G.nc = no - 1; for (int i = 0; i < no - 1; ++i) G.c[i] = ops[i]; G.t[0] = ops[no - 1]; C->ncv = 1; }
        else { fclose(f); fprintf(stderr, "revopt: %s: unknown gate %s\n", path, op); return -1; }
        if (C->ng == C->capg) { C->capg = C->capg ? 2 * C->capg : 256; C->g = realloc(C->g, sizeof(Gate) * (size_t)C->capg); }
        C->g[C->ng++] = G;
    }
    fclose(f);
    for (int i = 0; i < C->n; ++i) { if (!C->cons[i]) C->cons[i] = '-'; if (!C->garb[i]) C->garb[i] = '-'; }
    /* Some RevLib files list .garbage in the reverse order (9symml_195, C17_204 mark the function outputs f0, f1 as
     * garbage and the pass-through inputs as kept). A line is garbage only when the string marks it AND its .outputs name
     * is "g"; anything else is compared. .constants stays the authority (SyReC files name constant lines like variables,
     * and the reversed-garbage files carry consistent constants); a line only *named* 0 or 1 stays free, the safe side. */
    if (g_intended) {
        /* --intended: read .garbage the way the file means it. Output names "g" decide when the file uses them;
         * otherwise a string that marks more constant lines (which exist to carry outputs) than its reverse does is
         * reversed, and every marked line is garbage. */
        int named = 0;
        for (int i = 0; i < C->n && i < C->nout; ++i) if (!strcmp(C->outname[i], "g")) named = 1;
        if (named) {
            /* a g-named line is garbage only when the string marks it; the orientation that marks more g-named lines
             * is the file's meaning. Marking every g-named line regardless of the string made a real output named g
             * garbage (plus63mod4096_163, ham7_299 ...), so a wrong circuit passed. No marks: no garbage. */
            int as_given = 0, reversed = 0;
            for (int i = 0; i < C->n && i < C->nout; ++i) if (!strcmp(C->outname[i], "g")) { if (C->garb[i] == '1') ++as_given; if (C->garb[C->n - 1 - i] == '1') ++reversed; }
            static char t[MAXL]; for (int i = 0; i < C->n; ++i) t[i] = C->garb[C->n - 1 - i];
            const char *src = reversed > as_given ? t : C->garb;
            static char u[MAXL]; for (int i = 0; i < C->n; ++i) u[i] = (i < C->nout && !strcmp(C->outname[i], "g") && src[i] == '1') ? '1' : '-';
            memcpy(C->garb, u, (size_t)C->n);
        }
        else {
            int as_given = 0, reversed = 0;
            for (int i = 0; i < C->n; ++i) { if (C->cons[i] != '-' && C->garb[i] == '1') ++as_given; if (C->cons[i] != '-' && C->garb[C->n - 1 - i] == '1') ++reversed; }
            static char t[MAXL]; for (int i = 0; i < C->n; ++i) t[i] = C->garb[C->n - 1 - i];
            /* never a trivial reading: when exactly one orientation is trivial, the other one decides */
            int tw = garbage_trivial(C, C->garb), tr = garbage_trivial(C, t), flip = reversed < as_given;
            if (tw != tr) flip = tw;
            if (flip) memcpy(C->garb, t, (size_t)C->n);
        }
    } else
    for (int i = 0; i < C->n; ++i) if (C->garb[i] == '1' && !(i < C->nout && !strcmp(C->outname[i], "g"))) C->garb[i] = '-';
    return 0;
}

/* RevKit's quantum cost of a multiple-control Toffoli with c controls on a circuit of n lines (e = n - c - 1 empty lines).
 * Every cost is exact or refused: returns 0 with *q set, or -1 when the cost does not fit in 64 bits. With no empty
 * line the cost is 2^(c+1) - 3: c = 63 gives 2^64 - 3 (the largest that fits, written without a 64-bit shift, which C
 * leaves undefined), c = 64 gives 2^65 - 3 and is refused, never wrapped or saturated into a valid-looking score
 * (REVLIB-COST-OVERFLOW-20261022). */
static int qc_mct(int c, int n, unsigned long long *q) {
    int e = n - c - 1;
    switch (c) {
    case 0: case 1: *q = 1; return 0;
    case 2: *q = 5; return 0;
    case 3: *q = 13; return 0;
    case 4: *q = e >= 2 ? 26 : 29; return 0;
    case 5: *q = e >= 3 ? 38 : e >= 1 ? 52 : 61; return 0;
    case 6: *q = e >= 4 ? 50 : e >= 1 ? 80 : 125; return 0;
    case 7: *q = e >= 5 ? 62 : e >= 1 ? 100 : 253; return 0;
    case 8: *q = e >= 6 ? 74 : e >= 1 ? 128 : 509; return 0;
    case 9: *q = e >= 7 ? 86 : e >= 1 ? 152 : 1021; return 0;
    default:
        if (c < 0) return -1;
        if (e >= c - 2) { *q = 12ULL * (unsigned long long)(c + 1) - 34; return 0; }
        if (e >= 1) { *q = 24ULL * (unsigned long long)(c + 1) - 88; return 0; }
        if (c <= 62) { *q = (1ULL << (c + 1)) - 3; return 0; }
        if (c == 63) { *q = 0xFFFFFFFFFFFFFFFFULL - 2; return 0; }   /* 2^64 - 3 */
        return -1;
    }
}
static int qc_gate(const Gate *G, int n, unsigned long long *q) {
    switch (G->kind) {
    case G_T: return qc_mct(G->nc, n, q);
    case G_F: { unsigned long long m; if (qc_mct(G->nc + 1, n, &m)) return -1; return __builtin_add_overflow(m, 2ULL, q) ? -1 : 0; }
    case G_P: case G_PI: *q = 4; return 0;
    default: *q = 1; return 0;
    }
}
static int qc(const Circ *C, unsigned long long *total) {
    unsigned long long s = 0, g;
    for (int i = 0; i < C->ng; ++i) { if (qc_gate(&C->g[i], C->n, &g) || __builtin_add_overflow(s, g, &s)) return -1; }
    *total = s; return 0;
}


/* ---- exact simulation: every assignment of the free (non-constant) input lines, bit-sliced, in chunks ---- */
#define CHUNK 1024 /* words per chunk */
static int nfree(const Circ *C) { int k = 0; for (int i = 0; i < C->n; ++i) k += C->cons[i] == '-'; return k; }
static void apply(const Gate *G, uint64_t **st, int W) {
    for (int w = 0; w < W; ++w) {
        uint64_t m = ~0ULL;
        for (int i = 0; i < G->nc; ++i) m &= st[G->c[i]][w];
        switch (G->kind) {
        case G_T: st[G->t[0]][w] ^= m; break;
        case G_F: { uint64_t d = (st[G->t[0]][w] ^ st[G->t[1]][w]) & m; st[G->t[0]][w] ^= d; st[G->t[1]][w] ^= d; } break;
        case G_P: st[G->t[0]][w] ^= st[G->c[0]][w] & st[G->c[1]][w]; st[G->c[1]][w] ^= st[G->c[0]][w]; break;
        case G_PI: st[G->c[1]][w] ^= st[G->c[0]][w]; st[G->t[0]][w] ^= st[G->c[0]][w] & st[G->c[1]][w]; break;
        default: break;
        }
    }
}
/* Loads chunk ci (words [ci*CHUNK, ...)) of the input assignments into st and runs the circuit; returns the word count. */
static int run_chunk(const Circ *C, uint64_t **st, long long ci, int k) {
    long long total = k >= 6 ? 1LL << (k - 6) : 1;
    long long w0 = ci * CHUNK; int W = (int)((total - w0) < CHUNK ? (total - w0) : CHUNK);
    static const uint64_t pat[6] = {0xAAAAAAAAAAAAAAAAULL, 0xCCCCCCCCCCCCCCCCULL, 0xF0F0F0F0F0F0F0F0ULL, 0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL, 0xFFFFFFFF00000000ULL};
    int j = 0;
    for (int i = 0; i < C->n; ++i) {
        for (int w = 0; w < W; ++w) {
            if (C->cons[i] == '0') st[i][w] = 0;
            else if (C->cons[i] == '1') st[i][w] = ~0ULL;
            else st[i][w] = j < 6 ? pat[j] : (((w0 + w) >> (j - 6)) & 1) ? ~0ULL : 0;
        }
        if (C->cons[i] == '-') ++j;
    }
    for (int g = 0; g < C->ng; ++g) apply(&C->g[g], st, W);
    return W;
}

/* ---- NCV (v, v+) simulation: each line is a value in Z4 held in two bit planes (hi, lo): classical 0 and 1 are 0 and 2,
 * NOT/CNOT/Toffoli add 2, v adds 1 and v+ subtracts 1 (V^2 = NOT). A control must hold a classical 1 (2); a control in a
 * V state (lo = 1) is ill-formed and makes the comparison unjudgeable rather than equal. ---- */
static void apply4(const Gate *G, uint64_t **hi, uint64_t **lo, int W, uint64_t *bad) {
    for (int w = 0; w < W; ++w) {
        uint64_t m = ~0ULL;
        for (int i = 0; i < G->nc; ++i) { m &= hi[G->c[i]][w] & ~lo[G->c[i]][w]; *bad |= lo[G->c[i]][w]; }
        int t = G->t[0];
        switch (G->kind) {
        case G_T: hi[t][w] ^= m; break;
        case G_F: { int u = G->t[1]; uint64_t d = (hi[t][w] ^ hi[u][w]) & m, e = (lo[t][w] ^ lo[u][w]) & m; hi[t][w] ^= d; hi[u][w] ^= d; lo[t][w] ^= e; lo[u][w] ^= e; } break;
        case G_P: { uint64_t m2 = hi[G->c[0]][w] & ~lo[G->c[0]][w]; hi[t][w] ^= m; hi[G->c[1]][w] ^= m2; } break;
        case G_PI: { int a = G->c[0], b = G->c[1]; uint64_t m2 = hi[a][w] & ~lo[a][w]; hi[b][w] ^= m2; uint64_t mm = m2 & hi[b][w] & ~lo[b][w]; *bad |= lo[b][w]; hi[t][w] ^= mm; } break;
        case G_V: { uint64_t c = lo[t][w] & m; lo[t][w] ^= m; hi[t][w] ^= c; } break;
        case G_VP: { uint64_t b = ~lo[t][w] & m; lo[t][w] ^= m; hi[t][w] ^= b; } break;
        }
    }
}
/* 1 equal, 0 differs, -1 unjudgeable (ill-formed control, or a compared output left in a V state) */
static int compare4(const Circ *A, uint64_t **ah, uint64_t **al, uint64_t **bh, uint64_t **bl, int W, uint64_t lanemask) {
    for (int i = 0; i < A->n; ++i) {
        if (A->garb[i] == '1') continue;
        for (int w = 0; w < W; ++w) {
            if ((al[i][w] | bl[i][w]) & lanemask) return -1;
            if ((ah[i][w] ^ bh[i][w]) & lanemask) return 0;
        }
    }
    return 1;
}
static int equivalent4(const Circ *A, const Circ *B, int maxk, int sampled_words) {
    int k = nfree(A);
    if (sampled_words == 0 && k > maxk) return -1;
    long long total = sampled_words ? sampled_words : (k >= 6 ? 1LL << (k - 6) : 1), chunks = (total + CHUNK - 1) / CHUNK;
    uint64_t lanemask = (sampled_words || k >= 6) ? ~0ULL : ((1ULL << (1 << k)) - 1);
    int n = A->n;
    uint64_t **ah = malloc(sizeof(uint64_t *) * (size_t)n), **al = malloc(sizeof(uint64_t *) * (size_t)n), **bh = malloc(sizeof(uint64_t *) * (size_t)n), **bl = malloc(sizeof(uint64_t *) * (size_t)n);
    for (int i = 0; i < n; ++i) { ah[i] = malloc(8 * CHUNK); al[i] = malloc(8 * CHUNK); bh[i] = malloc(8 * CHUNK); bl[i] = malloc(8 * CHUNK); }
    static const uint64_t pat[6] = {0xAAAAAAAAAAAAAAAAULL, 0xCCCCCCCCCCCCCCCCULL, 0xF0F0F0F0F0F0F0F0ULL, 0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL, 0xFFFFFFFF00000000ULL};
    uint64_t x = 0x9E3779B97F4A7C15ULL; int res = 1;
    for (long long ci = 0; res == 1 && ci < chunks; ++ci) {
        long long w0 = ci * CHUNK; int W = (int)((total - w0) < CHUNK ? (total - w0) : CHUNK), j = 0;
        for (int i = 0; i < n; ++i) {
            for (int w = 0; w < W; ++w) {
                uint64_t v;
                if (A->cons[i] == '0') v = 0; else if (A->cons[i] == '1') v = ~0ULL;
                else if (sampled_words) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; v = x; }
                else v = j < 6 ? pat[j] : (((w0 + w) >> (j - 6)) & 1) ? ~0ULL : 0;
                ah[i][w] = bh[i][w] = v; al[i][w] = bl[i][w] = 0;
            }
            if (A->cons[i] == '-') ++j;
        }
        uint64_t bad = 0;
        for (int g = 0; g < A->ng; ++g) apply4(&A->g[g], ah, al, W, &bad);
        for (int g = 0; g < B->ng; ++g) apply4(&B->g[g], bh, bl, W, &bad);
        if (bad & lanemask) { res = -1; break; }
        res = compare4(A, ah, al, bh, bl, W, lanemask);
    }
    for (int i = 0; i < n; ++i) { free(ah[i]); free(al[i]); free(bh[i]); free(bl[i]); }
    free(ah); free(al); free(bh); free(bl);
    return res;
}
/* 1 equal on every non-garbage line of A for every free-input assignment, 0 not, -1 cannot judge (NCV, shape, size). */
static int equivalent(const Circ *A, const Circ *B, int maxk) {
    if (A->n != B->n) return -1;
    for (int i = 0; i < A->n; ++i) if (A->cons[i] != B->cons[i]) return -1;
    if (A->ncv || B->ncv) return equivalent4(A, B, maxk, 0);
    int k = nfree(A);
    if (k > maxk) return -1;
    long long total = k >= 6 ? 1LL << (k - 6) : 1, chunks = (total + CHUNK - 1) / CHUNK;
    uint64_t lanemask = k >= 6 ? ~0ULL : ((1ULL << (1 << k)) - 1);
    uint64_t **sa = malloc(sizeof(uint64_t *) * (size_t)A->n), **sb = malloc(sizeof(uint64_t *) * (size_t)A->n);
    for (int i = 0; i < A->n; ++i) { sa[i] = malloc(8 * CHUNK); sb[i] = malloc(8 * CHUNK); }
    int eq = 1;
    for (long long ci = 0; eq && ci < chunks; ++ci) {
        int W = run_chunk(A, sa, ci, k); run_chunk(B, sb, ci, k);
        for (int i = 0; eq && i < A->n; ++i) if (A->garb[i] != '1') for (int w = 0; w < W; ++w) if ((sa[i][w] ^ sb[i][w]) & lanemask) { eq = 0; break; }
    }
    for (int i = 0; i < A->n; ++i) { free(sa[i]); free(sb[i]); }
    free(sa); free(sb);
    return eq;
}


/* Sampled check for circuits too wide to enumerate: WORDS x 64 random free-input assignments (xorshift, fixed seed).
 * 1 no difference found, 0 a difference found. Evidence, never a proof. */
static int sampled_equal(const Circ *A, const Circ *B, int words) {
    if (A->ncv || B->ncv) return equivalent4(A, B, 0, words) == 1;
    uint64_t **sa = malloc(sizeof(uint64_t *) * (size_t)A->n), **sb = malloc(sizeof(uint64_t *) * (size_t)A->n);
    for (int i = 0; i < A->n; ++i) { sa[i] = malloc(8 * CHUNK); sb[i] = malloc(8 * CHUNK); }
    uint64_t x = 0x9E3779B97F4A7C15ULL; int eq = 1;
    for (int done = 0; eq && done < words; done += CHUNK) {
        int W = words - done < CHUNK ? words - done : CHUNK;
        for (int i = 0; i < A->n; ++i) for (int w = 0; w < W; ++w) {
            uint64_t v;
            if (A->cons[i] == '0') v = 0; else if (A->cons[i] == '1') v = ~0ULL;
            else { x ^= x << 13; x ^= x >> 7; x ^= x << 17; v = x; }
            sa[i][w] = sb[i][w] = v;
        }
        for (int g = 0; g < A->ng; ++g) apply(&A->g[g], sa, W);
        for (int g = 0; g < B->ng; ++g) apply(&B->g[g], sb, W);
        for (int i = 0; eq && i < A->n; ++i) if (A->garb[i] != '1') for (int w = 0; w < W; ++w) if (sa[i][w] != sb[i][w]) { eq = 0; break; }
    }
    for (int i = 0; i < A->n; ++i) { free(sa[i]); free(sb[i]); }
    free(sa); free(sb);
    return eq;
}


/* ---- miter: an independent circuit-to-CNF encoder (written for revverify; shares no code with revopt's miter).
 * Both circuits read the same input variables (constants fixed by unit clauses); every gate makes fresh variables for
 * the lines it changes (Tseitin: AND of the controls, then XOR into the target; Fredkin by two ITEs; Peres and inverse
 * Peres as their two steps). One clause asks for a difference on some compared (non-garbage) line, so UNSAT means the
 * circuits agree on every input. The reference (first file) decides the garbage reading. V gates are refused. */
static int mv_n; static int *mv_cl; static size_t mv_len, mv_cap; static long mv_ncl;
static void mv_put(int lit) { if (mv_len == mv_cap) { mv_cap = mv_cap ? 2 * mv_cap : 1 << 16; mv_cl = realloc(mv_cl, sizeof(int) * mv_cap); if (!mv_cl) die("out of memory"); } mv_cl[mv_len++] = lit; }
static void mv_clause(const int *l, int k) { for (int i = 0; i < k; ++i) mv_put(l[i]); mv_put(0); ++mv_ncl; }
static void mv_c2(int a, int b) { int l[2] = { a, b }; mv_clause(l, 2); }
static void mv_c3(int a, int b, int c) { int l[3] = { a, b, c }; mv_clause(l, 3); }
static int mv_and(const int *x, int k) {   /* fresh p = x1 & ... & xk (k >= 2) */
    int p = ++mv_n, *l = malloc(sizeof(int) * (size_t)(k + 1)); if (!l) die("out of memory");
    for (int i = 0; i < k; ++i) { mv_c2(-p, x[i]); l[i] = -x[i]; }
    l[k] = p; mv_clause(l, k + 1); free(l);
    return p;
}
static int mv_mutant;   /* REVVERIFY_MUTANT=1: a deliberately broken encoder (one XOR clause dropped), for the controls */
static int mv_xor(int a, int b) {   /* fresh y = a ^ b */
    int y = ++mv_n;
    mv_c3(-y, a, b); mv_c3(-y, -a, -b); mv_c3(y, -a, b); if (!mv_mutant) mv_c3(y, a, -b);
    return y;
}
static int mv_ite(int c, int a, int b) {   /* fresh y = c ? a : b */
    int y = ++mv_n;
    mv_c3(-c, -a, y); mv_c3(-c, a, -y); mv_c3(c, -b, y); mv_c3(c, b, -y);
    return y;
}
static int mv_prod(const Gate *G, const int *v) {   /* literal for the AND of the gate's controls; 0 means constant 1 */
    int x[MAXC]; for (int i = 0; i < G->nc; ++i) x[i] = v[G->c[i]];
    return G->nc == 0 ? 0 : G->nc == 1 ? x[0] : mv_and(x, G->nc);
}
static int mv_circ(const Circ *C, int *v) {
    for (int g = 0; g < C->ng; ++g) {
        const Gate *G = &C->g[g];
        switch (G->kind) {
        case G_T: { int p = mv_prod(G, v); v[G->t[0]] = p ? mv_xor(v[G->t[0]], p) : -v[G->t[0]]; } break;
        case G_F: { int p = mv_prod(G, v), a = v[G->t[0]], b = v[G->t[1]];
            if (!p) { v[G->t[0]] = b; v[G->t[1]] = a; } else { v[G->t[0]] = mv_ite(p, b, a); v[G->t[1]] = mv_ite(p, a, b); } } break;
        case G_P: { int x[2] = { v[G->c[0]], v[G->c[1]] }; v[G->t[0]] = mv_xor(v[G->t[0]], mv_and(x, 2)); v[G->c[1]] = mv_xor(v[G->c[1]], v[G->c[0]]); } break;
        case G_PI: { v[G->c[1]] = mv_xor(v[G->c[1]], v[G->c[0]]); int x[2] = { v[G->c[0]], v[G->c[1]] }; v[G->t[0]] = mv_xor(v[G->t[0]], mv_and(x, 2)); } break;
        default: return -1;
        }
    }
    return 0;
}
/* V gates (v, v+) in the miter: a line holds a value in Z4 as two literals (hi, lo); classical 0 and 1 are 0 and 2, v adds
 * 1, v+ subtracts 1, NOT/CNOT/Toffoli add 2. A gate fires where its controls are classical 1. A control holding an odd
 * value (a superposition) is outside what this check can vouch for, so it is reported as a failure (*bad), never ignored. */
static int mv_or2(int a, int b) {   /* fresh y = a | b */
    int y = ++mv_n; mv_c2(-a, y); mv_c2(-b, y); mv_c3(-y, a, b); return y;
}
static int mv_circ4(const Circ *C, int *hi, int *lo, int *bad) {
    for (int g = 0; g < C->ng; ++g) {
        const Gate *G = &C->g[g];
        for (int i = 0; i < G->nc; ++i) *bad = mv_or2(*bad, lo[G->c[i]]);
        int p = mv_prod(G, hi), t = G->t[0];
        switch (G->kind) {
        case G_T: hi[t] = p ? mv_xor(hi[t], p) : -hi[t]; break;
        case G_V: case G_VP: {   /* the carry into hi: lo was 1 (v) or 0 (v+) where the gate fires */
            int l = G->kind == G_V ? lo[t] : -lo[t], x[2] = { l, p }, c = p ? mv_and(x, 2) : l;
            lo[t] = p ? mv_xor(lo[t], p) : -lo[t]; hi[t] = mv_xor(hi[t], c); } break;
        case G_F: { int a = t, b = G->t[1], ha = hi[a], hb = hi[b], la = lo[a], lb = lo[b];
            if (!p) { hi[a] = hb; hi[b] = ha; lo[a] = lb; lo[b] = la; }
            else { hi[a] = mv_ite(p, hb, ha); hi[b] = mv_ite(p, ha, hb); lo[a] = mv_ite(p, lb, la); lo[b] = mv_ite(p, la, lb); } } break;
        case G_P: { int x[2] = { hi[G->c[0]], hi[G->c[1]] }; hi[t] = mv_xor(hi[t], mv_and(x, 2)); hi[G->c[1]] = mv_xor(hi[G->c[1]], hi[G->c[0]]); } break;
        case G_PI: { hi[G->c[1]] = mv_xor(hi[G->c[1]], hi[G->c[0]]); int x[2] = { hi[G->c[0]], hi[G->c[1]] }; hi[t] = mv_xor(hi[t], mv_and(x, 2)); } break;
        default: return -1;
        }
    }
    return 0;
}
static int miter_main(const char *pa, const char *pb, const char *out) {
    static Circ A, B; if (parse(pa, &A) || parse(pb, &B)) die("cannot parse");
    if (A.n != B.n) { printf("miter: line counts differ\n"); return 3; }
    for (int i = 0; i < A.n; ++i) if (A.cons[i] != B.cons[i]) { printf("miter: constants differ on line %d\n", i); return 3; }
    memcpy(B.garb, A.garb, (size_t)A.n);   /* the reference decides the garbage reading */
    int n = A.n, *va = malloc(sizeof(int) * (size_t)n), *vb = malloc(sizeof(int) * (size_t)n);
    if (!va || !vb) die("out of memory");
    mv_n = n; mv_len = 0; mv_ncl = 0; mv_mutant = getenv("REVVERIFY_MUTANT") != NULL;
    for (int i = 0; i < n; ++i) { va[i] = vb[i] = i + 1; if (A.cons[i] != '-') { int u = A.cons[i] == '1' ? i + 1 : -(i + 1); mv_clause(&u, 1); } }
    int *d = malloc(sizeof(int) * (size_t)(2 * n + 2)), nd = 0; if (!d) die("out of memory");
    if (A.ncv || B.ncv) {   /* V gates: the Z4 encoding; a control in a superposition counts as a failure */
        int zero = ++mv_n; { int u = -zero; mv_clause(&u, 1); }
        int *la = malloc(sizeof(int) * (size_t)n), *lb = malloc(sizeof(int) * (size_t)n), bad = zero; if (!la || !lb) die("out of memory");
        for (int i = 0; i < n; ++i) la[i] = lb[i] = zero;
        if (mv_circ4(&A, va, la, &bad) || mv_circ4(&B, vb, lb, &bad)) { printf("miter: unsupported gate\n"); return 3; }
        for (int i = 0; i < n; ++i) if (A.garb[i] != '1') { d[nd++] = va[i] == vb[i] ? 0 : mv_xor(va[i], vb[i]); d[nd++] = la[i] == lb[i] ? 0 : mv_xor(la[i], lb[i]); }
        d[nd++] = bad; free(la); free(lb);
    } else {
        if (mv_circ(&A, va) || mv_circ(&B, vb)) { printf("miter: unsupported gate\n"); return 3; }
        for (int i = 0; i < n; ++i) if (A.garb[i] != '1') d[nd++] = va[i] == vb[i] ? 0 : mv_xor(va[i], vb[i]);
    }
    int k = 0; for (int i = 0; i < nd; ++i) if (d[i]) d[k++] = d[i];
    if (k == 0) { int e = 1; mv_clause(&e, 1); e = -1; mv_clause(&e, 1); }   /* every compared output is the same literal: UNSAT */
    else mv_clause(d, k);
    FILE *f = fopen(out, "w"); if (!f) die("cannot write");
    fprintf(f, "p cnf %d %ld\n", mv_n, mv_ncl);
    for (size_t i = 0; i < mv_len; ++i) fprintf(f, mv_cl[i] ? "%d " : "0\n", mv_cl[i]);
    fclose(f);
    printf("miter (revverify encoder): lines=%d compared_outputs=%d vars=%d clauses=%ld -> %s\n", n, nd, mv_n, mv_ncl, out);
    free(va); free(vb); free(d);
    return 0;
}
static int stat_main(const char *path) {
    static Circ C; if (parse(path, &C)) die("cannot parse");
    int k[6] = {0}; for (int i = 0; i < C.ng; ++i) k[C.g[i].kind]++;
    unsigned long long q; int bad = qc(&C, &q);
    if (bad) printf("%s lines=%d gates=%d qc=REFUSED(cost above 2^64-1) t=%d f=%d p=%d pi=%d v=%d v+=%d\n", path, C.n, C.ng, k[0], k[1], k[2], k[3], k[4], k[5]);
    else printf("%s lines=%d gates=%d qc=%llu t=%d f=%d p=%d pi=%d v=%d v+=%d\n", path, C.n, C.ng, q, k[0], k[1], k[2], k[3], k[4], k[5]);
    return bad ? 3 : 0;
}
static int qc2real_main(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) { fprintf(stderr, "revverify: cannot open %s\n", path); return 3; }
    static char name[MAXL][32]; int n = 0, ng = 0, cap = 0; char **gl = NULL; char line[1 << 16];
    while (fgets(line, sizeof line, f)) {
        char *tok = strtok(line, " \t\r\n"); if (!tok || tok[0] == '#') continue;
        if (!strcmp(tok, ".v")) { while ((tok = strtok(NULL, " \t\r\n"))) { if (n == MAXL || strlen(tok) >= 32) { fprintf(stderr, "revverify: %s: too many or too long names\n", path); fclose(f); return 3; } strcpy(name[n++], tok); } continue; }
        if (tok[0] == '.' || !strcmp(tok, "BEGIN") || !strcmp(tok, "END")) continue;
        int isx = !strcmp(tok, "X"), istof = !strcmp(tok, "tof");
        if (!isx && !istof) { fprintf(stderr, "revverify: %s: gate '%s' is not X or tof: refused (not a classical circuit)\n", path, tok); fclose(f); return 3; }
        char buf[1 << 16]; int k = 0, len = 0;
        while ((tok = strtok(NULL, " \t\r\n"))) {
            int i; for (i = 0; i < n; ++i) if (!strcmp(name[i], tok)) break;
            if (i == n) { fprintf(stderr, "revverify: %s: unknown qubit '%s'\n", path, tok); fclose(f); return 3; }
            if (len + 34 >= (int)sizeof buf) { fprintf(stderr, "revverify: %s: gate too wide\n", path); fclose(f); return 3; }
            len += snprintf(buf + len, sizeof buf - (size_t)len, " %s", tok); ++k;
        }
        if (k == 0 || (isx && k != 1) || k > MAXC + 1) { fprintf(stderr, "revverify: %s: gate with %d operands refused\n", path, k); fclose(f); return 3; }
        if (ng == cap) { cap = cap ? 2 * cap : 256; gl = realloc(gl, sizeof(char *) * (size_t)cap); if (!gl) die("out of memory"); }
        char *g = malloc((size_t)len + 16); if (!g) die("out of memory"); snprintf(g, (size_t)len + 16, "t%d%s", k, buf); gl[ng++] = g;
    }
    fclose(f);
    if (n == 0) { fprintf(stderr, "revverify: %s: no .v line\n", path); return 3; }
    printf(".version 1.0\n.numvars %d\n.variables", n); for (int i = 0; i < n; ++i) printf(" %s", name[i]);
    printf("\n.inputs"); for (int i = 0; i < n; ++i) printf(" %s", name[i]);
    printf("\n.outputs"); for (int i = 0; i < n; ++i) printf(" %s", name[i]);
    printf("\n.constants "); for (int i = 0; i < n; ++i) putchar('-');
    printf("\n.garbage "); for (int i = 0; i < n; ++i) putchar('-');
    printf("\n.begin\n"); for (int i = 0; i < ng; ++i) { printf("%s\n", gl[i]); free(gl[i]); } printf(".end\n"); free(gl);
    return 0;
}
int main(int argc, char **argv) {
    if (argc >= 2 && !strcmp(argv[1], "--intended")) { g_intended = 1; ++argv; --argc; }
    if (argc >= 3 && !strcmp(argv[1], "stat")) return stat_main(argv[2]);
    if (argc >= 3 && !strcmp(argv[1], "qc2real")) return qc2real_main(argv[2]);
    if (argc >= 5 && !strcmp(argv[1], "miter")) return miter_main(argv[2], argv[3], argv[4]);
    if (argc >= 4 && !strcmp(argv[1], "equiv")) {
        static Circ A, B; if (parse(argv[2], &A) || parse(argv[3], &B)) die("cannot parse");
        if (A.n == B.n) memcpy(B.garb, A.garb, (size_t)A.n);   /* the reference (first file) decides the garbage reading */
        int e = equivalent(&A, &B, 26), s = -1;
        if (e == -1 && A.n == B.n) s = sampled_equal(&A, &B, 16384);
        printf("equivalent=%s\n", e == 1 ? "yes" : e == 0 ? "NO" : s == 1 ? "sampled_1048576" : s == 0 ? "NO_sampled" : "not_judged");
        return e == 1 ? 0 : s == 1 ? 4 : 1;
    }
    fprintf(stderr, "usage: revverify [--intended] stat FILE.real | equiv A.real B.real | miter A.real B.real OUT.cnf | qc2real FILE.qc\n");
    return 2;
}
