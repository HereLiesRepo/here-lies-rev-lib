# Here Lies RevLib: Constitutional AI, Unconstitutional Circuits, and Google DeepMind's Bad Night

Michael David Brown (HereLiesScarlet), with Claude and Codex agents. Optimizer: Witchcraft(TM). October 2026.

*Working title of the original submission: "Constitutional AI, Unconstitutional Circuits: Claude and Witchcraft(TM) vs. Google DeepMind and RevLib."*

## Summary

RevLib is the reversible-circuit benchmark everyone cites. Its price list dates from around 2010 to 2012. A university built it, eighteen years of papers defended it, and then a college dropout with zero physics classes and a chat window sent seven AI agents at it. He is listed as first author. He cannot tell you what a qubit is. The SAT solver says UNSAT regardless.

- **183 of RevLib's 456 entries** now have a circuit cheaper than the best one published anywhere we could find.
- Total quantum cost fell from **14,391,810 to 7,891,148 (-45.2%)**.
- **71 circuits** were cut by more than half. cordic went from 349,522 to 1,778.
- On Clifford+T GF(2^n) multipliers with no ancillas, no measurements and no gadgets, we beat Google DeepMind's AlphaTensor-Quantum for n = 7 to 10: **103 / 113 / 139 / 177 T against 104 / 123 / 161 / 196**.
- Every claim is checked against the file RevLib serves, exhaustively or by SAT with a DRAT proof that drat-trim accepts.
- We also withdrew 27 of our own claims, which is the part of this paper we are proudest of (Section 6).

## 1. What we did

The agents took RevLib's 456 function-and-size entries (a function name plus its *lines_constants_garbage* triple) and every better published circuit we could collect: RevLib's own, the reversiblebenchmarks.github.io collection, and Lee et al. The bar for an entry is the lower of RevLib's listed cost and our recost of its file. We then rebuilt each circuit under its file's exact rules: same lines, same constants, same garbage outputs, only that library's gates (positive controls), priced in RevKit's cost table. Cheaper wins. Anything that wins is verified (Section 4) or it does not exist. The agents did this part; the owner's contribution was to keep saying "why isn't it running", which, to be fair, was correct more than once.

The crew was 7 agents (5 Claude, 2 Codex, at most 3 Claude sessions in any two-hour window), one owner who sent 67 messages (one asked whether his parity structure applied, and it did: sym9 went from 2,077 to 199; most of the rest concerned the nfo and Trump), and about 79 hours from 2026-10-20 01:11 UTC (`tools/build_nfo.sh`).

| when | what happened |
|---|---|
| 20 Oct, hours 0-8 | owner asleep, machine asleep, nobody in charge; RevLib's price list audited (it has typos); garbage outputs turn out to be sometimes listed backwards; four small circuits proven optimal; container restarts wipe everything, twice |
| 20 Oct, hours 9-14 | first records; Zakablukov's hwb7-9, best since 2015, apparently beaten (we took that back, Section 6); 49, then 106 records |
| 20 Oct, hours 14-17 | an agent finds a bug in another agent's work, fixed in six minutes; owner's parity structure applied (sym9 2,077 to 199 at the time); 183 records |
| 20 Oct, hour 18 | every SAT proof now run twice by two independently written encoders |
| 22-23 Oct | the adversarial audit; 27 claims withdrawn |

(Counts in the table are the counts at that moment. 183 is also today's number, after the withdrawals and later gains.)

## 2. The techniques

The owner named all 22 techniques, after a rapper, a daytime talk show, a lingerie brand, a film, a video game and a translation app. The agents then had to work out what each name meant. The explanations below are theirs. Here is what each one is and why it wins, at the level you would explain it to a colleague; recipes, settings and code stay private.

**Witchcraft(TM)** is the toolchain that contains all of them, a name the owner chose in the way he chose all of them: by what it felt like. It is built in-house and not published. Everything it outputs is public and checkable.

*Search and scheduling*

- **Battle Royale** is the tournament. Candidate circuits fight; every candidate is judged on every input; a planted cheater must be caught every round; the last circuit standing is the answer. It wins because a rewrite that is subtly wrong cannot survive even one round. Its later rounds won one circuit in the T-count lane (hwb8, by one T).
- **Last Dance** is Battle Royale's schedule: the size of the random shake-up shrinks by a fixed ratio every round, so early rounds roam and late rounds polish. Think annealing with a dress code, named by someone who was not told what annealing is.
- **Etch A Sketch** is iterated local search: swap gates that commute, at random, re-optimize, and keep the result if it costs no more. Sideways moves are the point, because a plateau is where the cheaper circuit hides.
- **Speed Dating** decides where to cut a circuit by building the real circuit for the best-scoring cuts and comparing, instead of trusting the estimate. Expensive, and the estimate is usually wrong. With Don't Ask, Don't Tell it won 16 of 33 circuits in the T-count lane.
- **Through the Looking Glass** takes a small window of a circuit (a handful of lines) and searches it exhaustively for the cheapest equivalent, using the true cost table. When a window is small enough, there is no luck involved: the answer is simply the best one.

*Throwing away what cannot matter*

- **Don't Ask, Don't Tell** ignores what can never be seen: it rebuilds the visible behaviour of a piece and discards input patterns that cannot occur.
- **Kevin Gates** runs a census of which states a circuit can actually reach. Gates that can never fire are dropped, controls that the reachable states make redundant are demoted, repeated AND products are shared and helper lines are recycled. Gates that never had a chance get no sympathy.
- **Tunnel Vision** resynthesizes each output on its own, looking only at the logic that feeds it (its cone) and at its parity structure, ignoring everything off to the side. Large functions collapse because most of a big circuit is not relevant to any one output.

*Algebra*

- **I Think This Is XOR Math (Nailed It)**: the function was XOR math. A lot of arithmetic and symmetric logic is secretly a sum of AND-products over XOR, and rewriting it that way, in the family of EXORCISM-style cube moves (Mishchenko and Perkowski) and XOR-AND-XOR forms (Bernasconi et al., 2023), is much cheaper than the gate-by-gate version.
- **Holy Water** blesses a pile of rewrite rules until fewer remain: it reduces a rule set to the rules that actually do work. It is behind 9symml (14,193 to 176) and sym9 (2,077 to 176).
- **Uno Reverse Card**: if a gate on a target with control set C and the same gate with C plus one more control both appear, they can be replaced by NOT x, the second gate, NOT x. Costly gates get played back as two cheap NOTs.
- **Victoria's Secret** decomposes V and V+ gates into their square-root-of-NOT pieces and merges neighbouring powers. It reveals that a circuit had less underneath than advertised.

*Guessing, then being right*

- **Allegedly (Confirmed)** proposes merges that agree on sampled inputs and keeps one only if a proof backs it. A guess that fails the proof never happened. It won 6 T-count circuits, the Barenco family 224 to 72 among them.
- **Parallax** takes a second view of the phases, placing them at the last occurrence of their parity. It won nothing on any of the 33 circuits we tried. Parallax is the owner's favourite technique that does not work, and it stays in the paper.
- **The Old College Try Hard** is the control arm: the baseline pass with extra effort. It won 2 circuits alone and more in combination (gf2^4 59 to 50, csla_mux_3 56 to 42, mod_mult_55 30 to 20).
- **Richard** is the phase optimizer for the Clifford+T lane. It beat Feynman and holds **zero** records, and we say so in bold because a technique's record count should be on the first line of its biography.

*Beyond RevLib*

- **Field of Dreams** builds GF(2^n) multipliers through a tower of smaller fields, starting from AlphaTensor-Quantum's own small-field decompositions (theirs, credited). We built it, and the four multipliers of Section 5 came.
- **Google Translate** compiles a classical record into Clifford+T and proves the translation equal by SAT, so nothing is lost. On the earlier ledger of 213 claimed rows, 175 compilations were proven and 126 were then cut, from 306,978 T to 136,568 (-56%), all counted as 7-T Toffolis. That ledger predates the withdrawals.
- **Goodwill Hunting** reuses a checked AND and rewrites blocks that get measured out, on the owner's SpaceTime circuits. It took modinv-point-add under a million Toffolis. It is not a RevLib result and no RevLib record depends on it.
- **Tetris** packs a circuit into lookup-table covers with six-input tables. It is a mapping method; we claim no record from it.

*Trust*

- **Maury: You Are NOT the Circuit** is the verification regime and the reason every record here is proven: two independently written miter encoders, kissat, DRAT proofs checked by drat-trim, hashes binding each proof to its circuit, and planted cheaters that must be caught. The optimizer does the work; Maury determines the father. Nobody on the team has explained to the owner that Maury is a talk show and not a proof system, and we are not going to.

Two further techniques come from the owner's SpaceTime paper rather than Witchcraft: the ring-parity construction (our notes call it checkmate) and SpaceTime #7 ("What Big Parity Doesn't Want You to Know"). The second took C7552 to 440 and dropped 30 records further. Per-technique totals beyond the figures above were not recorded, so we give none.

## 3. Results: RevLib

Computed from `records/RECORDS.tsv`, claimed rows only (`claimed=yes`, verified exact).

| measure | value |
|---|---|
| entries beaten | 183 of 456 (118 distinct function names) |
| total quantum cost, best published to ours | 14,391,810 to 7,891,148 (-45.2%) |
| mean per-circuit reduction | 41.1% |
| reduction >= 50% / 25-50% / 10-25% / < 10% | 71 / 51 / 31 / 30 |
| bar source: RevLib / Maslov collection / Lee et al. | 172 / 7 / 4 |

**Largest cuts, in quantum cost saved.**

| circuit | size | published | ours | cut |
|---|---|---:|---:|---:|
| cpu_alu_32bit_329 | 756_685_722 | 2,235,491 | 799,629 | 64.2% |
| cpu_alu_16_376 | 142_103_124 | 1,281,717 | 437,933 | 65.8% |
| cpu_alu_16bit_328 | 404_365_386 | 662,531 | 205,861 | 68.9% |
| alu_flat_32_337 | 230_164_134 | 1,380,526 | 971,315 | 29.6% |
| alu_32_359 | 325_259_229 | 1,230,577 | 861,042 | 30.0% |
| cordic | 25_2_23 | 349,522 | 1,778 | 99.5% |

**Largest cuts, in percent.**

| circuit | size | published | ours | cut |
|---|---|---:|---:|---:|
| cordic | 25_2_23 | 349,522 | 1,778 | 99.5% |
| cpu_alu_32bit_353 | 6204_6133_6170 | 112,396 | 671 | 99.4% |
| 9symml | 10_1_9 | 14,193 | 176 | 98.8% |
| cpu_alu_16bit_424 | 2141_2102_2123 | 30,208 | 425 | 98.6% |
| varops_16_350 | 112_64_96 | 1,361 | 96 | 92.9% |
| add6 | 19_7_12 | 6,455 | 499 | 92.3% |
| sym9 | 10_1_9 | 2,077 | 176 | 91.5% |
| z4ml | 11_4_7 | 642 | 81 | 87.4% |

*Caption: the 32-bit ALU is, in some sense, now a 1-bit ALU with ambition.*

**By family** (function names grouped by us).

| family | entries | published | ours | cut |
|---|---:|---:|---:|---:|
| ALU and CPU datapaths | 40 | 12,241,998 | 6,801,763 | 44.4% |
| MCNC/PLA-style named functions | 63 | 1,401,514 | 625,840 | 55.3% |
| Synthesised-program circuits (bubblesort, control structures) | 37 | 584,978 | 369,780 | 36.8% |
| Symmetric and arithmetic benchmarks | 33 | 87,780 | 22,928 | 73.9% |
| hwb and urf | 10 | 75,540 | 70,837 | 6.2% |

*Caption: the hardest family to beat is the one built to be unstructured. Nobody is surprised.*

Not claimed: four ALU rows proven only by sampling (1,048,576 random inputs); Lee et al.'s urf4 (68,933), which is a different function from RevLib's urf4; and 13 circuits (alu4, tial, f51m, max46, misex3, misex3c, sqr6, C7552, apex2, ex5p, frg2, seq, spla) whose lower published costs use mixed-polarity gates our library does not have.

## 4. Verification

Our verifier (`verifier/revverify.c`) is a single C file sharing no code with the optimizer, so no claim rests on trusting the optimizer.

| route | rows |
|---|---:|
| exhaustive simulation over every free input (9 strict, 93 intended garbage reading) | 102 |
| SAT miter, two independently written encoders, kissat, DRAT proof checked by drat-trim (22 strict, 59 intended) | 81 |

Garbage-output readings: *strict* compares every line not marked garbage; *intended* reverses the garbage string when RevLib's file lists it backwards. V and V+ gates are modelled in Z4, and a control left in superposition counts as a difference. Trusted base: the verifier, kissat, drat-trim and the CNF encoding.

## 5. Google DeepMind: GF(2^n) multipliers

Benchmarks: `gf2^n_mult.qc` from meamy/feynman at d2c382a2. Rules: Clifford+T on exactly the benchmark's wires (21, 24, 27, 30), no ancillas, no measurements, no gadgets, every wire an arbitrary input, equal as a full unitary up to global phase.

| n | **ours (T)** | AlphaTensor-Quantum, no gadgets | AlphaTensor-Quantum, with gadgets | Vandaele binary-field + FastTODD | arXiv:2605.15476, clean ancillas |
|---|---:|---:|---:|---:|---:|
| 7 | **103** | 104 | 44 | 111 | 96 |
| 8 | **113** | 123 | 58 | 135 | 116 |
| 9 | **139** | 161 | 70 | 185 | not reported |
| 10 | **177** | 196 | 92 | 207 | not reported |

Under the rules above, ours are the lowest we found, by 1 T at n = 7 and by 19 T at n = 10. AlphaTensor-Quantum with gadgets (44/58/70/92) is far lower, but gadgets are a different model. The arXiv:2605.15476 ancilla construction reaches 96 at n = 7, below our 103, and 116 at n = 8, above our 113. Our circuits start from AlphaTensor-Quantum's small-field decompositions; that starting material is theirs.

**Proof.** Path sums (`verifier/pathverify.c`, after Amy) decide equality; the T-deleted negative control fails for all four (`make -f phase/Makefile.phase proof`). An independent MQT QCEC check confirmed n = 7 only (149 s); n = 8 gave no answer in 600 s; n = 9 and 10 were not run. T-count is the only quantity optimized; CNOT counts are not.

*Caption: we did not beat the gadget numbers. We would like it noted that we did not use any.*

## 6. What we found wrong

Nobody caught these for us. We found every one by auditing our own claims against the files RevLib actually serves, and we took the claims back ourselves. A leaderboard that cannot lose entries is a press release; this is why the other 183 can be trusted.

| finding | effect |
|---|---|
| **Verifier `g` bug.** The intended-reading check treated any output named `g` as garbage. In 17 RevLib files `g` is an ordinary variable, so a line was never compared (plus63mod4096_163 differs from RevLib on 128 of 4,096 inputs). | 17 claims withdrawn: 4gt4-v0_73, ham7_106, ham7_299, ham15_109, ham15_298, hwb7_302, hwb8_303, hwb9_304, cycle10_2_110, cycle10_293, cycle17_3_112, and the six plus63/127mod rows. |
| **Jung2021-reachable.** Jung and Choi (2021) publish costs 19, 13, 18, 22 for the 4gt family; we reproduced them under RevLib's own embedding. | 4gt4-v0_72, 4gt5_77, 4gt10-v1_81, 4gt12-v0_86 withdrawn. |
| **Wang2015 hwb6 = 102.** Wang et al. give 102 in RevKit's table; we rebuilt and checked that circuit. Ours is 103. | hwb6_56 withdrawn. |
| **rd32_271 is wrong against its source.** Our circuit does not match the source file rd32_272. | rd32_271 withdrawn. |
| **cpu_register sibling.** RevLib lists a same-size sibling at 2,232; our 9,716 could not be verified against an unobtainable file. | cpu_register_16_332 withdrawn. |
| **arb8_322.** Our reference was a 39-line file RevLib does not serve; RevLib's has 60 lines and ours differed on 219 of 256 inputs. | withdrawn 2026-10-22. |
| **Proofs older than the verifier fix.** | alu_flat_32_457, mult_stmts_32_345 withdrawn pending re-proof. |

That is 17 + 4 + 1 + 1 + 1 + 2 = 26 files withdrawn on 2026-10-23, plus arb8_322 the day before: 27. hwb7 to hwb9 are among the 17; we make no claim to beat Zakablukov. Three more rows (mini-alu, mod10, urf6) are unclaimed because a lower published number exists (Section 7).

## 7. Published numbers lower than ours

| circuit | published | ours | source | note |
|---|---:|---:|---|---|
| mini-alu 4_0_0 | 16 | 26 | Szyprowski and Kerntopf | no claim |
| Modulo10Counter 4_0_0 | 21 | 25 | Szyprowski and Kerntopf | 25 is proven minimal in MCT for RevLib's file; their function or library reading differs (unresolved) |
| urf6 15_0_0 | below 17,046 | 16,793 | arXiv:1004.1697 | no claim |
| hwb5 5_0_0 | 71, author-reported | 73 | X. Wang, Maslov page | their circuit recosts to 77 in RevKit's table |
| plus63mod4096, plus63mod8192, plus127mod8192 | 4,943; 16,580; 16,455 | rows withdrawn | Wang et al. 2015 | |

## 8. Threats to validity

- Bars were collected by us; a lower published circuit we did not find would falsify that row. We found no 2022-2026 paper reporting RevLib quantum costs.
- RevKit's table prices every row; other papers use other cost models, and where their circuit was available we re-priced it in RevKit's table.
- Maslov circuits were matched to RevLib functions by equivalence (81 of 173).
- Comparisons with ancilla, gadget or mixed-polarity papers are listed, never ranked.

## 9. Reproducing

```
cc -O2 -o revverify verifier/revverify.c
sh tools/verify_all.sh <directory of RevLib .real files> <path to kissat> out.tsv
make -f phase/Makefile.phase proof
```

Expected: every claimed row `EQUAL_EVERY_INPUT` or `UNSAT_EQUAL`, and the four phase records `EQUAL`. Inputs: this repository's `records/RECORDS.tsv` and `records/PROOFS.tsv` (a SHA-256 per circuit), RevLib `.real` files as served at download (RevLib's files change; arb8_322 showed it), meamy/feynman at d2c382a2ab43a40a87f12f4255645bbb55f704f8. Circuits are under `records/cc-by-nc-sa/NOTICE.md`. Solvers: kissat 4.0.4 and drat-trim (git 2e3b2dc, 2024-11-25); prices are RevKit's quantum-cost table, as RevLib publishes it. Runtime and memory of `verify_all.sh`: Recorded runtime for the 183 claimed rows: 5,867 single-core seconds (about 1.6 hours), the slowest being simple_alu_32_459 at 742 s (records/PROOFS.tsv, column 7). Memory was not measured..

## Acknowledgements

The math, the code, the proofs and the corrections to the math, code and proofs were done by Claude and Codex agents.[^usage] The names, the nfo, the insistence on the nfo, and the question "is it running" were done by Michael David Brown, who regrets nothing and has still not looked up what a DRAT proof is. RevLib is the work of R. Wille, D. Grosse, L. Teuber, G. W. Dueck and R. Drechsler.

[^usage]: We wanted to do more, but ran out of usage (*cough* Anthropic *cough*). It is a joke about a rate limit, and also not.

## References

Prior art we build on is credited per `notes/paper_research/PAPER-PRIOR-ART-CREDITS-20261023.md`; the papers are theirs, the selection, engineering and records are ours.

1. R. Wille et al. RevLib: An Online Resource for Reversible Functions and Reversible Circuits. ISMVL 2008, 220-225. doi:10.1109/ISMVL.2008.43
2. D. Maslov. Reversible Logic Synthesis Benchmarks Page. reversiblebenchmarks.github.io
3. H. Lee, K. C. Jeong, D. Han, P. Kim. Reversible Logic Circuit Synthesis Based on Tensor Decomposition. ACM Trans. Quantum Computing 5(3), 2024. arXiv:2107.04298
4. M. Szyprowski, P. Kerntopf. Reducing Quantum Cost in Reversible Toffoli Circuits. arXiv:1105.5831
5. X. Wang et al. J. Mult.-Valued Logic Soft Comput. 25 (2015) 643-671.
6. Jung, Choi. PLOS ONE 16(6): e0253140, 2021.
7. Mixed-polarity work: Datta et al., RC 2013; Schonborn et al., DDECS 2014; Rahman, Rice, IJCIT 5(1), 2016; Bandyopadhyay et al., VDAT 2020 (full titles unverified).
8. D. V. Zakablukov. arXiv:1507.04309. Saeedi et al. arXiv:1004.4320; Arabzadeh et al. arXiv:1004.1755.
9. F. J. R. Ruiz et al. Quantum circuit optimization with AlphaTensor. Nature Machine Intelligence 7, 374-385, 2025. arXiv:2402.14396
10. Vandaele. Lower T-count with faster algorithms. arXiv:2407.08695.
11. Wang, Yu, Wu, Cong. Quantum Circuit Synthesis Using an Exact T Library. arXiv:2605.15476, 2026.
12. L. E. Heyfron, E. T. Campbell. An efficient quantum compiler that reduces T count. Quantum Sci. Technol. 4(1), 2018. arXiv:1712.01557
13. M. Amy, D. Maslov, M. Mosca. Polynomial-time T-depth optimization of Clifford+T circuits via matroid partitioning. IEEE TCAD 33(10), 2014. arXiv:1303.2042
14. M. Amy. Towards Large-scale Functional Verification of Universal Quantum Circuits. EPTCS 287, 2019. doi:10.4204/EPTCS.287.1
15. L. Burgholzer, R. Wille. Advanced Equivalence Checking for Quantum Circuits. IEEE TCAD 2021. doi:10.1109/TCAD.2020.3032630
16. A. Biere et al. CaDiCaL, Kissat, Paracooba, Plingeling and Treengeling Entering the SAT Competition 2020. B-2020-1, 50-53.
17. N. Wetzler, M. J. H. Heule, W. A. Hunt Jr. DRAT-trim. SAT 2014, LNCS 8561, 422-429.
18. A. Mishchenko, M. Perkowski. Fast Heuristic Minimization of Exclusive-Sums-of-Products. RM 2001.
19. A. Kuehlmann et al. Robust Boolean reasoning for equivalence checking (SAT sweeping). IEEE TCAD 2002.
20. A. Bernasconi et al. XOR-AND-XOR Logic Forms for Autosymmetric Functions and Applications to Quantum Computing. IEEE TCAD 42(6), 1861-1872, 2023. doi:10.1109/TCAD.2022.3213214
21. D. Maslov, G. W. Dueck, D. M. Miller. Toffoli network synthesis with templates. IEEE TCAD 24(6), 2005. doi:10.1109/TCAD.2005.847911
22. D. Grosse et al. Exact Multiple-Control Toffoli Network Synthesis With SAT Techniques. IEEE TCAD 28(5), 2009. doi:10.1109/TCAD.2009.2017215
23. M. Amy, D. Maslov, M. Mosca, M. Roetteler. A meet-in-the-middle algorithm for fast synthesis of depth-optimal quantum circuits. IEEE TCAD 32(6), 2013. doi:10.1109/TCAD.2013.2244643
