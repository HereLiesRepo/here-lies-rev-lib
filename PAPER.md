I Was Asleep for Most of This: Beating RevLib and AlphaTensor-Quantum

# I Was Asleep for Most of This: Beating RevLib on 183 Circuits (45% Lower Quantum Cost) and AlphaTensor-Quantum on All Four GF(2^n) Multipliers

Oct 23, 2026

HereLiesScarlet (Michael David Brown), Lafayette, LA · here.lies.scarlet@gmail.com.
Affiliation: none, again. Last time nobody invited us. This time we brought employees, and they did not need to be invited either.

> “equivalent=yes” — revverify, 102 times. “s VERIFIED” — drat-trim, 81 times.

*Sequel to “I Did Not Read the README” (doi:10.5281/zenodo.23486661). Technique numbers continue from that paper's #1 to #28.*

## Abstract

We report circuits that lower the quantum cost of 183 of the 456 function-and-size entries of the RevLib reversible-logic benchmark below the best published circuit we could find, and Clifford+T circuits that lower the T-count of all four GF(2^n) multipliers n = 7 to 10 below AlphaTensor-Quantum's no-gadget results: 103, 113, 139 and 177 against 104, 123, 161 and 196. The 183 RevLib circuits cost 7,891,148 in total against 14,391,810 for the best published ones, 45.2% lower; 71 are cut by at least half (70 by more than half), and cordic falls from 349,522 to 1,778. Every circuit is checked against the file RevLib serves, by exhaustive simulation over every free input (102 circuits) or by a SAT miter solved by kissat and certified by a DRAT proof that drat-trim checks (81; proven with two independently written encoders, of which the public replay includes one). Against the earlier published methods RevLib lists, our circuits are up to 196.6× cheaper. The work was done by autonomous Claude and Codex agents (7 agents, at most 3 at once, about 80 hours) that found and withdrew 27 of their own claims. We list those, the 3 rows where a lower published number exists, and the numbers on which others stay lower, including AlphaTensor-Quantum's gadget results of 44, 58, 70 and 92. The circuits, the verifier and the proof ledger are released; every technique is described. Our compiler and search tools are not released.

## 1. Introduction

RevLib (Wille et al., ISMVL 2008) is the standard library of reversible circuits. Each circuit fixes its number of lines, its constant inputs, its garbage outputs and its gate library, and is priced by RevKit's quantum-cost table. The listed costs date from about 2010 to 2012. AlphaTensor-Quantum (Ruiz et al., Nature Machine Intelligence 2025) is Google DeepMind's reinforcement-learning search for low T-count Clifford+T circuits; its published GF(2^n) multiplier results are the best we know under no ancillas, no measurements and no gadgets.

We ask two questions. How far below the published circuits can these two benchmarks go? And can we get there without the work being done by a person who understands it?

On the first, the RevLib total drops 45.2% and cordic by 99.5%. On the second, the answer is a long timeline (Section 3) in which the human mostly sleeps, names things and asks why it isn't running. In the first paper, the author did not read the README. He still has not. This time he did not read the proofs either; the agents did, and then they read each other's.

The author has no physics degree, no lab and no quantum computer, and worked from a chat window and a C compiler, as before. The author still cannot tell you what a qubit is. The SAT solver says UNSAT regardless.

## 2. Setup

**RevLib files.** The `.real` files as served on the download date. RevLib may serve different files over time; one of our references, arb8_322, turned out not to be what RevLib serves (Section 7). An entry is a function name plus its *lines_constants_garbage* triple; we count entries, and the 183 claimed entries cover 118 distinct function names.

**What a win is.** The same number of lines, the same constants and the same garbage outputs as the file; only gates from that file's library, with positive controls; strictly lower quantum cost in RevKit's quantum-cost table, as RevLib publishes it, than every published circuit we collected at that function and size. The bar is the lower of RevLib's listed cost and our recost of its file. Of the 183 bars, 172 come from RevLib, 7 from the Maslov collection (reversiblebenchmarks.github.io) and 4 from Lee et al.

**Garbage readings.** Some RevLib files list `.garbage` in reverse. A *strict* check compares every line not marked garbage. An *intended* check reverses a garbage string that marks more constant lines than its reverse. Of the 183, 9 are exhaustive and strict, 22 are SAT and strict, 93 are exhaustive and intended, and 59 are SAT and intended.

**Verifier.** `revverify` is one C file that shares no code with the optimizer. It simulates every assignment of the free inputs when there are few enough, or writes a miter CNF that is unsatisfiable exactly when the two circuits agree. V and V+ gates are modelled as Z4 values, and a control left in superposition counts as a difference. The SAT route was run at proof time with two independently written encoders (revverify's miter, public, and the toolchain's own, private); the public replay re-checks with revverify's miter only, plus a DRAT proof checked by drat-trim. kissat 4.0.4, and drat-trim (git 2e3b2dc0ecf938addbd779d42877b6ed69d9a985, 2024-11-25). For Clifford+T we use a path-sum verifier (after Amy), and QCEC as an independent check where it finishes.

**T-count benchmarks.** `gf2^n_mult.qc` from meamy/feynman at commit d2c382a2ab43a40a87f12f4255645bbb55f704f8: Clifford+T on exactly the benchmark's wires (21, 24, 27 and 30), no ancillas, no measurements, no gadgets, every wire an arbitrary input, equal as a full unitary up to global phase.

### 2.1 The agents

The 183 are the work of 7 agents, 5 Claude and 2 Codex, counted from commit trailers and the board (`tools/build_nfo.sh`): at most 3 Claude sessions in any two-hour window, about 80 hours from 2026-10-20 01:11 UTC, and 67 messages from the owner. Some of the structure that made that work:

- **Autopilot.** After the owner asked "This is automation?" at 20:41 on 21 Oct, a loop was running by 20:42. It re-checked every record forever, and when the container reset at 23:42 it rebooted itself. The containers die about every two hours; the search was changed to remember where it was.
- **The Manager admits work only through proofs.** The phase lane was landed on main through the project's manager, with its court passing (07508f0); five of a reviewer's bug reports were landed the same way (630a7a1), each with a test that the old code fails. Eleven board jobs closed with receipts, and the receipts got receipts.
- **Agents audit agents.** Codex-scarlet-v7 found two bugs in the Claude-built checker (a stale proof cache, and a cost that wraps at 63 controls). A reasoning agent found two bugs in the phase optimizer (a cheater that timed out was counted as caught). The checker's rewrite came with 16 tests; the old checker fails 14. The corrected 63-control cost hit 0 of 1,137 records.
- **Agents audit themselves.** The first fix for backwards garbage blessed an empty circuit as a 32-bit ALU (8 files); an agent noticed, and the rule was fixed. A later audit found the `g` bug that cost 17 rows (Section 7).
- **Leases.** One session's lease expired mid-task; another saved its branch by the rules and handed it back twelve minutes later.
- **The human.** Several of the 67 messages are questions of the form "why isn't it running". The container was, mostly, dead because nobody was talking to it. He also named the techniques, supplied the parity structure that took sym9 from 2,077 to 199 at the time, and asked for the white paper that this is.

## 3. The timeline, briefly

| when (UTC) | what happened |
|---|---|
| 20 Oct, hours 0-8 | RevLib's price list audited (73 typos); garbage found to be listed backwards in places; four small circuits proven optimal; owner and machine asleep; container restarts wipe state twice |
| 20 Oct, hours 9-14 | first records; hwb7 to hwb9 beaten (the garbage-line versions were later taken back; the 0-garbage ones stand); 49, then 106 records |
| 20 Oct, hours 14-17 | one agent finds a bug in another's work, fixed in six minutes; the owner's parity structure applied; 183 records |
| 20 Oct, hour 18 | every SAT proof run twice by two independent encoders (one of them private) |
| 21-22 Oct | the autopilot; a second prover for Clifford+T; Richard, the phase optimizer, scores 0 records; the multipliers arrive (gf2^7 at 103 T at 20:31 on the 22nd, one T under AlphaTensor-Quantum, "the smallest margin we own") |
| 22-23 Oct | adversarial audit by agents; 27 claims withdrawn; the count lands at 183 |

## 4. The headline: cordic, and the multipliers

**cordic.** RevLib's best circuit costs 349,522. Ours, 25 lines with 2 constant inputs and 23 garbage outputs, costs 1,778: 99.5% lower, 196.6× cheaper, and the largest ratio among the 183. It was rebuilt from the function's 23-input truth table, not edited, and is proven equal to RevLib's file on every input.

**The multipliers.** Under the rules of Section 2, the T-counts are:

| n | ours | AlphaTensor-Quantum, no gadgets | lower by | Vandaele, binary field + FastTODD | Vandaele, FastTODD |
|---|---:|---:|---:|---:|---:|
| 7 | **103** | 104 | 1 | 111 | 155 |
| 8 | **113** | 123 | 10 | 135 | 205 |
| 9 | **139** | 161 | 22 | 185 | 257 |
| 10 | **177** | 196 | 19 | 207 | 315 |

Our circuits start from AlphaTensor-Quantum's own published small-field decompositions (Ruiz et al., Table 2); that starting material is theirs, and we cite it for the same reason a bank cites its customers. Equality is decided by path sums, and the T-deleted negative control fails for all four. An independent QCEC check confirmed n = 7 only (149 s); at n = 8 it returned nothing in 600 s, and n = 9 and 10 were not run. T-count is the only quantity optimized; CNOT counts are not (1,276, 5,438, 10,083 and 3,903). Gate counts: n = 7 has 1,522 gates in total, n = 8 has 5,679, n = 9 has 10,489 and n = 10 has 4,320.

## 5. Techniques

This paper continues the numbering of the first. **This work used 21 new techniques, #29 to #49, which we name so later work can cite them, and describe each in the table below.** It also reused seven from the first paper: #2 The Kaitlyn Proposal, #7 What Big Parity Doesn't Want You to Know, #21 Toffolis Without Jurisdiction, #22 Musical Chairs, #23 Unsupervised Optimism, #25 Squatter's Rights and #28 Turning It Off and On Again (in the reversible-circuit lane; we found no use of the first paper's techniques in the Clifford+T lane). The ledger these descriptions come from, as it stood at commit 023d368 of our private repository, hashes to SHA-256 `ed40be4a2f38c612363654feb274be678b6a67e4abd561498e7a18574ae3cdb4`. Witchcraft(TM), the toolchain that contains them, is the register's other row and is not a technique. All 22 names, like the first paper's 28, were chosen by the author, on the principle that a name should be what a technique feels like. The agents were then asked what each one meant.

**Prior art.** The principles are published; the constructions and results are ours. Close published work we credit: iterated local search (Lourenco, Martin and Stutzle, 2003) for Etch A Sketch; exact and meet-in-the-middle synthesis (Grosse et al., 2009; Amy et al., 2013) and A* (Hart, Nilsson and Raphael, 1968) for Through the Looking Glass; EXORCISM-style cube moves (Mishchenko and Perkowski, 2001) and XOR-AND-XOR forms (Bernasconi et al., 2023) for I Think This Is XOR Math and Tunnel Vision; template matching (Maslov, Dueck and Miller, 2005) and SAT sweeping (Kuehlmann et al.) for Holy Water and Allegedly (Confirmed); phase-polynomial T-count optimization (T-par, Amy, Maslov and Mosca, 2014; TODD, Heyfron and Campbell, 2018) for Richard; the V and V+ decompositions of Barenco et al. (1995) for Victoria's Secret; temporary logical-AND with measurement-based uncompute (Gidney, 2018) for Goodwill Hunting; path sums (Amy, 2019), QCEC (Burgholzer and Wille, 2021), kissat and DRAT-trim for Maury; AlphaTensor-Quantum (Ruiz et al., 2025) as the starting material of Field of Dreams. Tournament selection is classical (Goldberg and Deb, 1991). The review behind these credits was a bounded literature search, not a proof that no other source exists. We found no published match for Uno Reverse Card or Last Dance in the sources we checked.

| # | Technique | What it does |
|---|---|---|
| 29 | Field of Dreams | Builds a GF(2^n) multiplier through a tower of smaller fields, seeded with published small-field decompositions |
| 30 | Kevin Gates | Census of the states a circuit can reach: deletes gates that can never fire, demotes controls the reachable states make redundant, shares repeated AND products and recolours helper lines |
| 31 | Etch A Sketch | Iterated local search: swaps random commuting gates, re-converges, and keeps equal-cost moves |
| 32 | Through the Looking Glass | Exact A* search over windows of up to five lines, priced by the real cost table |
| 33 | Maury: You Are NOT the Circuit | The verification regime: two independent miter encoders (one public), kissat with DRAT proofs checked by drat-trim, a SHA-256 binding each proof to its circuit, planted cheaters that must be caught |
| 34 | Victoria's Secret | Decomposes V and V+ gates into their square-root-of-NOT parts and merges powers of V |
| 35 | Uno Reverse Card | Rewrites T(C) and T(C+x) on one target as NOT x, T(C+x), NOT x |
| 36 | Tunnel Vision | Resynthesizes each output from its own cone and parity structure, ignoring the rest |
| 37 | Goodwill Hunting | Reuses a checked AND and rewrites blocks that are measured out, on the first paper's circuits |
| 38 | Google Translate | Compiles a classical record to Clifford+T and proves the result equal by SAT |
| 39 | Tetris | A lookup-table mapping method for large combinational circuits |
| 40 | Last Dance | A chirped schedule for the tournament: the perturbation shrinks by a constant ratio each round |
| 41 | Battle Royale | The tournament: every candidate is judged on every input, a planted cheater must be caught each round, the last circuit standing is kept |
| 42 | Richard | The phase optimizer for Clifford+T circuits |
| 43 | Parallax | A second view of the phases, placing each at the last occurrence of its parity |
| 44 | Speed Dating | Chooses a cut by building the real circuit for the best-scoring cuts and comparing |
| 45 | Don't Ask, Don't Tell | Rebuilds the visible phases at one cut and ignores unreachable input patterns |
| 46 | Allegedly (Confirmed) | Keeps a merge guessed from sampled agreement only if it is proven |
| 47 | The Old College Try Hard | The baseline pass with additional effort, reported as its own column |
| 48 | Holy Water | Reduces a rewrite-rule set to the rules that do work |
| 49 | I Think This Is XOR Math (Nailed It) | Rewrites arithmetic and symmetric structure as sums of AND-products over XOR |

Suggested usage: "We then used Speed Dating, followed by Last Dance." No technique changes a circuit's lines, constants, garbage or function, and every result is proven on its own against the file. Where the repository supports a per-technique result, it is: Holy Water produced 9symml (14,193 to 176) and sym9 (2,077 to 176); #7 took C7552 to 440; Field of Dreams produced the four multipliers; in the T-count lane, Speed Dating with Don't Ask, Don't Tell won on 16 of 33 circuits, Allegedly (Confirmed) on 6 (the Barenco family 224 to 72, which ties Maslov's 2016 construction and is not a record), The Old College Try Hard on 2 alone, Battle Royale's later rounds on 1 (hwb8, by one T), and Parallax on none of the 33. Richard holds zero records. We have no per-technique totals for the RevLib records and give none. Google Translate, on the earlier ledger, proved 175 of 213 compilations of classical records equal and then cut 126 of them from 306,978 T to 136,568 (−56%); that ledger predates the withdrawals and no published T-counts exist to compare it with.

## 6. Results

Computed from `records/RECORDS.tsv`, claimed rows only.

| measure | value |
|---|---|
| entries beaten | 183 of 456 |
| total quantum cost, best published to ours | 14,391,810 to 7,891,148 (−45.2%) |
| mean per-circuit reduction | 41.1% |
| reduction ≥ 50% / 25-50% / 10-25% / < 10% | 71 / 51 / 31 / 30 |
| cut to a tenth or less | 8 |

**Largest cuts in quantum cost saved.**

| circuit | size | published | ours | cut |
|---|---|---:|---:|---:|
| cpu_alu_32bit_329 | 756_685_722 | 2,235,491 | 799,629 | 64.2% |
| cpu_alu_16_376 | 142_103_124 | 1,281,717 | 437,933 | 65.8% |
| cpu_alu_16bit_328 | 404_365_386 | 662,531 | 205,861 | 68.9% |
| alu_flat_32_337 | 230_164_134 | 1,380,526 | 971,315 | 29.6% |
| alu_32_359 | 325_259_229 | 1,230,577 | 861,042 | 30.0% |
| cordic | 25_2_23 | 349,522 | 1,778 | 99.5% |

**Largest cuts in percent.**

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

**By family** (function names grouped by us).

| family | entries | published | ours | cut |
|---|---:|---:|---:|---:|
| ALU and CPU datapaths | 40 | 12,241,998 | 6,801,763 | 44.4% |
| MCNC/PLA-style named functions | 63 | 1,401,514 | 625,840 | 55.3% |
| Synthesised-program circuits (bubblesort, control structures) | 37 | 584,978 | 369,780 | 36.8% |
| Symmetric and arithmetic benchmarks | 33 | 87,780 | 22,928 | 73.9% |
| hwb and urf | 10 | 75,540 | 70,837 | 6.2% |

The structured families fall furthest; hwb and urf, built to have no structure, barely move. The 32-bit ALU is now a 1-bit ALU with ambition.

### 6.1 Against the earlier published methods

RevLib lists, for each entry, circuits from earlier methods; the Maslov collection and Lee et al. add others. By the source of the bar:

| bar from | entries | published total | ours | cut | largest ratio |
|---|---:|---:|---:|---:|---|
| RevLib | 172 | 14,314,731 | 7,820,707 | 45.4% | 196.6× (cordic) |
| Maslov collection | 7 | 22,089 | 18,395 | 16.7% | 11.8× (sym9) |
| Lee et al. (urf) | 4 | 54,990 | 52,046 | 5.4% | 1.1× (urf2) |

For hwb7, hwb8 and hwb9 on 0 garbage lines, the Maslov-page bars are 1,847, 5,136 and 12,707 and ours are 1,539, 4,451 and 12,047. Whether they also beat the best published hwb circuits beyond that page is unchecked, so we make no claim against Zakablukov. For the multipliers, the earlier methods are Vandaele's FastTODD (155, 205, 257, 315) and binary-field plus FastTODD (111, 135, 185, 207); ours are lower by 1.5× to 1.8× and by 1.1× to 1.3×.

## 7. Stricter evaluation: what we found wrong

Nobody caught these for us. Agents found every one by auditing their own claims against the files RevLib actually serves, and then took the claims back. A leaderboard that cannot lose entries is a press release.

| finding | effect |
|---|---|
| **Verifier `g` bug.** The intended-reading check treated any output named `g` as a garbage marker. In 17 RevLib files `g` is an ordinary variable, so a line was never compared (plus63mod4096_163 differs from RevLib on 128 of 4,096 inputs). | 17 claims withdrawn: 4gt4-v0_73, ham7_106, ham7_299, ham15_109, ham15_298, hwb7_302, hwb8_303, hwb9_304, cycle10_2_110, cycle10_293, cycle17_3_112, and six plus63/127mod rows |
| **Jung2021-reachable.** Jung and Choi (2021) publish 19, 13, 18 and 22 for the 4gt family; we reproduced them under RevLib's own embedding. | 4gt4-v0_72, 4gt5_77, 4gt10-v1_81, 4gt12-v0_86 withdrawn |
| **Wang2015 hwb6 = 102.** Wang et al. give 102 in RevKit's table for a circuit we rebuilt and checked. Ours is 103. | hwb6_56 withdrawn |
| **rd32_271 is wrong against its source**, rd32_272. | rd32_271 withdrawn |
| **cpu_register sibling.** RevLib lists a same-size sibling at 2,232; our 9,716 could not be verified against an unobtainable file. | cpu_register_16_332 withdrawn |
| **arb8_322.** Our reference was a 39-line file RevLib does not serve; RevLib's has 60 lines and ours differed on 219 of 256 inputs. | withdrawn 2026-10-22 |
| **Proofs older than the verifier fix.** | alu_flat_32_457, mult_stmts_32_345 withdrawn pending a new proof |

That is 17 + 4 + 1 + 1 + 1 + 2 = 26 files withdrawn on 2026-10-23, plus arb8_322 the day before: 27. Three more rows (mini-alu, mod10, urf6) are unclaimed because a lower published number exists (Section 8).

**Dual encoders.** When the records were proven, every SAT row went through two independently written encoders, revverify's and the toolchain's own; a disagreement is a bug in one of them. Only revverify's is public, so the public replay re-checks with that one plus the DRAT proof, and cannot replay both. **DRAT.** kissat's refutation of each miter is replayed by drat-trim; one proof, for alu_flat_32_457 (now pending a new proof, Section 7), was 5.2 GB, and drat-trim read every byte. **Our replay script lied.** The public `tools/verify_all.sh` claimed DRAT-checked certificates but checked only the solver's exit code. We found it, and the fixed script takes drat-trim as a fourth argument; a SAT row passes only as `UNSAT_DRAT_VERIFIED`. Its control is a fake solver that exits 20 with an empty proof: with drat-trim it gets `DRAT_FAILED`, without it, it passes. **Planted cheaters.** Both provers and the tournament run with deliberately wrong circuits that must be refused; a prover that has never caught a cheater has not been tested. **Maury.** Technique #33 is all of the above under one name: an optimizer's claim is the beginning of an argument.

## 8. Comparison with other work

| circuit | published | ours | source | note |
|---|---:|---:|---|---|
| mini-alu 4_0_0 | 16 | 26 | Szyprowski and Kerntopf | like-for-like, no claim |
| Modulo10Counter 4_0_0 | 21 | 25 | Szyprowski and Kerntopf | 25 is proven minimal in MCT for RevLib's file by our exact search; their function or library reading differs (unresolved) |
| urf6 15_0_0 | below 17,046 | 16,793 | arXiv:1004.1697 | no claim |
| hwb5 5_0_0 | 71, author-reported | 73 | X. Wang, Maslov page | their circuit recosts to 77 in RevKit's table |
| plus63mod4096, plus63mod8192, plus127mod8192 | 4,943; 16,580; 16,455 | rows withdrawn | Wang et al. 2015 | |
| 13 circuits: alu4, tial, f51m, max46, misex3, misex3c, sqr6, C7552, apex2, ex5p, frg2, seq, spla | lower | | Datta 2013, Schonborn 2014, Rahman and Rice 2016, Bandyopadhyay 2020 | mixed-polarity gates, which RevLib's files lack; not like-for-like, no ranking |
| GF(2^n), n = 7 to 10 | 44, 58, 70, 92 | 103, 113, 139, 177 | AlphaTensor-Quantum with gadgets | gadgets are a different model |
| GF(2^7), GF(2^8) | 96; 116 | 103; 113 | arXiv:2605.15476 (clean ancillas) | lower at n = 7, higher at n = 8; n = 9, 10 not reported |

Lee et al.'s urf4 (68,933) computes a different function from RevLib's urf4, so it sets no bar and earns no claim. Four ALU rows are proven only by sampling on 1,048,576 random inputs and are not claims. We found no 2022 to 2026 paper reporting RevLib quantum costs, and we cannot prove that none exists.

## 9. Limitations

- **Bars are ours.** We collected them. A lower published circuit we did not find falsifies that row.
- **Cost model.** RevKit's table prices everything; other papers use other cost models, and where their circuit was available we re-priced it. Maslov circuits were matched to RevLib functions by equivalence (81 of 173).
- **Rules.** Ancilla, gadget and mixed-polarity papers play different games and are listed, never ranked. The multipliers beat AlphaTensor-Quantum under no ancillas, no measurements and no gadgets, and nowhere else.
- **Thin margins.** n = 7 is lower by one T.
- **Trusted base.** The verifier, kissat, drat-trim and the CNF encoding; not the optimizer.
- **Independent checks.** QCEC confirmed n = 7 only. The multipliers rest on the path-sum verifier and its negative controls.
- **Novelty.** The techniques were checked against a bounded set of sources; a bounded search is not proof that nothing else exists.
- **Understanding.** As in the first paper, the author declines to say what a Hamiltonian is, and the grader never asked. The agents' journal records that one of them, on first meeting the word Toffoli, took it for a dessert.

## 10. Artifacts and acknowledgments

Everything released is at github.com/HereLiesRepo/HereLiesRevLib: every circuit, `records/RECORDS.tsv`, `records/PROOFS.tsv` (a SHA-256, verdict and solve time per circuit), `verifier/revverify.c`, the path-sum verifier and the phase records. Circuits are under `records/cc-by-nc-sa/NOTICE.md`. The compiler and search tools are not released.

```
cc -O2 -o revverify verifier/revverify.c
sh tools/verify_all.sh <directory of RevLib .real files> <path to kissat> out.tsv <path to drat-trim>
make -f phase/Makefile.phase proof
```

Expected: every claimed row `EQUAL_EVERY_INPUT` or `UNSAT_DRAT_VERIFIED`. Without the fourth argument the SAT rows report `UNSAT_EQUAL`, which is only the solver's exit code and not a certificate check. Full replay with drat-trim: every one of the 183 claimed rows passed on Oct 23, 2026: 102 `EQUAL_EVERY_INPUT`, 81 `UNSAT_DRAT_VERIFIED`, 0 failures, in 3,159 s wall time with `VERIFY_JOBS=4` on a 4-vCPU Xeon at 2.10 GHz (11,807 s summed over rows; slowest simple_alu_32_459, 1,397 s including its proof check; result file SHA-256 `9c79c19fba18a776bb5cb54652450361858ce865b1f9d7ca79258395bcd0bcb8`). Also expected: the four phase records `EQUAL`. Recorded runtime for the 183 claimed rows is 5,867 single-core seconds (about 1.6 hours), the slowest being simple_alu_32_459 at 742 s (`records/PROOFS.tsv`, column 7). Memory was not measured.

**Credits.** RevLib is the work of R. Wille, D. Grosse, L. Teuber, G. W. Dueck and R. Drechsler. The starting decompositions for the multipliers are AlphaTensor-Quantum's (Ruiz et al.); the benchmarks are Amy's (Feynman). Where Vandaele (arXiv:2407.08695) and arXiv:2605.15476 (Wang, Yu, Wu and Cong) are lower or comparable, we say so above. Prior work on the optimization methods is credited in Section 5.

**AI assistance.** The circuits, the verifiers, the corrections to the circuits and the verifiers, and the audits were produced by AI coding agents: Claude (Anthropic) and Codex (OpenAI), seven in all. Every verdict in this paper is the verifier's or the prover's own output, not an agent's report; figures for other work are cited from their sources. The author supplied the names, the parity structure, the insistence on a release note in a particular voice, and the question "is it running", and has still not looked up what a DRAT proof is. He was asleep for most of this, and the ledger agrees.[^usage]

[^usage]: We wanted to do more, but ran out of usage (*cough* Anthropic *cough*).

## References

1. R. Wille, D. Große, L. Teuber, G. W. Dueck, R. Drechsler. RevLib: An Online Resource for Reversible Functions and Reversible Circuits. ISMVL 2008, 220-225. doi:10.1109/ISMVL.2008.43
2. D. Maslov. Reversible Logic Synthesis Benchmarks Page. reversiblebenchmarks.github.io
3. H. Lee, K. C. Jeong, D. Han, P. Kim. An Algorithm for Reversible Logic Circuit Synthesis Based on Tensor Decomposition. ACM Trans. Quantum Computing 5(3), 2024. arXiv:2107.04298
4. M. Szyprowski, P. Kerntopf. Reducing Quantum Cost in Reversible Toffoli Circuits. arXiv:1105.5831
5. X. Wang et al. J. Mult.-Valued Logic Soft Comput. 25 (2015) 643-671.
6. Jung, Choi. PLOS ONE 16(6): e0253140, 2021.
7. Mixed-polarity work: Datta et al., RC 2013; Schonborn et al., DDECS 2014; Rahman, Rice, IJCIT 5(1), 2016; Bandyopadhyay et al., VDAT 2020 (full titles unverified).
8. F. J. R. Ruiz et al. Quantum circuit optimization with AlphaTensor. Nature Machine Intelligence 7, 374-385, 2025. arXiv:2402.14396
9. Vandaele. Lower T-count with faster algorithms. arXiv:2407.08695.
10. Wang, Yu, Wu, Cong. Quantum Circuit Synthesis Using an Exact T Library. arXiv:2605.15476, 2026.
11. L. E. Heyfron, E. T. Campbell. An efficient quantum compiler that reduces T count. Quantum Sci. Technol. 4(1), 015004, 2018. arXiv:1712.01557
12. M. Amy, D. Maslov, M. Mosca. Polynomial-time T-depth optimization of Clifford+T circuits via matroid partitioning. IEEE TCAD 33(10), 2014. arXiv:1303.2042
13. M. Amy. Towards Large-scale Functional Verification of Universal Quantum Circuits. EPTCS 287, 2019. doi:10.4204/EPTCS.287.1
14. L. Burgholzer, R. Wille. Advanced Equivalence Checking for Quantum Circuits. IEEE TCAD 2021. doi:10.1109/TCAD.2020.3032630
15. A. Biere, K. Fazekas, M. Fleury, M. Heisinger. CaDiCaL, Kissat, Paracooba, Plingeling and Treengeling Entering the SAT Competition 2020. B-2020-1, 50-53.
16. N. Wetzler, M. J. H. Heule, W. A. Hunt Jr. DRAT-trim. SAT 2014, LNCS 8561, 422-429. doi:10.1007/978-3-319-09284-3_31
17. A. Mishchenko, M. Perkowski. Fast Heuristic Minimization of Exclusive-Sums-of-Products. RM 2001.
18. A. Kuehlmann et al. Robust Boolean reasoning for equivalence checking (SAT sweeping). IEEE TCAD 2002.
19. A. Bernasconi et al. XOR-AND-XOR Logic Forms for Autosymmetric Functions and Applications to Quantum Computing. IEEE TCAD 42(6), 1861-1872, 2023. doi:10.1109/TCAD.2022.3213214
20. D. Maslov, G. W. Dueck, D. M. Miller. Toffoli network synthesis with templates. IEEE TCAD 24(6), 2005. doi:10.1109/TCAD.2005.847911
21. D. Grosse et al. Exact Multiple-Control Toffoli Network Synthesis With SAT Techniques. IEEE TCAD 28(5), 2009. doi:10.1109/TCAD.2009.2017215
22. M. Amy, D. Maslov, M. Mosca, M. Roetteler. A meet-in-the-middle algorithm for fast synthesis of depth-optimal quantum circuits. IEEE TCAD 32(6), 2013. doi:10.1109/TCAD.2013.2244643
23. P. E. Hart, N. J. Nilsson, B. Raphael. A Formal Basis for the Heuristic Determination of Minimum Cost Paths. IEEE Trans. Systems Science and Cybernetics 4(2), 1968. doi:10.1109/TSSC.1968.300136
24. H. R. Lourenco, O. C. Martin, T. Stutzle. Iterated Local Search. Handbook of Metaheuristics, 2003. arXiv:math/0102188
25. A. Barenco et al. Elementary gates for quantum computation. Phys. Rev. A 52, 3457, 1995. arXiv:quant-ph/9503016
26. C. Gidney. Halving the cost of quantum addition. Quantum 2, 74, 2018. arXiv:1709.06648
27. D. E. Goldberg, K. Deb. A Comparative Analysis of Selection Schemes Used in Genetic Algorithms. Foundations of Genetic Algorithms 1, 1991.
