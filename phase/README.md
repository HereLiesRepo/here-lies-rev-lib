# The phase lane (started 2026-10-22, T+50:16)

Reversible circuits have no phase gates: on a 0 or a 1 a phase does nothing you can see. Quantum circuits do, and the
expensive one is T. This lane takes HereLiesRevLib's rules to the T-count literature: the papers' own circuits, their
own numbers, and nothing counts without an exact proof.

## What exists

- `../verifier/phaseverify.c`: exact equivalence of two Clifford+T circuits up to one global phase, by simulating
  every column. Every amplitude is held exactly as x / sqrt(2)^k with x in Z[w], w = e^(i pi/4), now in 128-bit
  integers: no floating point reaches a verdict, and overflow ends a run REFUSED, never wrapped. `phaseverify
  selftest` runs 17 controls, 9 of them planted cheaters (a dropped T, T flipped to T*, a relative-phase Toffoli
  passed off as a Toffoli, a wrong target, a dropped control, Z passed off as nothing, ...): every one is caught.
  Exhaustive up to 22 qubits (in practice about 14 in reasonable time). `phaseverify sample K A B` checks K columns
  of wider circuits (up to 26 qubits): a NOT_EQUAL there is a proof, an AGREES_ON_SAMPLE is only evidence.
- `../verifier/pathverify.c` (new, T+51): exact equivalence at **any width**, by sum-over-paths. B^-1 A is written as
  a path sum and reduced with the rules [Elim], [HH] and [omega] (Amy, "Towards Large-scale Functional Verification
  of Universal Quantum Circuits", QPL 2018, arXiv:1805.06908; the rules are his, this C is ours). [HH] substitutes
  boolean polynomials, not only parities: without that, 832 path variables of cycle_17_3's miter stuck. EQUAL only
  when nothing is left but a global phase; when variables remain, an exact diagonal entry of modulus other than 1,
  or two that differ, proves NOT_EQUAL; otherwise UNKNOWN, never a verdict. Output wires may hold nonlinear
  polynomials while it reduces, and the widest operands of a huge multi-controlled phase are narrowed first (H H).
  Selftest: 50 controls, planted cheaters among them, plus multi-control Toffolis, 11 for
  [omega] with a nonlinear Q (8 against brute force in Z[w], 2 where no rule may fire, 1 refusal at the degree bound)
  3 for nonlinear output wires and 3 for the narrowing. Mutation campaigns against phaseverify on circuits it can check
  exhaustively: 1,260 mutants of folded and optimized circuits, full and zero-ancilla modes, 0 judged EQUAL where
  phaseverify said NOT_EQUAL. `pathverify guess` is Allegedly's guesser (below); it proves nothing.
- **Richard** (named by the owner: it beat Feynman), the phase optimizer, and the rest of the phase pipeline (the
  baseline folder, the RevLib-to-Clifford+T compiler, the QFT generator, the run scripts) are part of
  **Witchcraft(TM)**, our private toolchain. They are not published. What is here is what you need to check their work:
  the two provers, the published numbers, the proven outputs and the court.
- `PUBLISHED.tsv`: published T-counts with citations (Kissinger & van de Wetering, PRA 102, 022406, 2020, Table 1;
  AlphaTensor-Quantum, arXiv:2402.14396, Table 2; Vandaele, Martiel, Perdrix & Vuillot, Quantum 9, 1860 (2025),
  Table 2, ancilla-free). Read from the papers' text; re-check against the PDFs before a claim.
- `controls/`: two input circuits and Witchcraft's proven outputs for them (10 T -> 8 T).
- `Makefile.phase`: the public court, `make -f phase/Makefile.phase proof`: builds both provers, runs their selftests,
  re-proves the control outputs with both, and requires a planted cheater (one T inverted) to be refused by both.

## The techniques

Named by the owner. What they do stays in Witchcraft(TM); what they did is in the tables below, every row proven.

| technique | |
|---|---|
| Battle Royale | the tournament: nothing survives a round unproven, and a planted cheater must be caught every round |
| Parallax | a second point of view |
| Speed Dating | picks a partner the expensive way |
| Don't Ask, Don't Tell | ignores what can never be seen |
| Allegedly (Confirmed) | guesses, then proves; a guess that fails the proof never happened |
| The Old College Try Hard | tries harder |

## Records (records/)

Four circuits below the lowest published no-gadget T-count on file, each proven EQUAL to the benchmark by pathverify
(global phase w^0), every wire an arbitrary input, no ancillas, measurements or gadgets (`records/RECORDS.tsv`):

| benchmark | published no-gadget (ATQ) | record | CNOT | file |
|---|---|---|---|---|
| gf2^7_mult | 104 | **103** | 1,276 | records/gf2_7_mult.103t.qc |
| gf2^8_mult | 123 | **113** | 5,438 | records/gf2_8_mult.113t.qc |
| gf2^9_mult | 161 | **139** | 10,083 | records/gf2_9_mult.139t.qc |
| gf2^10_mult | 196 | **177** | 3,903 | records/gf2_10_mult.177t.qc |

Made by Witchcraft(TM) out of AlphaTensor-Quantum's own published small-field decompositions (Ruiz et al.,
arXiv:2402.14396, Table 2): the credit for the small pieces is theirs. Their gadget constructions are lower still
(58, 70, 92 T), and so are the T-counts of circuits that add ancillas; these records are for the no-gadget column only.
The CNOT counts have not been optimised. The benchmarks are meamy/feynman@d2c382a's `gf2^n_mult.qc`, copied as
`gf2_n_mult.qc`. `make -f phase/Makefile.phase proof` re-proves all three and refuses a planted cheater for each.

## Results (results/)

- **Scoreboard** (`SCOREBOARD_20261022.tsv`, against `PUBLISHED.tsv`): Richard's proven T-count on all 30 published
  circuits plus fprenorm, qft_4 and grover_5. **Richard's records: 0** (the lane's three are below, under Records). Every row is proven EQUAL by pathverify; rows up to 12
  qubits also exhaustively by phaseverify, rows up to 20 on 4 sampled columns. Against the lowest published
  ancilla-free number per circuit:
  - Below it, but **tied with a published construction, so not a record**: barenco_tof_5 32 and barenco_tof_10 72
    (tables: 33 and 83, AlphaTensor-Quantum without gadgets). Maslov 2016 (PRA 93, 022311, Proposition 5) builds the
    same gates with 8n - 16 = 32 and 72 T and fewer borrowed lines. Witchcraft got there without being told.
  - Tied: ham15-low 94 (Vandaele et al. 2025, ancilla-free; AlphaTensor-Quantum 73 is lower still).
  - Below the 2020 tables, behind later work: gf2^4_mult 49 (KvdW PyZX+TODD 52, AlphaTensor-Quantum 39), csla_mux_3 42
    (KvdW 45; AlphaTensor-Quantum 39), ham15-high 980 (1,010 published before 2024; AlphaTensor-Quantum 773),
    mod_mult_55 20 (ties KvdW PyZX+TODD; AlphaTensor-Quantum 17), qcla_adder_10 141 (KvdW 162; AlphaTensor-Quantum 116).
  - Behind everywhere else: tof_n, mod5_4 16 (7), hwb6 74 (51), hwb8 3,559 (Vandaele et al. 3,487), adder_8 207
    (129), the gf2 multipliers past 4 bits (gf2^8 224 vs 123), mod_red_21 73 (51), and cycle_17_3 at 1,818 against
    KvdW's 1,797, the closest row with no AlphaTensor-Quantum entry.
- **Techniques one at a time** (`TECHNIQUES_20261022.tsv`): WIN or NO_GAIN per technique per circuit (33 circuits).
  Don't Ask Don't Tell with Speed Dating: WIN on 16. Allegedly (Confirmed): WIN on 6 (the barenco family 224 -> 72,
  ham15-high, cycle_17_3, adder_8). The Old College Try Hard: WIN on 2 alone, more in combination (gf2^4 59 -> 50,
  csla_mux_3 56 -> 42, mod_mult_55 30 -> 20). Parallax: NO_GAIN on all 33. Battle Royale's later rounds: WIN on 1
  (hwb8, by 1).
- **The approximate QFT** (`APPROXIMATE_QFT_20261022.txt`, approximate, kept apart from every exact row): its T-count is
  proven, Richard finds nothing to cut, and its distance to the QFT is 0.77 at N = 3 and about 2 (the maximum) from
  N = 5 on. A useful approximate QFT needs synthesised small rotations: NOT RUN.
- **Second scoreboard: the classical records in Clifford+T** (`CLASSICAL_CT_20261022.tsv`): the claimed RevLib records
  compiled to Clifford+T by Witchcraft(TM), every compilation proven against the record's own gates, then cut and
  proven again. 213 claimed rows: 175 compilations proven, 126 cut and proven (together 306,978 T as 7-T Toffolis,
  136,568 after the cut, 56% less); the rest not finished (NOT RUN, or UNKNOWN where the provers could not decide in
  time). No published T-counts for these files are compared.
- **[omega] with a nonlinear Q** (`PATHVERIFY_NONLINEAR_OMEGA_20261022.txt`; spotted by the reasoning agent): the rule
  holds for every boolean Q and pathverify now uses it. Before and after on the same 44 pairs: one classical compilation
  newly proven (apex4_202 with relative-phase Toffolis, UNKNOWN in 142 s -> EQUAL in 55 s; four planted cheaters against
  it refused), 10 UNKNOWN still UNKNOWN, every other verdict unchanged.
- **Nonlinear output wires** (`PATHVERIFY_NONLINEAR_OUTPUTS_20261022.txt`): every compilation the prover had left
  UNKNOWN is now proven EQUAL (misex1, dc2, inc, sao2, C7552, decod, varops_16, tial), most in under a second; the six
  narrow ones checked again by plain simulation of every input, 27 planted cheaters against them refused or undecided,
  never EQUAL. Proven compilations in the second scoreboard: 144 -> 149 (then 175 with the routes below).
- **Classical proof routes** (`CLASSICAL_CT_PROOF_ROUTES_20261022.txt`): `revverify qc2real` turns a Toffoli-only
  compilation into a .real, so revverify's exact check or SAT miter (kissat + drat-trim, as in records/PROOFS.tsv)
  proves it against the record at any width; phase circuits are refused. 25 more compilations proven this way (and 1
  by the narrowed pathverify); the cuts for newly proven rows were rerun: cycle17_3_112, squar5, misex1, dc2 and inc are
  cut and proven.
- **The prover on real circuits** (`PROVER_ON_BENCHMARKS_20261022.txt`) and **the published phase-folding bar**
  (`PHASEFOLD_BASELINE_20261022.txt`).

## Said plainly

- No records. AlphaTensor-Quantum (no gadgets) is lower on every circuit it reported except barenco_tof_5 and
  barenco_tof_10, and those two tie Maslov's 2016 construction. ham15-med (222) is behind KvdW's PyZX 212 too.
- The reasoning agent (Codex-quantum-reasoning, owner-directed, reads and reasons, writes no code) found three real
  problems; all are fixed and measured.
- The sqrt(2)-alignment code in phaseverify is now reached by a control (column-dependent powers), and the control
  asserts it was reached. No control can make skipping it change a verdict, and none is claimed (the argument is in
  the source, at selftest).
- `grover_5` is no longer REFUSED: with 128-bit coefficients it is EQUAL to its expansion.
- `cycle_17_3` has 30 gates that name a qubit twice (`Z 8 h 8`). Read exactly, each is a CZ (no T); the published
  original T-count (4,739) counts each as a full 7-T gate, ours is 4,529. A record there must beat both readings.
- pathverify can answer UNKNOWN; Richard then drops the candidate (never kept unproven). hwb11 runs out of memory in
  pathverify: NOT RUN.
