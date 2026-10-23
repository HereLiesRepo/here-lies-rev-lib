/* pathverify -- exact equivalence of two Clifford+T circuits of any width, by path sums.
 *   pathverify equiv A.qc B.qc          EQUAL, NOT_EQUAL or UNKNOWN (never a guess)
 *   pathverify --zero-ancillas equiv A B   judge only inputs whose non-.i qubits start at 0 (both files must agree on .i)
 *   pathverify selftest                 controls, planted cheaters among them (exit 0 only when every one holds)
 *   pathverify guess A.qc               candidate T-gate merges for phase/phaseopt (Allegedly (Confirmed)); proves nothing
 * Not ours: the sum-over-paths representation of a Clifford+T circuit and its reduction rules ([Elim], [HH], [omega])
 * are Amy, "Towards Large-scale Functional Verification of Universal Quantum Circuits", QPL 2018 (EPTCS 287, 2019;
 * arXiv:1805.06908), the verifier inside Amy's Feynman toolkit. This file reimplements them in C so phase-lane results
 * can be proven past the width of verifier/phaseverify.
 * Method: the circuit B^-1 A is written as U|x> = 2^(-h/2) sum_y w^P(x,y) |f(x,y)>, w = e^(i pi/4), with h the number
 * of H gates, y one path variable per H, f an affine parity per wire and P a multilinear polynomial over Z8 (boolean
 * variables, so x^2 = x; degree up to MAXDEG, past which the run is REFUSED, never truncated). H on a wire holding
 * parity p adds a variable y with phase 4 y p; a phase w^k on parity p adds k * E(p), where for a boolean polynomial
 * q = c + m_1 + ... + m_n, E(q) = c + (1-2c) sum over nonempty T of (-2)^(|T|-1) prod_{i in T} m_i (exact mod 8); CZ
 * and CCZ add 4 * the product of their parities; X flips a constant; CNOT and swap move parities. Then, until nothing
 * changes:
 *   [Elim]  y appears nowhere: sum_y 1 = 2, so h -= 2.
 *   [HH]    y is in no output and only in 4 y Q, Q a boolean polynomial of other variables: the sum is 2 [Q = 0]; when
 *           Q = z + Q' with z a path variable not in Q', substitute z := Q' everywhere (the phase, and the outputs when
 *           Q' is affine; otherwise z must be in no output), h -= 2.
 *   [omega] y is in no output and only in (2 + 4 c) y + 4 y Q': the sum is sqrt2 w^(1 - 2 Q), Q = c + Q'; h -= 1.
 * EQUAL only when every path variable is gone, h = 0, every output is its own input and P is a constant (a global
 * phase): B^-1 A is then exactly a global phase times the identity. With no path variable left and anything else
 * standing, B^-1 A is a diagonal permutation that is not a scalar: NOT_EQUAL. Path variables that no rule removes:
 * UNKNOWN, never a verdict. Multilinear forms over Z8 of functions on bits are unique, so a nonconstant P is a
 * nonconstant phase. C11, no dependencies. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXV 8192
#define MAXOP 32                                                   /* operands of one gate (multi-control Toffoli, multi-Z) */
#define NONE 0xFFFFFFFFu
static void die(const char *m) { fprintf(stderr, "pathverify: %s\n", m); exit(2); }
static void *xrealloc(void *p, size_t n) { p = realloc(p, n ? n : 1); if (!p) die("out of memory"); return p; }

/* ---------------- circuits ---------------- */
enum { G_H, G_X, G_Y, G_PH, G_CX, G_SWAP, G_CCX, G_CZ, G_CCZ, G_MCX, G_MCZ };
typedef struct { int kind, na, a[MAXOP], k; } Gate;            /* G_PH: w^k on a[0]; CCX, MCX: the last operand is the target */
typedef struct { int n; char (*name)[32]; Gate *g; int ng, cap; int has_i; unsigned char *isin; } Circ;
static void cpush(Circ *C, Gate x) { if (C->ng == C->cap) { C->cap = C->cap ? 2 * C->cap : 256; C->g = xrealloc(C->g, sizeof(Gate) * (size_t)C->cap); } C->g[C->ng++] = x; }
static int qubit(const Circ *C, const char *s) { for (int i = 0; i < C->n; ++i) if (!strcmp(C->name[i], s)) return i; return -1; }
static int parse(const char *path, Circ *C, char *why, size_t wl) {
    FILE *f = fopen(path, "r"); if (!f) { snprintf(why, wl, "cannot open %s", path); return 0; }
    memset(C, 0, sizeof *C); C->name = calloc(MAXV, 32); C->isin = calloc(MAXV, 1); if (!C->name || !C->isin) die("out of memory");
    static char line[1 << 16]; int body = 0, lno = 0;
    while (fgets(line, sizeof line, f)) {
        ++lno; char *hh = strchr(line, '#'); if (hh) *hh = 0;
        static char *tok[MAXV + 2]; int nt = 0;
        for (char *p = strtok(line, " \t\r\n,"); p && nt < MAXV + 2; p = strtok(NULL, " \t\r\n,")) tok[nt++] = p;
        if (!nt) continue;
        if (!strcmp(tok[0], ".v")) { for (int i = 1; i < nt; ++i) { if (C->n >= MAXV) { fclose(f); snprintf(why, wl, "too many qubits"); return 0; } snprintf(C->name[C->n++], 32, "%s", tok[i]); } continue; }
        if (!strcmp(tok[0], ".i")) { C->has_i = 1; for (int i = 1; i < nt; ++i) { int q = qubit(C, tok[i]); if (q < 0) { fclose(f); snprintf(why, wl, "%s:%d: .i names unknown qubit '%s'", path, lno, tok[i]); return 0; } C->isin[q] = 1; } continue; }
        if (tok[0][0] == '.') continue;
        if (!strcmp(tok[0], "BEGIN")) { body = 1; continue; }
        if (!strcmp(tok[0], "END")) { body = 0; continue; }
        if (!body) continue;
        const char *k = tok[0]; int q[MAXV], na = 0;
        int isz = !strcmp(k, "Z") || !strcmp(k, "Zd");
        for (int i = 1; i < nt; ++i) {
            int x = qubit(C, tok[i]); if (x < 0) { fclose(f); snprintf(why, wl, "%s:%d: unknown qubit '%s'", path, lno, tok[i]); return 0; }
            int dup = 0; for (int j = 0; j < na; ++j) if (q[j] == x) dup = 1;
            if (dup && isz) continue;                         /* Z a b a = Z a b: (-1)^(a b a) = (-1)^(a b) */
            if (dup) { fclose(f); snprintf(why, wl, "%s:%d: qubit '%s' used twice", path, lno, tok[i]); return 0; }
            q[na++] = x;
        }
        if (na > MAXOP) { fclose(f); snprintf(why, wl, "%s:%d: gate with %d operands, above %d", path, lno, na, MAXOP); return 0; }
        Gate g; memset(&g, 0, sizeof g); g.na = na; for (int i = 0; i < na; ++i) g.a[i] = q[i];
        int ok = 1;
        if (!strcmp(k, "H") && na == 1) g.kind = G_H;
        else if (!strcmp(k, "X") && na == 1) g.kind = G_X;
        else if (!strcmp(k, "Y") && na == 1) g.kind = G_Y;
        else if (!strcmp(k, "T") && na == 1) { g.kind = G_PH; g.k = 1; }
        else if (!strcmp(k, "T*") && na == 1) { g.kind = G_PH; g.k = 7; }
        else if ((!strcmp(k, "S") || !strcmp(k, "P")) && na == 1) { g.kind = G_PH; g.k = 2; }
        else if ((!strcmp(k, "S*") || !strcmp(k, "P*")) && na == 1) { g.kind = G_PH; g.k = 6; }
        else if (isz && na == 1) { g.kind = G_PH; g.k = 4; }
        else if (isz && na == 2) g.kind = G_CZ;
        else if (isz && na == 3) g.kind = G_CCZ;
        else if (isz && na > 3) g.kind = G_MCZ;
        else if ((!strcmp(k, "tof") || !strcmp(k, "cnot") || !strcmp(k, "CNOT")) && na == 2) g.kind = G_CX;
        else if (!strcmp(k, "tof") && na == 3) g.kind = G_CCX;
        else if (!strcmp(k, "tof") && na > 3) g.kind = G_MCX;
        else if (!strcmp(k, "swap") && na == 2) g.kind = G_SWAP;
        else ok = 0;
        if (!ok) { fclose(f); snprintf(why, wl, "%s:%d: gate '%s' with %d operands not supported (refusing rather than guessing)", path, lno, k, na); return 0; }
        cpush(C, g);
    }
    fclose(f);
    if (!C->n) { snprintf(why, wl, "%s: no .v line", path); return 0; }
    return 1;
}

/* ---------------- parities: sorted variable lists plus a constant ---------------- */
typedef struct { uint32_t *v; int n, cap; int c; } Par;
static void pset1(Par *p, uint32_t x) { if (p->cap < 1) { p->cap = 4; p->v = xrealloc(p->v, 4 * sizeof(uint32_t)); } p->v[0] = x; p->n = 1; p->c = 0; }
static void pcopy(Par *d, const Par *s) { if (d->cap < s->n) { d->cap = s->n + 4; d->v = xrealloc(d->v, sizeof(uint32_t) * (size_t)d->cap); } memcpy(d->v, s->v, sizeof(uint32_t) * (size_t)s->n); d->n = s->n; d->c = s->c; }
static uint32_t *tmpv; static int tmpcap;
static void pxor(Par *d, const Par *s) {                          /* d ^= s */
    if (tmpcap < d->n + s->n) { tmpcap = 2 * (d->n + s->n) + 8; tmpv = xrealloc(tmpv, sizeof(uint32_t) * (size_t)tmpcap); }
    int i = 0, j = 0, m = 0;
    while (i < d->n || j < s->n) {
        if (j >= s->n || (i < d->n && d->v[i] < s->v[j])) tmpv[m++] = d->v[i++];
        else if (i >= d->n || s->v[j] < d->v[i]) tmpv[m++] = s->v[j++];
        else { ++i; ++j; }
    }
    if (d->cap < m) { d->cap = m + 8; d->v = xrealloc(d->v, sizeof(uint32_t) * (size_t)d->cap); }
    memcpy(d->v, tmpv, sizeof(uint32_t) * (size_t)m); d->n = m; d->c ^= s->c;
}
static int phas(const Par *p, uint32_t x) { int lo = 0, hi = p->n - 1; while (lo <= hi) { int m = (lo + hi) / 2; if (p->v[m] == x) return 1; if (p->v[m] < x) lo = m + 1; else hi = m - 1; } return 0; }

/* ---------------- the phase polynomial: monomials of any degree up to MAXDEG, coefficients mod 8 ---------------- */
#define MAXDEG 40
typedef struct { uint32_t off; uint8_t len, c, alive; } Node;     /* variables mv[off .. off+len), sorted */
static Node *nodes; static size_t nn, ncap;
static uint32_t *mv; static size_t mvn, mvcap;                     /* monomial variable arena */
static int64_t *ht; static size_t hcap, hused;                    /* open addressing: node index or -1 */
static int64_t constant;                                          /* the empty monomial, mod 8 */
static int degree_overflow;
static size_t peak_live;                                          /* the most phase monomials alive at once */
typedef struct { int64_t *v; int n, cap; } Occ;
static Occ *occ; static size_t nocc;                              /* per variable: node indices (stale entries allowed) */
static size_t nvars, ninputs;
static int *outcnt;                                               /* per variable: number of output parities holding it */
static unsigned char *gone;                                       /* per variable: summed out or substituted away */
static uint64_t mhash(const uint32_t *k, int n) { uint64_t h = 1469598103934665603ULL ^ (uint64_t)n; for (int i = 0; i < n; ++i) { h ^= k[i]; h *= 1099511628211ULL; h ^= h >> 29; } return h; }
static uint64_t nhash(int64_t idx) { return mhash(mv + nodes[idx].off, nodes[idx].len); }
static void occ_add(uint32_t v, int64_t idx) {
    if (v >= nocc) die("variable index past the occurrence table");
    Occ *o = &occ[v]; if (o->n == o->cap) { o->cap = o->cap ? 2 * o->cap : 4; o->v = xrealloc(o->v, sizeof(int64_t) * (size_t)o->cap); } o->v[o->n++] = idx;
}
static void rehash(void) {
    size_t nc = hcap ? 2 * hcap : 1 << 16; int64_t *t = malloc(sizeof(int64_t) * nc); if (!t) die("out of memory");
    for (size_t i = 0; i < nc; ++i) t[i] = -1;
    for (size_t i = 0; i < hcap; ++i) if (ht[i] >= 0) { size_t j = nhash(ht[i]) & (nc - 1); while (t[j] >= 0) j = (j + 1) & (nc - 1); t[j] = ht[i]; }
    free(ht); ht = t; hcap = nc;
}
static void hdel(int64_t idx) {                                    /* remove node idx from the table (backward-shift deletion) */
    size_t j = nhash(idx) & (hcap - 1);
    while (ht[j] != idx) j = (j + 1) & (hcap - 1);
    size_t i = j;
    for (;;) {
        ht[i] = -1; size_t k = i;
        for (;;) {
            k = (k + 1) & (hcap - 1);
            if (ht[k] < 0) { --hused; return; }
            size_t home = nhash(ht[k]) & (hcap - 1);
            if ((k > i && (home <= i || home > k)) || (k < i && (home <= i && home > k))) break;
        }
        ht[i] = ht[k]; i = k;
    }
}
static int cmpu(const void *a, const void *b) { uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b; return x < y ? -1 : x > y; }
/* add c * (the monomial over the variables vs[0..n)); vs need not be sorted or distinct (x^2 = x) */
static void madd(const uint32_t *vs, int n, int c) {
    c &= 7; if (!c) return;
    uint32_t k[4 * MAXDEG + 8]; int m = 0;
    if (n > 4 * MAXDEG) { degree_overflow = 1; return; }
    if (n) { memcpy(k, vs, sizeof(uint32_t) * (size_t)n); qsort(k, (size_t)n, sizeof(uint32_t), cmpu); }
    for (int i = 0; i < n; ++i) if (!m || k[m - 1] != k[i]) k[m++] = k[i];
    if (m == 0) { constant = (constant + c) & 7; return; }
    if (m > MAXDEG) { degree_overflow = 1; return; }
    if ((hused + 1) * 2 > hcap) rehash();
    uint64_t h = mhash(k, m); size_t j = h & (hcap - 1);
    while (ht[j] >= 0) {
        Node *x = &nodes[ht[j]];
        if (x->len == m && !memcmp(mv + x->off, k, sizeof(uint32_t) * (size_t)m)) { x->c = (uint8_t)((x->c + c) & 7); if (!x->c) { x->alive = 0; hdel(ht[j]); } return; }
        j = (j + 1) & (hcap - 1);
    }
    if (nn == ncap) { ncap = ncap ? 2 * ncap : 1 << 16; nodes = xrealloc(nodes, sizeof(Node) * ncap); }
    if (mvn + (size_t)m > mvcap) { mvcap = mvcap ? 2 * mvcap : 1 << 18; while (mvcap < mvn + (size_t)m) mvcap *= 2; mv = xrealloc(mv, sizeof(uint32_t) * mvcap); }
    Node *x = &nodes[nn]; x->off = (uint32_t)mvn; x->len = (uint8_t)m; x->c = (uint8_t)c; x->alive = 1;
    if (mvn > 0xFFFFFFF0u) die("monomial arena past 2^32 entries");
    memcpy(mv + mvn, k, sizeof(uint32_t) * (size_t)m); mvn += (size_t)m;
    ht[j] = (int64_t)nn; ++hused; if (hused > peak_live) peak_live = hused;
    for (int i = 0; i < m; ++i) occ_add(k[i], (int64_t)nn);
    ++nn;
}
/* reclaim dead nodes: rebuild the node array, the arena, the table and every occurrence list from the live nodes */
static void compact(void) {
    size_t live = 0, words = 0; for (size_t i = 0; i < nn; ++i) if (nodes[i].alive) { ++live; words += nodes[i].len; }
    Node *nn2 = malloc(sizeof(Node) * (live + 1)); uint32_t *mv2 = malloc(sizeof(uint32_t) * (words + 1)); if (!nn2 || !mv2) die("out of memory");
    size_t j = 0, w = 0;
    for (size_t i = 0; i < nn; ++i) if (nodes[i].alive) { nn2[j] = nodes[i]; nn2[j].off = (uint32_t)w; memcpy(mv2 + w, mv + nodes[i].off, sizeof(uint32_t) * nodes[i].len); w += nodes[i].len; ++j; }
    free(nodes); free(mv); nodes = nn2; nn = ncap = live; mv = mv2; mvn = mvcap = words;
    if (ncap == 0) { ncap = 1; } if (mvcap == 0) { mvcap = 1; }
    for (size_t v = 0; v < nocc; ++v) occ[v].n = 0;
    free(ht); ht = NULL; hcap = 0; hused = 0;
    size_t want = 1 << 16; while (want < 2 * live + 2) want *= 2;
    ht = malloc(sizeof(int64_t) * want); if (!ht) die("out of memory"); hcap = want; for (size_t i = 0; i < hcap; ++i) ht[i] = -1;
    for (size_t i = 0; i < nn; ++i) { size_t h = nhash((int64_t)i) & (hcap - 1); while (ht[h] >= 0) h = (h + 1) & (hcap - 1); ht[h] = (int64_t)i; ++hused;
        for (int k = 0; k < nodes[i].len; ++k) occ_add(mv[nodes[i].off + k], (int64_t)i); }
}
static void kill_node(int64_t idx) { if (nodes[idx].alive) { nodes[idx].alive = 0; hdel(idx); } }
static int node_has(const Node *x, uint32_t v) { const uint32_t *k = mv + x->off; int lo = 0, hi = x->len - 1; while (lo <= hi) { int m = (lo + hi) / 2; if (k[m] == v) return 1; if (k[m] < v) lo = m + 1; else hi = m - 1; } return 0; }

/* boolean polynomials (XOR of monomials plus a constant): what a variable is substituted by */
typedef struct { uint32_t *v; int *off, *len; int n, nv, capm, capv; int c; } BP;
static void bp_clear(BP *q) { q->n = q->nv = 0; q->c = 0; }
static void bp_add(BP *q, const uint32_t *k, int len) {
    if (q->n == q->capm) { q->capm = q->capm ? 2 * q->capm : 16; q->off = xrealloc(q->off, sizeof(int) * (size_t)q->capm); q->len = xrealloc(q->len, sizeof(int) * (size_t)q->capm); }
    if (q->nv + len > q->capv) { q->capv = 2 * (q->nv + len) + 16; q->v = xrealloc(q->v, sizeof(uint32_t) * (size_t)q->capv); }
    q->off[q->n] = q->nv; q->len[q->n] = len; memcpy(q->v + q->nv, k, sizeof(uint32_t) * (size_t)len); q->nv += len; q->n++;
}
static void bp_free(BP *q) { free(q->v); free(q->off); free(q->len); memset(q, 0, sizeof *q); }
static int bp_affine(const BP *q) { for (int i = 0; i < q->n; ++i) if (q->len[i] > 1) return 0; return 1; }
/* add a * r * E(q): r a monomial, E(q) the exact Z8 value of the boolean polynomial q,
 * E(c + m_1 + ... + m_n) = c + (1 - 2c) sum over nonempty T of (-2)^(|T|-1) prod_{i in T} m_i (exact mod 8) */
static void add_times_EB(int a, const uint32_t *r, int nr, const BP *q) {
    a &= 7; if (!a) return;
    if (q->c) madd(r, nr, a);
    int s = q->c ? -a : a;
    int maxt = (a & 1) ? 3 : (a & 2) ? 2 : 1;                      /* (-2)^(|T|-1) a vanishes mod 8 past this */
    uint32_t w[4 * MAXDEG + 8]; int nb = nr;
    if (nr > MAXDEG) { degree_overflow = 1; return; }
    if (nr) memcpy(w, r, sizeof(uint32_t) * (size_t)nr);
    for (int i = 0; i < q->n; ++i) {
        int li = q->len[i]; if (nb + li > 4 * MAXDEG) { degree_overflow = 1; return; }
        memcpy(w + nb, q->v + q->off[i], sizeof(uint32_t) * (size_t)li); madd(w, nb + li, s);
        if (maxt < 2) continue;
        for (int j = i + 1; j < q->n; ++j) {
            int lj = q->len[j]; if (nb + li + lj > 4 * MAXDEG) { degree_overflow = 1; return; }
            memcpy(w + nb + li, q->v + q->off[j], sizeof(uint32_t) * (size_t)lj); madd(w, nb + li + lj, -2 * s);
            if (maxt < 3) continue;
            for (int l = j + 1; l < q->n; ++l) {
                int ll = q->len[l]; if (nb + li + lj + ll > 4 * MAXDEG) { degree_overflow = 1; return; }
                memcpy(w + nb + li + lj, q->v + q->off[l], sizeof(uint32_t) * (size_t)ll); madd(w, nb + li + lj + ll, 4 * s);
            }
        }
    }
}
static BP parbp;
static void add_times_E(int a, const uint32_t *r, int nr, const Par *p) {   /* the affine special case */
    bp_clear(&parbp); parbp.c = p->c; for (int i = 0; i < p->n; ++i) bp_add(&parbp, &p->v[i], 1);
    add_times_EB(a, r, nr, &parbp);
}
/* add 4 * prod of the parities ps[0..np) (only their values mod 2 matter under the factor 4) */
static void add_4prod(Par **ps, int np) {
    uint32_t w[3]; int idx[3] = { 0, 0, 0 };
    int lim[3]; for (int i = 0; i < np; ++i) lim[i] = ps[i]->n;
    for (idx[0] = -1; idx[0] < lim[0]; ++idx[0]) {
        if (idx[0] < 0 && !ps[0]->c) continue;
        for (idx[1] = (np > 1 ? -1 : 0); idx[1] < (np > 1 ? lim[1] : 1); ++idx[1]) {
            if (np > 1 && idx[1] < 0 && !ps[1]->c) continue;
            for (idx[2] = (np > 2 ? -1 : 0); idx[2] < (np > 2 ? lim[2] : 1); ++idx[2]) {
                if (np > 2 && idx[2] < 0 && !ps[2]->c) continue;
                int m = 0;
                for (int i = 0; i < np; ++i) if (idx[i] >= 0) w[m++] = ps[i]->v[idx[i]];
                madd(w, m, 4);
            }
        }
    }
}

/* the same for any number of parities: one factor at a time, its constant or one of its variables */
static uint32_t prodw[MAXOP];
static void prod_rec(Par **ps, int np, int i, int m) {
    if (i == np) { madd(prodw, m, 4); return; }
    if (ps[i]->c) prod_rec(ps, np, i + 1, m);
    for (int j = 0; j < ps[i]->n; ++j) { prodw[m] = ps[i]->v[j]; prod_rec(ps, np, i + 1, m + 1); }
}
static void add_4prod_n(Par **ps, int np) { prod_rec(ps, np, 0, 0); }

/* ---------------- building the path sum of B^-1 A ---------------- */
static Par *wire; static int nq; static long hcount;
/* guess mode (Allegedly (Confirmed)): substitutions are remembered, phase gates of A are remembered */
static int rel_mode; static BP *subst_of; static size_t nsubst;
typedef struct { int gi, k; Par p; } PTerm;
static PTerm *pterms; static int npterms, cur_gate;
static uint32_t newvar(void) {
    uint32_t v = (uint32_t)nvars++;
    if (nvars > nocc) { size_t o = nocc; nocc = nocc ? 2 * nocc : 4096; while (nocc < nvars) nocc *= 2; occ = xrealloc(occ, sizeof(Occ) * nocc); memset(occ + o, 0, sizeof(Occ) * (nocc - o)); outcnt = xrealloc(outcnt, sizeof(int) * nocc); memset(outcnt + o, 0, sizeof(int) * (nocc - o)); gone = xrealloc(gone, nocc); memset(gone + o, 0, nocc - o); }
    return v;
}
static void gate(const Gate *g, int inverse);
/* A multi-controlled phase writes 4 times the product of its operands' parities, and that product has as many monomials
 * as the parities' sizes multiplied: one 14-control gate of RevLib's co14_215 on wires holding a few variables each wrote
 * over 50 million and never finished. H H is the identity, so before such a gate the widest operand wire gets H H, which
 * leaves it a single fresh variable (and one more [HH] constraint tying it to the old parity), until the product is at
 * most PROD_LIMIT. Exact: only the bookkeeping changes. */
#define PROD_LIMIT 256.0
static long narrowings;
static void narrow_operands(const Gate *g) {
    for (;;) {
        double prod = 1; int widest = -1, wn = 1;
        for (int i = 0; i < g->na; ++i) { const Par *p = &wire[g->a[i]]; int n = p->n + (p->c ? 1 : 0); prod *= n > 0 ? n : 1; if (n > wn) { wn = n; widest = g->a[i]; } }
        if (prod <= PROD_LIMIT || widest < 0) return;
        Gate h; memset(&h, 0, sizeof h); h.kind = G_H; h.na = 1; h.a[0] = widest; gate(&h, 0); gate(&h, 0); ++narrowings;
    }
}
static void gate(const Gate *g, int inverse) {
    switch (g->kind) {
    case G_H: { int q = g->a[0]; uint32_t y = newvar(); Par py = { 0 }; pset1(&py, y);
        Par *pp[2] = { &py, &wire[q] }; add_4prod(pp, 2); pcopy(&wire[q], &py); free(py.v); ++hcount; break; }
    case G_X: wire[g->a[0]].c ^= 1; break;
    case G_Y: { add_times_E(4, NULL, 0, &wire[g->a[0]]); wire[g->a[0]].c ^= 1; constant = (constant + 2) & 7; break; }  /* Y = i X Z = Y^-1 */
    case G_PH: if (rel_mode && !inverse && (g->k & 1)) { pterms = xrealloc(pterms, sizeof(PTerm) * (size_t)(npterms + 1)); PTerm *t = &pterms[npterms++]; memset(t, 0, sizeof *t); t->gi = cur_gate; t->k = g->k; pcopy(&t->p, &wire[g->a[0]]); }
        add_times_E(inverse ? (8 - g->k) & 7 : g->k, NULL, 0, &wire[g->a[0]]); break;
    case G_CX: pxor(&wire[g->a[1]], &wire[g->a[0]]); break;
    case G_SWAP: { Par t = wire[g->a[0]]; wire[g->a[0]] = wire[g->a[1]]; wire[g->a[1]] = t; break; }
    case G_CZ: { Par *pp[2] = { &wire[g->a[0]], &wire[g->a[1]] }; add_4prod(pp, 2); break; }
    case G_CCZ: { narrow_operands(g); Par *pp[3] = { &wire[g->a[0]], &wire[g->a[1]], &wire[g->a[2]] }; add_4prod(pp, 3); break; }
    case G_CCX: case G_MCX: { Gate h; memset(&h, 0, sizeof h); h.kind = G_H; h.na = 1; h.a[0] = g->a[g->na - 1];
        Gate z = *g; z.kind = G_MCZ; gate(&h, 0); gate(&z, 0); gate(&h, 0); break; }
    case G_MCZ: { narrow_operands(g); Par *pp[MAXOP]; for (int i = 0; i < g->na; ++i) pp[i] = &wire[g->a[i]]; add_4prod_n(pp, g->na); break; }
    }
}

/* ---------------- reduction ---------------- */
static int is_path(uint32_t v) { return v >= ninputs; }
/* Nonlinear outputs. [HH] solves z := R wherever z occurs, outputs included; that is sound for any boolean R. An output
 * wire a nonlinear R reaches is held as a boolean polynomial (nlw[q], nlon[q] = 1) and goes back to a parity as soon as
 * it is affine again. A wire left nonlinear at the end is never read as the identity, and refute declines to judge. */
static BP *nlw; static unsigned char *nlon; static int nlw_n;
typedef struct { uint32_t k[MAXDEG + 1]; int n; } Mono;
static Mono *mbuf; static size_t mn, mcap;
static int mono_cmp(const void *a, const void *b) { const Mono *x = a, *y = b; if (x->n != y->n) return x->n < y->n ? -1 : 1; for (int i = 0; i < x->n; ++i) if (x->k[i] != y->k[i]) return x->k[i] < y->k[i] ? -1 : 1; return 0; }
static void mpush(const uint32_t *a, int na, const uint32_t *b, int nb, uint32_t skip) {   /* push the monomial (a \ skip) * b */
    uint32_t t[2 * MAXDEG + 2]; int n = 0;
    if (na + nb > 2 * MAXDEG + 1) { degree_overflow = 1; return; }
    for (int i = 0; i < na; ++i) if (a[i] != skip) t[n++] = a[i];
    for (int i = 0; i < nb; ++i) t[n++] = b[i];
    qsort(t, (size_t)n, sizeof(uint32_t), cmpu);
    int m = 0; for (int i = 0; i < n; ++i) if (!m || t[m - 1] != t[i]) t[m++] = t[i];
    if (m > MAXDEG) { degree_overflow = 1; return; }
    if (mn == mcap) { mcap = mcap ? 2 * mcap : 64; mbuf = xrealloc(mbuf, sizeof(Mono) * mcap); }
    memcpy(mbuf[mn].k, t, sizeof(uint32_t) * (size_t)m); mbuf[mn].n = m; ++mn;
}
static int wire_has(int q, uint32_t z) {
    if (!nlon || !nlon[q]) return phas(&wire[q], z);
    for (int i = 0; i < nlw[q].nv; ++i) if (nlw[q].v[i] == z) return 1;
    return 0;
}
static void wire_count(int q, int d) {                              /* outcnt += d for each variable wire q holds, once */
    if (!nlon || !nlon[q]) { for (int i = 0; i < wire[q].n; ++i) outcnt[wire[q].v[i]] += d; return; }
    BP *w = &nlw[q]; uint32_t *t = xrealloc(NULL, sizeof(uint32_t) * (size_t)(w->nv + 1)); memcpy(t, w->v, sizeof(uint32_t) * (size_t)w->nv);
    qsort(t, (size_t)w->nv, sizeof(uint32_t), cmpu);
    for (int i = 0; i < w->nv; ++i) if (!i || t[i] != t[i - 1]) outcnt[t[i]] += d;
    free(t);
}
static void wire_subst(int q, uint32_t z, const BP *r) {           /* wire q with z := r, exactly (XOR of monomials) */
    if (!nlw) { nlw_n = nq; nlw = calloc((size_t)nq, sizeof(BP)); nlon = calloc((size_t)nq, 1); if (!nlw || !nlon) die("out of memory"); }
    int c; mn = 0;
    if (nlon[q]) { BP *w = &nlw[q]; c = w->c;
        for (int i = 0; i < w->n && !degree_overflow; ++i) {
            const uint32_t *m = w->v + w->off[i]; int l = w->len[i], has = 0;
            for (int j = 0; j < l; ++j) if (m[j] == z) has = 1;
            if (!has) { mpush(m, l, NULL, 0, UINT32_MAX); continue; }
            if (r->c & 1) mpush(m, l, NULL, 0, z);
            for (int k = 0; k < r->n; ++k) mpush(m, l, r->v + r->off[k], r->len[k], z);
        }
    } else { Par *p = &wire[q]; c = p->c;
        for (int i = 0; i < p->n && !degree_overflow; ++i) {
            if (p->v[i] != z) { mpush(&p->v[i], 1, NULL, 0, UINT32_MAX); continue; }
            if (r->c & 1) mpush(NULL, 0, NULL, 0, UINT32_MAX);
            for (int k = 0; k < r->n; ++k) mpush(r->v + r->off[k], r->len[k], NULL, 0, UINT32_MAX);
        }
    }
    if (degree_overflow) return;
    qsort(mbuf, mn, sizeof(Mono), mono_cmp);
    BP *w = &nlw[q]; bp_clear(w); int affine = 1;
    for (size_t i = 0; i < mn; ) {
        size_t j = i; while (j < mn && !mono_cmp(&mbuf[i], &mbuf[j])) ++j;
        if ((j - i) & 1) { if (mbuf[i].n == 0) c ^= 1; else { bp_add(w, mbuf[i].k, mbuf[i].n); if (mbuf[i].n > 1) affine = 0; } }
        i = j;
    }
    w->c = c;
    if (affine) { Par *p = &wire[q]; p->n = 0; p->c = c;
        if (p->cap < w->n + 1) { p->cap = w->n + 4; p->v = xrealloc(p->v, sizeof(uint32_t) * (size_t)p->cap); }
        for (int i = 0; i < w->n; ++i) p->v[p->n++] = w->v[w->off[i]];   /* sorted: one-variable monomials sort by variable */
        nlon[q] = 0;
    } else nlon[q] = 1;
}
static int any_nonlinear_output(void) { if (!nlon) return 0; for (int q = 0; q < nlw_n; ++q) if (nlon[q]) return 1; return 0; }
/* substitute variable z := q (q must not hold z) in the phase and in every output */
static int64_t *slist; static int scap;
static void substitute(uint32_t z, const BP *q) {
    if (rel_mode) {
        if (z >= nsubst) { size_t o = nsubst; nsubst = 2 * (size_t)z + 64; subst_of = xrealloc(subst_of, sizeof(BP) * nsubst); memset(subst_of + o, 0, sizeof(BP) * (nsubst - o)); }
        BP *d = &subst_of[z]; bp_clear(d); d->c = q->c | 2; for (int i = 0; i < q->n; ++i) bp_add(d, q->v + q->off[i], q->len[i]);
    }
    Occ *o = &occ[z]; int nl = 0;
    if (scap < o->n + 1) { scap = o->n + 16; slist = xrealloc(slist, sizeof(int64_t) * (size_t)scap); }
    for (int i = 0; i < o->n; ++i) { int64_t id = o->v[i]; if (nodes[id].alive && node_has(&nodes[id], z)) slist[nl++] = id; }
    o->n = 0; gone[z] = 1;                                          /* z is about to vanish: solved, not summed */
    uint32_t r[MAXDEG + 1];
    for (int i = 0; i < nl; ++i) {
        int64_t id = slist[i]; if (!nodes[id].alive) continue;
        Node x = nodes[id]; kill_node(id);
        int nr = 0; for (int j = 0; j < x.len; ++j) if (mv[x.off + j] != z) r[nr++] = mv[x.off + j];
        add_times_EB(x.c, r, nr, q);
    }
    if (outcnt[z] && rel_mode && !bp_affine(q)) { for (int i = 0; i < q->nv; ++i) outcnt[q->v[i]]++; }   /* guess mode: pin what an output now depends on */
    else if (outcnt[z] && (!bp_affine(q) || any_nonlinear_output())) {
        for (int w = 0; w < nq && !degree_overflow; ++w) if (wire_has(w, z)) { wire_count(w, -1); wire_subst(w, z, q); wire_count(w, 1); }
    }
    else if (outcnt[z]) {
        Par d = { 0 }; pset1(&d, z); d.c = q->c;
        Par t = { 0 }; for (int i = 0; i < q->n; ++i) { Par one = { 0 }; pset1(&one, q->v[q->off[i]]); pxor(&t, &one); free(one.v); }
        pxor(&d, &t);                                               /* d = z ^ q: xor-ing it removes z, adds q */
        for (int w = 0; w < nq; ++w) if (phas(&wire[w], z)) {
            for (int i = 0; i < wire[w].n; ++i) outcnt[wire[w].v[i]]--;
            pxor(&wire[w], &d);
            for (int i = 0; i < wire[w].n; ++i) outcnt[wire[w].v[i]]++;
        }
        free(d.v); free(t.v);
    }
}
/* y's live monomials: the linear coefficient, and Q (the XOR of m \ y over the others, which must all carry 4) */
static int64_t *ylist; static int ycap;
static int classify(uint32_t y, int *ny, int *lin, BP *Q) {
    Occ *o = &occ[y]; int n = 0, ok = 1, m = 0; *lin = 0; bp_clear(Q);
    if (ycap < o->n + 1) { ycap = o->n + 16; ylist = xrealloc(ylist, sizeof(int64_t) * (size_t)ycap); }
    uint32_t r[MAXDEG + 1];
    for (int i = 0; i < o->n; ++i) {
        int64_t id = o->v[i]; Node *x = &nodes[id]; if (!x->alive || !node_has(x, y)) continue;
        o->v[m++] = id; ylist[n++] = id;                             /* compact the occurrence list */
        if (x->len == 1) *lin = (*lin + x->c) & 7;
        else if (x->c == 4) { int nr = 0; for (int j = 0; j < x->len; ++j) if (mv[x->off + j] != y) r[nr++] = mv[x->off + j]; bp_add(Q, r, nr); }
        else ok = 0;
    }
    o->n = m; *ny = n;
    return ok;
}
static int zstrategy;                                             /* which z to solve for when several qualify */
/* in Q, a path variable z that is one of Q's monomials on its own and appears in no other monomial of Q */
static int pick_z(const BP *Q, uint32_t y, int *which) {
    int best = -1; size_t bcost = (size_t)-1;
    for (int i = 0; i < Q->n; ++i) {
        if (Q->len[i] != 1) continue;
        uint32_t z = Q->v[Q->off[i]];
        if (!is_path(z) || z == y || gone[z]) continue;
        int elsewhere = 0;
        for (int j = 0; j < Q->n && !elsewhere; ++j) if (j != i) for (int l = 0; l < Q->len[j]; ++l) if (Q->v[Q->off[j] + l] == z) { elsewhere = 1; break; }
        if (elsewhere) continue;
        size_t c = zstrategy == 0 ? (size_t)UINT32_MAX - z            /* 0: the latest variable first */
                 : zstrategy == 1 ? (size_t)z                                  /* 1: the earliest first */
                 : (size_t)occ[z].n + (outcnt[z] ? 1000000u : 0);              /* 2: the cheapest substitution, outputs last */
        if (c < bcost) { bcost = c; best = i; }
    }
    if (best >= 0) *which = best;
    return best >= 0;
}
static long rule_elim, rule_hh, rule_omega, omega_nonlinear;
/* Affine substitutions and [omega] first, to a fixed point; then the single cheapest nonlinear step, then affine again.
 * A nonlinear step is either [HH] with a nonlinear Q (cost: occurrences of z times the size of Q) or [omega] with a
 * nonlinear Q (cost: the pairs of Q's monomials that -2 E(Q) writes). Both can multiply monomials, so they go one at a
 * time. [omega] holds for every boolean Q, linear or not: sum_y w^(2y (1 + 2Q)) is 1 + i = sqrt2 w when Q = 0 and
 * 1 - i = sqrt2 w^-1 when Q = 1, so sqrt2 w^(1 - 2Q) either way; E(Q) is Q's exact value mod 8 (add_times_EB). The rule
 * is Amy's [omega] (QPL 2018, https://arxiv.org/abs/1805.06908); applying it to nonlinear Q is ours. */
static long nonlinear_steps;
static int reduce_pass(int nonlinear, BP *Q, BP *R, int *zero_sum) {
    int changed = 1, any = 0;
    uint32_t besty = 0; double bestc = -1; int bestomega = 0;
    while (changed && !degree_overflow && !*zero_sum) {
        changed = 0;
        if (nn > (1u << 20) && hused * 3 < nn) compact();
        for (uint32_t y = (uint32_t)ninputs; y < nvars && !degree_overflow; ++y) {
            if (gone[y] || outcnt[y]) continue;
            int ny, lin; if (!classify(y, &ny, &lin, Q)) continue;
            if (!bp_affine(Q)) {
                if (!nonlinear) continue;
                if (lin == 0 || lin == 4) { int zi; Q->c = lin == 4; if (!pick_z(Q, y, &zi)) continue;
                    double c = (double)occ[Q->v[Q->off[zi]]].n * (double)(Q->n + 1); if (bestc < 0 || c < bestc) { bestc = c; besty = y; bestomega = 0; } }
                else if (lin == 2 || lin == 6) { double c = (double)Q->n * (double)(Q->n + 1) / 2 + 1;
                    if (bestc < 0 || c < bestc) { bestc = c; besty = y; bestomega = 1; } }
                continue;
            }
            if (ny == 0) { gone[y] = 1; hcount -= 2; ++rule_elim; changed = 1; continue; }   /* [Elim] */
            if (lin == 0 || lin == 4) {                              /* [HH]: 4 y Q */
                Q->c = lin == 4;
                int zi = -1;
                if (Q->n == 0) { if (Q->c) { *zero_sum = 1; break; } }  /* sum_y (-1)^y = 0: not a unitary */
                else if (!pick_z(Q, y, &zi)) continue;               /* a constraint no variable solves: no rule */
                for (int i = 0; i < ny; ++i) kill_node(ylist[i]);
                occ[y].n = 0; gone[y] = 1;
                if (zi >= 0) {
                    uint32_t z = Q->v[Q->off[zi]]; bp_clear(R); R->c = Q->c;
                    for (int i = 0; i < Q->n; ++i) if (i != zi) bp_add(R, Q->v + Q->off[i], Q->len[i]);
                    substitute(z, R);
                }
                hcount -= 2; ++rule_hh; changed = 1;
            } else if (lin == 2 || lin == 6) {                       /* [omega]: sum_y w^(2y + 4yQ) = sqrt2 w^(1 - 2Q) */
                Q->c = lin == 6;
                for (int i = 0; i < ny; ++i) kill_node(ylist[i]);
                occ[y].n = 0; gone[y] = 1;
                constant = (constant + 1) & 7;
                add_times_EB(6, NULL, 0, Q);                        /* -2 E(Q) */
                hcount -= 1; ++rule_omega; changed = 1;
            }
            any |= changed;
        }
    }
    if (nonlinear && bestc >= 0 && bestomega && !*zero_sum && !degree_overflow) {   /* the one nonlinear [omega] chosen above */
        uint32_t y = besty; int ny, lin;
        if (!gone[y] && !outcnt[y] && classify(y, &ny, &lin, Q) && (lin == 2 || lin == 6)) { Q->c = lin == 6;
            for (int i = 0; i < ny; ++i) kill_node(ylist[i]);
            occ[y].n = 0; gone[y] = 1;
            constant = (constant + 1) & 7;
            add_times_EB(6, NULL, 0, Q);                            /* -2 E(Q); degree past MAXDEG refuses the run */
            hcount -= 1; ++rule_omega; ++omega_nonlinear; ++nonlinear_steps; any = 1;
        } }
    else if (nonlinear && bestc >= 0 && !*zero_sum && !degree_overflow) {     /* the one nonlinear [HH] chosen above */
        uint32_t y = besty; int ny, lin, zi;
        if (classify(y, &ny, &lin, Q) && (lin == 0 || lin == 4)) { Q->c = lin == 4;
            if (pick_z(Q, y, &zi)) {
                for (int i = 0; i < ny; ++i) kill_node(ylist[i]);
                occ[y].n = 0; gone[y] = 1;
                uint32_t z = Q->v[Q->off[zi]]; bp_clear(R); R->c = Q->c;
                for (int i = 0; i < Q->n; ++i) if (i != zi) bp_add(R, Q->v + Q->off[i], Q->len[i]);
                substitute(z, R);
                hcount -= 2; ++rule_hh; ++nonlinear_steps; any = 1;
            } } }
    return any;
}
static int reduce_all(void) {
    BP Q = { 0 }, R = { 0 }; int zero_sum = 0;
    for (;;) {
        reduce_pass(0, &Q, &R, &zero_sum);
        if (zero_sum || degree_overflow) break;
        if (!reduce_pass(1, &Q, &R, &zero_sum)) break;
        if (zero_sum || degree_overflow) break;
    }
    bp_free(&Q); bp_free(&R);
    return zero_sum ? -1 : 0;
}

/* ---------------- equivalence ---------------- */
enum { EQUAL, NOT_EQUAL, UNKNOWN, REFUSED };
static const char *vname[] = { "EQUAL", "NOT_EQUAL", "UNKNOWN", "REFUSED" };
static int zero_ancillas;
static void reset(void) {
    for (size_t i = 0; i < nocc; ++i) free(occ[i].v);
    free(occ); occ = NULL; nocc = 0; free(outcnt); outcnt = NULL; free(gone); gone = NULL;
    free(nodes); nodes = NULL; nn = ncap = 0; free(ht); ht = NULL; hcap = hused = 0; free(mv); mv = NULL; mvn = mvcap = 0;
    constant = 0; degree_overflow = 0; nvars = ninputs = 0; hcount = 0; rule_elim = rule_hh = rule_omega = omega_nonlinear = 0; narrowings = 0; nonlinear_steps = 0; peak_live = 0;
    if (wire) { for (int q = 0; q < nq; ++q) free(wire[q].v); free(wire); wire = NULL; }
    if (nlw) { for (int q = 0; q < nlw_n; ++q) bp_free(&nlw[q]); free(nlw); nlw = NULL; free(nlon); nlon = NULL; nlw_n = 0; }
}
/* Refutation when path variables remain: if B^-1 A were c I (|c| = 1), every diagonal entry <x|B^-1 A|x> would equal
 * the same c. Each entry is computed exactly, 2^(-h/2) sum over the remaining path variables of w^P where the outputs
 * equal x, as an element of Z[w]. An entry of modulus other than 1, or two entries that differ, proves NOT_EQUAL. Only
 * inputs the judged space allows are tried (ancillas at 0 under --zero-ancillas). Finding no witness proves nothing:
 * the verdict then stays UNKNOWN. */
static uint64_t rng = 0x9E3779B97F4A7C15ULL;
static uint64_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }
static int refute(const Circ *A, char *why, size_t wl) {
    if (any_nonlinear_output()) return 0;                          /* the diagonal-entry walk reads outputs as parities */
    int r = 0; int *ridx = calloc(nvars + 1, sizeof(int)); if (!ridx) die("out of memory");
    for (size_t y = ninputs; y < nvars; ++y) if (!gone[y]) ridx[y] = ++r;
    if (r > 22 || hcount < 0 || hcount > 60) { free(ridx); return 0; }
    size_t live = 0; for (size_t i = 0; i < nn; ++i) if (nodes[i].alive) ++live;
    int nin = 0; for (int q = 0; q < nq; ++q) if (!(zero_ancillas && A->has_i && !A->isin[q])) ++nin;
    int ntry = nin <= 8 ? 1 << nin : 48;
    double work = (double)(live + (size_t)nq) * (double)((size_t)1 << r) * ntry;
    if (work > 4e9) { free(ridx); return 0; }
    unsigned char *x = calloc((size_t)nq, 1); uint32_t *mask = malloc(sizeof(uint32_t) * (live + 1)); int *cf = malloc(sizeof(int) * (live + 1));
    uint32_t *wmask = malloc(sizeof(uint32_t) * (size_t)nq); int *wval = malloc(sizeof(int) * (size_t)nq);
    if (!x || !mask || !cf || !wmask || !wval) die("out of memory");
    int64_t ref[4] = { 0 }; int have = 0, out = 0;
    for (int t = 0; t < ntry && !out; ++t) {
        int bit = 0;
        for (int q = 0; q < nq; ++q) {
            if (zero_ancillas && A->has_i && !A->isin[q]) { x[q] = 0; continue; }
            x[q] = nin <= 8 ? (unsigned char)((t >> bit++) & 1) : (unsigned char)(t == 0 ? 0 : t == 1 ? 1 : (rnd() >> 11) & 1);
        }
        int m = 0, c0 = (int)constant;
        for (size_t i = 0; i < nn; ++i) if (nodes[i].alive) {
            uint32_t mk = 0; int on = 1;
            for (int j = 0; j < nodes[i].len && on; ++j) { uint32_t v = mv[nodes[i].off + j]; if (v < ninputs) on = x[v]; else mk |= 1u << (ridx[v] - 1); }
            if (!on) continue;
            if (!mk) c0 += nodes[i].c; else { mask[m] = mk; cf[m++] = nodes[i].c; }
        }
        for (int q = 0; q < nq; ++q) { wmask[q] = 0; wval[q] = wire[q].c ^ x[q];
            for (int i = 0; i < wire[q].n; ++i) { uint32_t v = wire[q].v[i]; if (v < ninputs) wval[q] ^= x[v]; else wmask[q] |= 1u << (ridx[v] - 1); } }
        int64_t cnt[8] = { 0 };
        for (uint32_t y = 0; y < (1u << r); ++y) {
            int okq = 1; for (int q = 0; q < nq && okq; ++q) if ((__builtin_popcount(y & wmask[q]) ^ wval[q]) & 1) okq = 0;
            if (!okq) continue;
            int e = c0; for (int i = 0; i < m; ++i) if ((y & mask[i]) == mask[i]) e += cf[i];
            cnt[e & 7]++;
        }
        int64_t S[4] = { cnt[0] - cnt[4], cnt[1] - cnt[5], cnt[2] - cnt[6], cnt[3] - cnt[7] };
        /* |S|^2 = S conj(S): the rational part and the sqrt2 part (w + w^-1 = sqrt2, w^2 + w^-2 = 0, w^3 + w^-3 = -sqrt2) */
        int64_t n0 = S[0]*S[0] + S[1]*S[1] + S[2]*S[2] + S[3]*S[3];
        int64_t n1 = S[0]*S[1] + S[1]*S[2] + S[2]*S[3] - S[3]*S[0];
        int64_t want = (int64_t)1 << hcount;
        if (n0 != want || n1 != 0) { snprintf(why, wl, "input %d: the diagonal entry of B^-1 A has modulus other than 1 (|sum|^2 = %lld + %lld sqrt2, 2^h = %lld; %d path variables left)", t, (long long)n0, (long long)n1, (long long)want, r); out = 1; break; }
        if (!have) { memcpy(ref, S, sizeof ref); have = 1; }
        else if (memcmp(ref, S, sizeof ref)) { snprintf(why, wl, "two inputs give different diagonal entries of B^-1 A (%d path variables left)", r); out = 1; }
    }
    free(x); free(mask); free(cf); free(wmask); free(wval); free(ridx);
    return out;
}
static int equiv1(const Circ *A, const Circ *B, char *why, size_t wl) {
    if (A->n != B->n) { snprintf(why, wl, "qubit counts differ (%d vs %d)", A->n, B->n); return NOT_EQUAL; }
    for (int i = 0; i < A->n; ++i) if (strcmp(A->name[i], B->name[i])) { snprintf(why, wl, "qubit %d is '%s' in one and '%s' in the other", i, A->name[i], B->name[i]); return REFUSED; }
    if (zero_ancillas && (A->has_i != B->has_i || memcmp(A->isin, B->isin, (size_t)A->n))) { snprintf(why, wl, "--zero-ancillas: the two files declare different inputs (.i)"); return REFUSED; }
    reset(); rehash();
    nq = A->n; wire = calloc((size_t)nq, sizeof(Par)); if (!wire) die("out of memory");
    for (int q = 0; q < nq; ++q) { uint32_t v = newvar(); if (!(zero_ancillas && A->has_i && !A->isin[q])) pset1(&wire[q], v); }
    ninputs = nvars;
    for (int i = 0; i < A->ng && !degree_overflow; ++i) gate(&A->g[i], 0);
    for (int i = B->ng - 1; i >= 0 && !degree_overflow; --i) gate(&B->g[i], 1);
    for (int q = 0; q < nq; ++q) for (int i = 0; i < wire[q].n; ++i) outcnt[wire[q].v[i]]++;
    size_t v0 = nvars - ninputs;
    if (degree_overflow) { snprintf(why, wl, "a phase monomial of degree above %d (refusing rather than truncating)", MAXDEG); return REFUSED; }
    int zs = reduce_all();
    if (degree_overflow) { snprintf(why, wl, "a phase monomial of degree above %d appeared during reduction (refusing rather than truncating)", MAXDEG); return REFUSED; }
    if (zs < 0) { snprintf(why, wl, "a path sum summed to zero (not a unitary)"); return UNKNOWN; }
    size_t left = 0; for (size_t y = ninputs; y < nvars; ++y) if (!gone[y]) ++left;
    char stats[240]; snprintf(stats, sizeof stats, "%zu path variables; elim %ld, HH %ld, omega %ld (of them nonlinear %ld), nonlinear steps %ld, peak monomials %zu", v0, rule_elim, rule_hh, rule_omega, omega_nonlinear, nonlinear_steps, peak_live);
    if (left) { if (refute(A, why, wl)) return NOT_EQUAL; snprintf(why, wl, "%zu path variables left unreduced (%s)", left, stats); return UNKNOWN; }
    int ident = hcount == 0;
    for (int q = 0; q < nq && ident; ++q) {
        int anc = zero_ancillas && A->has_i && !A->isin[q];
        if (nlon && nlon[q]) ident = 0;
        else if (wire[q].c) ident = 0;
        else if (anc) ident = wire[q].n == 0;
        else ident = wire[q].n == 1 && wire[q].v[0] == (uint32_t)q;
    }
    size_t live = 0; for (size_t i = 0; i < nn; ++i) if (nodes[i].alive) ++live;
    if (ident && live == 0) { snprintf(why, wl, "B^-1 A reduces to a global phase w^%d times the identity (%s)", (int)constant, stats); return EQUAL; }
    snprintf(why, wl, "B^-1 A reduces to a classical map that is not a scalar: %s (%s)", !ident ? "outputs differ from inputs" : "a nonconstant phase remains", stats);
    return NOT_EQUAL;
}
/* The rules are sound in any order, but a miter can stick under one choice of substituted variable and reduce under
 * another: an UNKNOWN is retried with the other two choices before it is reported. */
static int equiv(const Circ *A, const Circ *B, char *why, size_t wl) {
    int v = UNKNOWN;
    for (zstrategy = 0; zstrategy < 3; ++zstrategy) { v = equiv1(A, B, why, wl); if (v != UNKNOWN) break; }
    zstrategy = 0;
    return v;
}


/* ---------------- guess: Allegedly (Confirmed), the guessing half ----------------
 * The circuit's own path sum is reduced with the same rules, remembering every substitution z := Q. A substitution
 * holds wherever the summed variable's delta holds, so two phase gates whose parities differ as polynomials may still
 * be the same function on every path that contributes. Each T-gate's parity is evaluated on 256 random assignments of
 * the inputs and the variables left free, the substituted ones computed from their Q; gates that agree (or are exact
 * complements) on every sample are printed as MERGE candidates. A guess is only a guess: phaseopt applies it and
 * keeps it only when `equiv` proves the result (this mode proves nothing). */
static int guess(const Circ *A) {
    rel_mode = 1; reset(); rehash();
    nq = A->n; wire = calloc((size_t)nq, sizeof(Par)); if (!wire) die("out of memory");
    for (int q = 0; q < nq; ++q) { uint32_t v = newvar(); if (!(zero_ancillas && A->has_i && !A->isin[q])) pset1(&wire[q], v); }   /* as equiv1 */
    ninputs = nvars;
    for (int i = 0; i < A->ng && !degree_overflow; ++i) { cur_gate = i; gate(&A->g[i], 0); }
    for (int q = 0; q < nq; ++q) for (int i = 0; i < wire[q].n; ++i) outcnt[wire[q].v[i]]++;
    if (degree_overflow) { printf("REFUSED (degree)\n"); return 3; }
    reduce_all();
    uint64_t *val = calloc(nvars + 1, sizeof(uint64_t)); unsigned char *st = calloc(nvars + 1, 1);
    uint64_t (*fp)[4] = calloc((size_t)npterms + 1, sizeof *fp); unsigned char *bad = calloc((size_t)npterms + 1, 1);
    if (!val || !st || !fp || !bad) die("out of memory");
    for (int r = 0; r < 4; ++r) {
        memset(st, 0, nvars);
        for (size_t v = 0; v < nvars; ++v) if (v < ninputs || !gone[v]) { val[v] = rnd() ^ (rnd() << 23); st[v] = 1; }
        for (int pass = 0, prog = 1; pass < 256 && prog; ++pass) { prog = 0;
            for (size_t v = ninputs; v < nvars && v < nsubst; ++v) if (!st[v] && (subst_of[v].c & 2)) {
                BP *d = &subst_of[v]; int ok = 1; uint64_t acc = (d->c & 1) ? ~0ULL : 0;
                for (int i = 0; i < d->n && ok; ++i) { uint64_t m = ~0ULL; for (int l = 0; l < d->len[i]; ++l) { uint32_t u = d->v[d->off[i] + l]; if (!st[u]) { ok = 0; break; } m &= val[u]; } acc ^= m; }
                if (ok) { val[v] = acc; st[v] = 1; prog = 1; } } }
        for (int t = 0; t < npterms; ++t) { uint64_t acc = pterms[t].p.c ? ~0ULL : 0;
            for (int i = 0; i < pterms[t].p.n; ++i) { uint32_t u = pterms[t].p.v[i]; if (!st[u]) { bad[t] = 1; break; } acc ^= val[u]; }
            fp[t][r] = acc; }
    }
    int cand = 0, ok = 0; char *used = calloc((size_t)npterms + 1, 1);
    for (int t = 0; t < npterms; ++t) ok += !bad[t];
    for (int a = 0; a < npterms; ++a) if (!bad[a] && !used[a]) for (int b = a + 1; b < npterms; ++b) if (!bad[b] && !used[b]) {
        int eq = 1, cp = 1; for (int r = 0; r < 4; ++r) { if (fp[a][r] != fp[b][r]) eq = 0; if (fp[a][r] != ~fp[b][r]) cp = 0; }
        if (!eq && !cp) continue;
        int same = pterms[a].p.n == pterms[b].p.n && !memcmp(pterms[a].p.v, pterms[b].p.v, sizeof(uint32_t) * (size_t)pterms[a].p.n);
        if (same) continue;                                         /* phase folding already merges these */
        printf("MERGE %d %d %s\n", pterms[a].gi, pterms[b].gi, eq ? "same" : "complement"); used[b] = 1; ++cand;
    }
    printf("GUESSES %d (T gates %d, evaluable on every sample %d)\n", cand, npterms, ok);
    free(val); free(st); free(fp); free(bad); free(used); rel_mode = 0;
    return 0;
}

/* ---------------- selftest ---------------- */
static int write_tmp(char *path, const char *body) {
    snprintf(path, 64, "/tmp/pathverify_XXXXXX"); int fd = mkstemp(path); if (fd < 0) return 0;
    FILE *f = fdopen(fd, "w"); fputs(body, f); fclose(f); return 1;
}
static void cfree(Circ *C) { free(C->g); free(C->name); free(C->isin); }
static int check(const char *name, const char *a, const char *b, int want) {
    char pa[64], pb[64], why[512]; Circ A, B;
    if (!write_tmp(pa, a) || !write_tmp(pb, b)) die("cannot write a temporary file");
    int ok = parse(pa, &A, why, sizeof why) && parse(pb, &B, why, sizeof why);
    int got = ok ? equiv(&A, &B, why, sizeof why) : REFUSED;
    int pass = got == want;
    printf("%-40s want %-9s got %-9s %s  (%s)\n", name, vname[want], vname[got], pass ? "ok" : "FAIL", why);
    remove(pa); remove(pb); if (ok) { cfree(&A); cfree(&B); }
    return pass;
}
/* [omega] with a nonlinear Q, checked against brute force (OPT-PATHVERIFY-NONLINEAR-OMEGA-20261022). Inputs a, b, c are
 * variables 0..2 and y is path variable 3. The phase is a background B(a, b, c) plus lin y plus 4 y Q(a, b, c); the
 * reducer's nonlinear pass runs on it as it would inside a miter. For every one of the 8 inputs, sum over y of w^P is
 * computed exactly in Z[w] and compared with sqrt2 w^(P'), P' the phase the reducer left (sqrt2 = w + w^7). Nothing is
 * floating point. want = 1: y must be summed out and every input must agree. want = 0: y must stay (no rule applies).
 * want = 2: the run must end in degree_overflow (the refusal), never in a truncated phase. */
static void zw_add(int64_t *z, int k, int64_t m) { k &= 7; if (k >= 4) { k -= 4; m = -m; } z[k] += m; }
static int64_t phase_at(unsigned x) {                               /* the live phase polynomial at bits x (y = bit 3) */
    int64_t p = constant;
    for (size_t i = 0; i < nn; ++i) { if (!nodes[i].alive) continue; int on = 1;
        for (int j = 0; j < nodes[i].len; ++j) if (!((x >> mv[nodes[i].off + j]) & 1)) { on = 0; break; }
        if (on) p += nodes[i].c; }
    return p & 7;
}
typedef struct { const char *name; int lin, ny_out; int nq; uint32_t q[6][4]; int ql[6]; int qc; int cross; int want; } OmegaCase;
static int omega_case(const OmegaCase *t) {
    reset(); rehash();
    for (int i = 0; i < 4; ++i) newvar();
    ninputs = 3; hcount = 1;
    uint32_t a = 0, b = 1, c = 2, y = 3;
    { uint32_t m1[1] = { a }, m2[2] = { b, c }; madd(m1, 1, 1); madd(m2, 2, 3); constant = 5; }   /* background */
    { uint32_t m[1] = { y }; madd(m, 1, t->lin); }
    if (t->qc) { uint32_t m[1] = { y }; madd(m, 1, 4); }
    for (int i = 0; i < t->nq; ++i) { uint32_t m[8]; int l = t->ql[i]; memcpy(m, t->q[i], sizeof(uint32_t) * (size_t)l); m[l] = y; madd(m, l + 1, 4); }
    if (t->cross) { uint32_t m[2] = { a, y }; madd(m, 2, 2); }       /* coefficient 2 on a y: no rule may fire */
    if (t->ny_out) outcnt[y] = 1;                                   /* y in an output: no rule may fire */
    int64_t before[8][4]; memset(before, 0, sizeof before);
    for (unsigned x = 0; x < 8; ++x) for (unsigned yy = 0; yy < 2; ++yy) zw_add(before[x], (int)phase_at(x | yy << 3), 1);
    BP Q = { 0 }, R = { 0 }; int zs = 0;
    reduce_pass(1, &Q, &R, &zs);
    bp_free(&Q); bp_free(&R);
    int got, agree = 1;
    if (degree_overflow) got = 2;
    else if (!gone[y]) got = 0;
    else {
        got = 1;
        for (unsigned x = 0; x < 8 && agree; ++x) {                 /* y is gone: the phase no longer reads bit 3 */
            int64_t after[4] = { 0, 0, 0, 0 }; int k = (int)phase_at(x);
            zw_add(after, k + 1, 1); zw_add(after, k + 7, 1);      /* sqrt2 w^k */
            if (memcmp(after, before[x], sizeof after) || hcount != 0) agree = 0;
        }
    }
    int pass = got == t->want && agree;
    printf("%-40s want %-9s got %-9s %s  (omega rules %ld, nonlinear %ld)\n", t->name,
           t->want == 1 ? "SUMMED" : t->want == 0 ? "KEPT" : "REFUSED", got == 1 ? (agree ? "SUMMED" : "WRONG") : got == 0 ? "KEPT" : "REFUSED",
           pass ? "ok" : "FAIL", rule_omega, omega_nonlinear);
    return pass;
}
/* the growth boundary: Q = (x0..x24) + (x25..x49), each monomial within MAXDEG, but -2 E(Q) writes their product, of
 * degree 50 > MAXDEG. The run must stop at degree_overflow (REFUSED in a miter), not keep a truncated phase. */
static int omega_refusal_control(void) {
    reset(); rehash();
    for (int i = 0; i < 51; ++i) newvar();
    ninputs = 50; hcount = 1; uint32_t y = 50, m[26];
    { uint32_t l[1] = { y }; madd(l, 1, 2); }
    for (int h = 0; h < 2; ++h) { for (int i = 0; i < 25; ++i) m[i] = (uint32_t)(25 * h + i); m[25] = y; madd(m, 26, 4); }
    BP Q = { 0 }, R = { 0 }; int zs = 0; reduce_pass(1, &Q, &R, &zs); bp_free(&Q); bp_free(&R);
    int pass = degree_overflow == 1;
    printf("%-40s want %-9s got %-9s %s\n", "omega nonlinear past MAXDEG refuses", "REFUSED", degree_overflow ? "REFUSED" : "NOT", pass ? "ok" : "FAIL");
    reset(); return pass;
}
/* nonlinear outputs (wire_subst), checked by truth table over every assignment. Inputs a b c are variables 0..2, z and
 * d are path variables 3 and 4, one output wire starts as a + z. z := ab + d + 1 makes it nonlinear (it must be held as
 * a polynomial, agree with a + ab + d + 1 everywhere and count a once); d := ab + c + 1 makes it affine again (a + c,
 * back to a parity, z and d no longer counted as outputs). A substitution writing a monomial past MAXDEG must end in degree_overflow. */
static int wire_eval(int q, unsigned x) {
    if (nlon && nlon[q]) { const BP *w = &nlw[q]; int v = w->c & 1;
        for (int i = 0; i < w->n; ++i) { int on = 1; for (int j = 0; j < w->len[i]; ++j) if (!((x >> w->v[w->off[i] + j]) & 1)) on = 0; v ^= on; }
        return v; }
    int v = wire[q].c & 1; for (int i = 0; i < wire[q].n; ++i) v ^= (x >> wire[q].v[i]) & 1; return v;
}
static int nlout_controls(void) {
    int all = 1, ok;
    reset(); rehash(); for (int i = 0; i < 5; ++i) newvar(); ninputs = 3;
    nq = 1; wire = calloc(1, sizeof(Par)); if (!wire) die("out of memory");
    { Par t = { 0 }; pset1(&wire[0], 0); pset1(&t, 3); pxor(&wire[0], &t); free(t.v); }
    wire_count(0, 1);
    BP R = { 0 };
    { uint32_t ab[2] = { 0, 1 }, d[1] = { 4 }; bp_add(&R, ab, 2); bp_add(&R, d, 1); R.c = 1; }
    substitute(3, &R);
    ok = nlon && nlon[0] && outcnt[3] == 0 && outcnt[4] == 1 && outcnt[0] == 1 && !degree_overflow;
    for (unsigned x = 0; x < 32 && ok; ++x) { int a = x & 1, b = (x >> 1) & 1, d = (x >> 4) & 1; if (wire_eval(0, x) != (a ^ (a & b) ^ d ^ 1)) ok = 0; }
    printf("%-40s %s\n", "nonlinear output: z := ab + d + 1 is held", ok ? "ok" : "FAIL"); all &= ok;
    bp_clear(&R); { uint32_t ab[2] = { 0, 1 }, c[1] = { 2 }; bp_add(&R, ab, 2); bp_add(&R, c, 1); R.c = 1; }
    substitute(4, &R);
    ok = nlon && !nlon[0] && outcnt[4] == 0 && outcnt[0] == 1 && outcnt[2] == 1 && wire[0].n == 2 && wire[0].c == 0 && !degree_overflow;
    for (unsigned x = 0; x < 32 && ok; ++x) { int a = x & 1, c = (x >> 2) & 1; if (wire_eval(0, x) != (a ^ c)) ok = 0; }
    printf("%-40s %s\n", "nonlinear output: back to a parity", ok ? "ok" : "FAIL"); all &= ok;
    reset(); rehash(); for (int i = 0; i < 44; ++i) newvar(); ninputs = 42;
    nq = 1; wire = calloc(1, sizeof(Par)); if (!wire) die("out of memory");
    { Par t = { 0 }; pset1(&wire[0], 42); pset1(&t, 0); pxor(&wire[0], &t); free(t.v); } wire_count(0, 1);
    bp_clear(&R); { uint32_t m[41]; for (int i = 0; i < 41; ++i) m[i] = (uint32_t)i; bp_add(&R, m, 41); bp_add(&R, m, 1); R.c = 0; }
    substitute(42, &R);
    ok = degree_overflow == 1;
    printf("%-40s %s\n", "nonlinear output past MAXDEG refuses", ok ? "ok" : "FAIL"); all &= ok;
    bp_free(&R); reset();
    return all;
}
static int omega_controls(void) {
    static const OmegaCase cs[] = {
        { "omega nonlinear: Q = ab, lin 2",          2, 0, 1, { { 0, 1 } }, { 2 }, 0, 0, 1 },
        { "omega nonlinear: Q = ab, lin 6",          6, 0, 1, { { 0, 1 } }, { 2 }, 0, 0, 1 },
        { "omega nonlinear: Q = ab + c, lin 2",      2, 0, 2, { { 0, 1 }, { 2 } }, { 2, 1 }, 0, 0, 1 },
        { "omega nonlinear: Q = ab + c, lin 6",      6, 0, 2, { { 0, 1 }, { 2 } }, { 2, 1 }, 0, 0, 1 },
        { "omega nonlinear: Q = ab + bc, lin 2",     2, 0, 2, { { 0, 1 }, { 1, 2 } }, { 2, 2 }, 0, 0, 1 },
        { "omega nonlinear: Q = ab + abc + a, lin 6", 6, 0, 3, { { 0, 1 }, { 0, 1, 2 }, { 0 } }, { 2, 3, 1 }, 0, 0, 1 },
        { "omega nonlinear: Q = 1+ab+bc+ac, lin 2",  2, 0, 3, { { 0, 1 }, { 1, 2 }, { 0, 2 } }, { 2, 2, 2 }, 1, 0, 1 },
        { "omega nonlinear: Q = 1+ab+bc+ac, lin 6",  6, 0, 3, { { 0, 1 }, { 1, 2 }, { 0, 2 } }, { 2, 2, 2 }, 1, 0, 1 },
        { "CHEAT omega: y is an output",             2, 1, 1, { { 0, 1 } }, { 2 }, 0, 0, 0 },
        { "CHEAT omega: cross coefficient 2 ay",     2, 0, 1, { { 0, 1 } }, { 2 }, 0, 1, 0 },
    };
    int all = 1;
    for (size_t i = 0; i < sizeof cs / sizeof cs[0]; ++i) all &= omega_case(&cs[i]);
    reset();
    all &= omega_refusal_control();
    return all;
}
#define HDR3 ".v a b c\nBEGIN\n"
#define TOF7 "H c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT* c\ntof a c\nT b\nT c\nH c\ntof a b\nT a\nT* b\ntof a b\n"
static int selftest(void) {
    int all = 1;
    all &= check("identity: H H = nothing", HDR3 "H a\nH a\nEND\n", HDR3 "END\n", EQUAL);
    all &= check("T T = S", HDR3 "T a\nT a\nEND\n", HDR3 "S a\nEND\n", EQUAL);
    all &= check("S S = Z", HDR3 "S a\nS a\nEND\n", HDR3 "Z a\nEND\n", EQUAL);
    all &= check("HZH = X", HDR3 "H a\nZ a\nH a\nEND\n", HDR3 "X a\nEND\n", EQUAL);
    all &= check("HSH S H S = global phase (omega rule)", HDR3 "H a\nS a\nH a\nS a\nH a\nS a\nEND\n", HDR3 "END\n", EQUAL);
    all &= check("global phase: XZXZ = -I", HDR3 "X a\nZ a\nX a\nZ a\nEND\n", HDR3 "END\n", EQUAL);
    all &= check("Toffoli = its 7-T circuit", HDR3 "tof a b c\nEND\n", HDR3 TOF7 "END\n", EQUAL);
    all &= check("CCZ = H tof H", HDR3 "Z a b c\nEND\n", HDR3 "H c\ntof a b c\nH c\nEND\n", EQUAL);
    all &= check("CZ = H cnot H", HDR3 "Z a b\nEND\n", HDR3 "H b\ntof a b\nH b\nEND\n", EQUAL);
    all &= check("Y = i X Z", HDR3 "Y a\nEND\n", HDR3 "Z a\nX a\nEND\n", EQUAL);
    all &= check("C3X = Barenco with one dirty line", ".v a b c d t\nBEGIN\ntof a b c t\nEND\n", ".v a b c d t\nBEGIN\ntof c d t\ntof a b d\ntof c d t\ntof a b d\nEND\n", EQUAL);
    all &= check("CHEAT C3X is not C2X", ".v a b c d t\nBEGIN\ntof a b c t\nEND\n", ".v a b c d t\nBEGIN\ntof a b t\nEND\n", NOT_EQUAL);
    all &= check("multi-Z is symmetric", ".v a b c d\nBEGIN\nZ a b c d\nEND\n", ".v a b c d\nBEGIN\nZ d c b a\nEND\n", EQUAL);
    all &= check("swap = 3 CNOTs", HDR3 "swap a b\nEND\n", HDR3 "tof a b\ntof b a\ntof a b\nEND\n", EQUAL);
    all &= check("H-conjugated CNOT reverses it", HDR3 "H a\nH b\ntof a b\nH a\nH b\nEND\n", HDR3 "tof b a\nEND\n", EQUAL);
    all &= check("T commutes with CNOT control", HDR3 "T a\ntof a b\nEND\n", HDR3 "tof a b\nT a\nEND\n", EQUAL);
    all &= check("two Toffolis cancel (T-level)", HDR3 TOF7 TOF7 "END\n", HDR3 "END\n", EQUAL);
    /* planted cheaters: each must be caught (NOT_EQUAL) or at worst left UNKNOWN; never EQUAL */
    all &= check("CHEAT relative phase: Z is not I", HDR3 "Z a\nEND\n", HDR3 "END\n", NOT_EQUAL);
    all &= check("CHEAT T is not S", HDR3 "T a\nEND\n", HDR3 "S a\nEND\n", NOT_EQUAL);
    all &= check("CHEAT T is not T*", HDR3 "T a\nEND\n", HDR3 "T* a\nEND\n", NOT_EQUAL);
    all &= check("CHEAT dropped T in Toffoli", HDR3 "tof a b c\nEND\n", HDR3 "H c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT* c\ntof a c\nT b\nH c\ntof a b\nT a\nT* b\ntof a b\nEND\n", NOT_EQUAL);
    all &= check("CHEAT T flipped to T* in Toffoli", HDR3 "tof a b c\nEND\n", HDR3 "H c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT c\ntof a c\nT b\nT c\nH c\ntof a b\nT a\nT* b\ntof a b\nEND\n", NOT_EQUAL);
    all &= check("CHEAT wrong target", HDR3 "tof a b c\nEND\n", HDR3 "tof a c b\nEND\n", NOT_EQUAL);
    all &= check("CHEAT dropped control", HDR3 "tof a b c\nEND\n", HDR3 "tof b c\nEND\n", NOT_EQUAL);
    all &= check("CHEAT relative-phase Toffoli", HDR3 "tof a b c\nEND\n", HDR3 "H c\nT c\ntof b c\nT* c\ntof a c\nT c\ntof b c\nT* c\nH c\nEND\n", NOT_EQUAL);
    all &= check("CHEAT H is not I", HDR3 "H a\nEND\n", HDR3 "END\n", NOT_EQUAL);
    all &= check("CHEAT HSH is not S", HDR3 "H a\nS a\nH a\nEND\n", HDR3 "S a\nEND\n", NOT_EQUAL);
    all &= check("CHEAT CNOT direction", HDR3 "tof a b\nEND\n", HDR3 "tof b a\nEND\n", NOT_EQUAL);
    all &= check("CHEAT Y is not X", HDR3 "Y a\nEND\n", HDR3 "X a\nEND\n", NOT_EQUAL);
    /* zero ancillas: c starts at 0, so a Toffoli onto c then cnot-ing c into... a T on c is invisible */
    zero_ancillas = 1;
    all &= check("zero-ancilla: T on a 0 ancilla is nothing", ".v a b c\n.i a b\nBEGIN\nT c\nEND\n", ".v a b c\n.i a b\nBEGIN\nEND\n", EQUAL);
    all &= check("CHEAT zero-ancilla: X on the ancilla", ".v a b c\n.i a b\nBEGIN\nX c\nEND\n", ".v a b c\n.i a b\nBEGIN\nEND\n", NOT_EQUAL);
    all &= check("CHEAT zero-ancilla: T on an input", ".v a b c\n.i a b\nBEGIN\nT a\nEND\n", ".v a b c\n.i a b\nBEGIN\nEND\n", NOT_EQUAL);
    zero_ancillas = 0;
    all &= check("full unitary: T on c is not nothing", ".v a b c\n.i a b\nBEGIN\nT c\nEND\n", ".v a b c\n.i a b\nBEGIN\nEND\n", NOT_EQUAL);
    all &= omega_controls();
    all &= nlout_controls();
    {   /* operand narrowing: nine controls, each a 2-variable parity (product 512 > PROD_LIMIT), must be narrowed and stay exact */
        const char *pre = ".v a b c d e f g h i j k l m n o p q r t\nBEGIN\ntof b a\ntof d c\ntof f e\ntof h g\ntof j i\ntof l k\ntof n m\ntof p o\ntof r q\n";
        const char *post = "tof b a\ntof d c\ntof f e\ntof h g\ntof j i\ntof l k\ntof n m\ntof p o\ntof r q\nEND\n";
        char A[1024], B[1024], C[1024];
        snprintf(A, sizeof A, "%sZ a c e g i k m o q\n%s", pre, post);
        snprintf(B, sizeof B, "%sH q\ntof a c e g i k m o q\nH q\n%s", pre, post);
        snprintf(C, sizeof C, "%sZ a c e g i k m o\n%s", pre, post);
        all &= check("narrowed: wide-parity MCZ = H MCX H", A, B, EQUAL);
        int fired = narrowings > 0; printf("%-40s %s\n", "narrowed: the narrowing was reached", fired ? "ok" : "FAIL"); all &= fired;
        all &= check("CHEAT narrowed: a control dropped", A, C, NOT_EQUAL);
    }
    printf(all ? "selftest: every control holds\n" : "selftest: FAILED\n");
    return all ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--zero-ancillas")) { zero_ancillas = 1; ++argv; --argc; }
    if (argc == 2 && !strcmp(argv[1], "selftest")) return selftest();
    if (argc == 3 && !strcmp(argv[1], "guess")) { Circ A; char why[512]; if (!parse(argv[2], &A, why, sizeof why)) { printf("REFUSED (%s)\n", why); return 3; } return guess(&A); }
    if (argc == 4 && !strcmp(argv[1], "equiv")) {
        Circ A, B; char why[512];
        if (!parse(argv[2], &A, why, sizeof why) || !parse(argv[3], &B, why, sizeof why)) { printf("REFUSED (%s)\n", why); return 3; }
        int v = equiv(&A, &B, why, sizeof why);
        printf("%s (%s)\n", vname[v], why);
        return v == EQUAL ? 0 : v == NOT_EQUAL ? 1 : v == UNKNOWN ? 2 : 3;
    }
    fprintf(stderr, "usage: pathverify [--zero-ancillas] equiv A.qc B.qc | guess A.qc | selftest\n");
    return 3;
}
