# HereLiesRevLib

Every circuit in [RevLib](https://www.revlib.org) (Bremen), re-optimized under its own rules: same lines, same gate
library (MCT, NCT, MCT+P, MCF, MCT+MCF, NCV), same quantum-cost table (RevKit). A circuit counts as beaten only when
our rewrite **dominates** it: gates ≤ and quantum cost ≤, one strictly.

**[RELEASE.nfo](RELEASE.nfo)**: the release notes, the State of the RevLib Address and the hour-by-hour clock (what the owner asked, what ran on its own).

## Tally (`results/score.sh`, 2026-10-20; a snapshot from before tonight's 26 withdrawals, which it still counts)

| | circuits | beaten | exact | sampled |
|---|---|---|---|---|
| every realization with a RevLib table row | 415 | 191 | 103 | 88 |
| RevLib's best realization per function | 225 | 71 | 41 | 30 |

- **exact**: equivalent on every free-input assignment (`verifier/revverify.c`), or proven equal on every input by a SAT miter (`verifier/revverify.c` miter + a SAT solver such as kissat, UNSAT). A dropped control is caught as `equivalent=NO` / SAT.
- **sampled**: equal on 1,048,576 random assignments. Evidence, not proof; listed apart and never mixed in.
- Six circuit files have no row in RevLib's table, so no library and no record: excluded.

## Below the best published circuit: 183 records (records/)

Against every published circuit we collected (RevLib, the reversiblebenchmarks.github.io best-known collection, and
Lee et al. 2021), in RevLib's own cost table and RevLib's own gate library for that file, proven on every input. Full
list and caveats: [records/README.md](records/README.md), generated from records/RECORDS.tsv. Highlights:

| function | lines | best published | ours | how |
|---|---|---|---|---|
| cordic | 25 | 349,522 (RevLib) | **1,778** | rebuilt (23-input truth table) |
| 9symml | 10 | 14,193 (RevLib) | **176** | rebuilt: Holy Water |
| z4ml | 11 | 642 (RevLib) | **81** | rebuilt |
| cm85a | 14 | 2,252 (RevLib) | **416** | rebuilt |
| sqn | 10 | 2,122 (RevLib) | **328** | rebuilt |
| sym9 | 10 | 2,077 (Maslov collection) | **176** | rebuilt: Holy Water |
| sym6 | 7 | 224 (Maslov collection) | **90** | rebuilt |
| add6 | 19 | 6,455 (RevLib) | **499** | rebuilt |
| rd84 | 12 | 2,749 (RevLib) | **358** | rebuilt |
| clip | 14 | 6,731 (RevLib) | **899** | rebuilt |
| apex4 | 28 | 237,963 (RevLib) | **56,230** | rebuilt |
| hwb7 | 7 | 1,847 (Maslov collection) | **1,539** | optimized |
| hwb8 | 8 | 5,136 (Maslov collection) | **4,451** | optimized |
| urf1 | 9 | 11,646 (Lee et al. 2021) | **10,979** | optimized |
| urf2 | 8 | 4,538 (Lee et al. 2021) | **4,189** | optimized |
| urf3 | 10 | 30,065 (Lee et al. 2021) | **28,671** | optimized |

Every circuit here came out of **Witchcraft(TM)**, our own optimizer and compiler toolchain, built in-house with no
outside libraries. Witchcraft is not in this repository and its recipe is not described here. What this repository does
publish is everything you need to check us: every circuit, every proof, and a verifier small enough to read in one
sitting. "Rebuilt" means Witchcraft generated the circuit from the function's truth table; "optimized" means it
started from the published circuit. Runs continue; numbers only go down, except the 26 we took back (see records/README.md).

Prior work we build on, credited (the papers are theirs; the selection, the engineering and the records are ours):
Bernasconi, Berti, Ciriani, Del Corso & Fulginiti, IEEE TCAD 42(6), 2023 (doi:10.1109/TCAD.2022.3213214); Keren, Levin &
Stankovic; Mishchenko & Perkowski, RM Workshop 2001; Meinel, Somenzi & Theobald, DAC 1997 and IEEE TCAD 19(5), 2000;
Stankovic & Astola 2002; Karpovsky, Stankovic & Astola, IEEE TC 52(5), 2003; Chen & Stade, SAS 2023; Jang et al.,
Quantum 6, 798 (2022); Kuehlmann et al. (SAT sweeping); for the line-reuse candidates, Wille, Soeken & Drechsler, DAC 2010 (line reduction by merging garbage
outputs with constant inputs), Paler, Wille & Devitt, Phys. Rev. A 94, 042337 (2016) (wire recycling), Parent, Roetteler & Svore
(REVS, arXiv:1510.00377), Meuli et al., DATE 2019 (reversible pebbling) and Bennett 1973; and the owner's own ring-parity construction (checkmate) and
SpaceTime #7, which had no published match in our search.

## Techniques

All named by the owner. All of them live in Witchcraft(TM). How they work is ours; what they did is in the tables.

- **Battle Royale**: the tournament. Every candidate is judged on every input, a planted cheater has to be caught every
  round, last circuit standing wins.
- **Holy Water**: blesses a pile of rules until there are fewer of them.
- **Speed Dating**: picks a partner the expensive way, by actually going out with every one.
- **Don't Ask, Don't Tell**: ignores what can never be seen.
- **Allegedly (Confirmed)**: guesses, then proves. A guess that fails the proof never happened.
- **I Think This Is XOR Math (Nailed It)**: it was XOR math.
- **Richard**: the phase optimizer. It beat Feynman. It has 0 records ([phase/](phase/README.md)).
- **Field of Dreams**: we built it, and the multipliers came. Four of them, all under AlphaTensor-Quantum's no-gadget counts.
- **Kevin Gates**: has opinions about gates that never had a chance.
- **Etch A Sketch**: draws a circuit, shakes it, and nobody is upset.
- **Through the Looking Glass**: finds what was always on the other side.
- **Maury: You Are NOT the Circuit**: the reason every record here is proven. We run the test; the answer is on the card.
- **Victoria's Secret**: reveals there was less underneath than advertised.
- **Uno Reverse Card**: when a gate is about to cost you, play it back.
- **Tunnel Vision**: ignores everything off to the side, and wins anyway.
- **Goodwill Hunting**: took modinv-point-add under a million Toffolis. It was not our fault. It's not your fault either.
- **Google Translate**: speaks fluent Clifford+T, and we checked it didn't lose anything in translation.
- **Tetris**: packs the pieces. The lines clear.
- **Last Dance**: the party winds down and the circuit goes home smaller.
- **The phase lane's records**: gf2^7_mult 103 T, gf2^8_mult 113, gf2^9_mult 139, gf2^10_mult 177, under
  AlphaTensor-Quantum's no-gadget 104, 123, 161 and 196, built from AlphaTensor-Quantum's own small-field decompositions (credit theirs), proven
  EQUAL ([phase/records/](phase/records/RECORDS.tsv)). Richard still has 0; a different spell did these.

All 22 are named by the owner (the full list is in the paper).

## What this is NOT (read before quoting it)

RevLib's table is from around 2010–2012. Later papers beat many of its entries, so **beating RevLib's table is not a
world record**. Checked so far: mini_alu qc 16 and the 4-bit exact quantum-cost minima (Szyprowski & Kerntopf,
arXiv:1105.5831), urf1 16,619 and urf2 6,600 (arXiv:1004.1697). Ours: mini_alu 26, urf1 16,658, urf2 6,963. Those
RevLib-start numbers are not records; starting from Lee et al.'s circuits instead, urf1/2/3/5 are (see above).

## Proven optimal (complete exact search, `proofs/mitm_receipts.txt`)

| circuit | library | RevLib qc | proven optimum |
|---|---|---|---|
| toffoli_double_4 | MCT | 10 | **7** |
| rd32-v1_68 | MCT | 13 | **10** |
| rd32-v1_69 | MCT+P | 9 | 9 (RevLib's is already optimal) |

Each receipt prints states, cap, inserted, re-costed, duplicate and dropped states; OPTIMAL is printed only when no
cheaper circuit can lie outside the range the search fully covered.

## RevLib best-per-function realizations we beat exactly

Snapshot of `results/score.tsv` before tonight's withdrawals; the seven files withdrawn for the checker defect
(hwb7_302, hwb8_303, hwb9_304, cycle10_293, plus63mod4096_309, plus63mod8192_310, plus127mod8192_308) are removed.
Some rows here beat RevLib's own circuit but are not records (4gt10, mod10, urf6, mini_alu); records/README.md is the record list.

| circuit | library | function | gates | quantum cost |
|---|---|---|---|---|
| 0410184_169.real | NCT | 0410184 | 46 → **45** | 90 → **89** |
| 4gt10-v1_81.real | MCT | 4greaterthan10 | 6 → **6** | 34 → **26** |
| 4gt4-v1_74.real | MCT | 4greaterthan4 | 5 → **5** | 57 → **53** |
| alu2_199.real | MCT | alu2 | 157 → **156** | 5654 → **5649** |
| alu4_201.real | MCT | alu4 | 1063 → **1061** | 55388 → **55386** |
| apex4_202.real | MCT | apex4 | 5376 → **5374** | 237963 → **237961** |
| arb8_323.real | MCT+MCF | Arbiter(with8clients) | 24 → **22** | 746 → **537** |
| bw_291.real | MCT | bw | 307 → **305** | 943 → **933** |
| cnt3-5_179.real | MCT | TernaryCounter | 25 → **20** | 65 → **60** |
| dc2_222.real | MCT | dc2 | 75 → **74** | 1886 → **1881** |
| decod24-enable_125.real | MCT | Decod24withenable | 9 → **8** | 21 → **20** |
| ex5p_296.real | MCT | ex5p | 647 → **640** | 1843 → **1824** |
| example2_231.real | MCT | example2 | 157 → **156** | 5654 → **5649** |
| in0_235.real | MCT | in0 | 338 → **334** | 20031 → **20027** |
| miller_11.real | MCT | Millergate | 5 → **5** | 17 → **9** |
| mini_alu_305.real | MCT | mini-alu | 20 → **19** | 60 → **59** |
| mod10_176.real | MCT | Modulo10Counter | 7 → **7** | 43 → **35** |
| pdc_307.real | MCT | pdc | 2080 → **2045** | 6500 → **6413** |
| sf_275.real | MCT | SampleFunction | 11 → **8** | 51 → **44** |
| spla_315.real | MCT | spla | 1709 → **1696** | 5925 → **5864** |
| sqr6_259.real | MCT | sqr6 | 81 → **80** | 1033 → **1012** |
| sqrt8_260.real | MCT | sqrt8 | 40 → **39** | 622 → **621** |
| urf1_278.real | MCT | UnstructuredReversibleFunction1 | 6761 → **6401** | 17030 → **16658** |
| urf2_154.real | MCT | UnstructuredReversibleFunction2 | 620 → **620** | 16152 → **16144** |
| urf2_277.real | MCT | UnstructuredReversibleFunction2 | 3144 → **3004** | 7111 → **6963** |
| urf3_279.real | MCT | UnstructuredReversibleFunction3 | 14075 → **13346** | 37631 → **36882** |
| urf4_187.real | MCT | UnstructuredReversibleFunction4 | 32004 → **31245** | 160020 → **155817** |
| urf5_280.real | MCT | UnstructuredReversibleFunction5 | 5097 → **4781** | 14607 → **14255** |
| urf6_281.real | MCT | UnstructuredReversibleFunction6 | 5088 → **4720** | 17161 → **16793** |
| x2_267.real | MCT | x2 | 38 → **38** | 625 → **609** |

## Layout

- `circuits/exact/`, `circuits/sampled/`: our .real files, named as RevLib's originals.
- `results/score.tsv`: every circuit, ours vs theirs; `results/revlib_table.tsv`: RevLib's table as scraped.
- `proofs/`: verifier output and exact-search receipts. `verifier/revverify.c`: standalone C11, `cc -O2`.
- `notes/clock.txt`: the UTC timeline.

Tools: our optimizer (private). The checking is public: `verifier/revverify.c` and `tools/verify_all.sh` re-check every record.
Credit: RevLib, R. Wille, D. Große, L. Teuber, G. W. Dueck, R. Drechsler (ISMVL 2008).
