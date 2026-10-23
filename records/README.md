# Below the best published circuit

Every row: a RevLib function at a fixed size (lines_constants_garbage), where our circuit has a lower quantum cost
(RevKit's table, the one RevLib publishes with) than **every published circuit we collected** for that function at that
size, and stays inside the gate library of RevLib's own file. Generated from `RECORDS.tsv` by
`tools/records_readme.sh`.

Sources collected:
- RevLib's realizations (457 table rows). The bar is the lower of RevLib's listed cost and our recost of its file.
- The best-known collection at [reversiblebenchmarks.github.io](https://reversiblebenchmarks.github.io/) (Maslov et al.):
  173 circuits, 81 matched a RevLib function by exact equivalence. Negative controls are written as NOT, gate, NOT and paid for.
- Lee, Jeong, Han, Kim, arXiv:2107.04298 (ACM TQC 2024), [circuits](https://github.com/ReversibleLogicCircuit/SizeReduction) (urf1, 2, 3, 5).

**Check.** exact: equal to RevLib's file on every input (`verifier/revverify.c`). exact (SAT): equal on every input
by a SAT miter proof (`verifier/revverify.c` miter + a SAT solver such as kissat, UNSAT). exact (intended): equal on every input with the file's
.garbage read the way it is meant (`revverify --intended`): several RevLib files write the string reversed (sao2_257:
constants 0000----------, garbage 1111111111---- marks the four function outputs as garbage and four inputs as kept;
reversed, the ten inputs are garbage and the outputs are compared). Output names "g" decide when a file uses them; the
reversal applies only otherwise, when the string as written marks more constant lines than its reverse. sampled: equal on at least 1,048,576 random inputs (tournament winners: 2,097,152); evidence, not a
record, listed apart.

**Claimed: 209 rows** (22 of them by SAT proof). Sampled, pending proof: 4. Not claimed: 3.

## Claimed (exact)

| RevLib file | function | lines_const_garbage | best published (source) | ours qc | ours gates | check |
|---|---|---|---|---|---|---|
| 4gt10-v1_81 | 4greaterthan10 | 5_1_4 | 34 (revlib:4gt10-v1_81.real) | **26** | 6 | exact |
| 4gt12-v0_86 | 4greaterthan12 | 5_1_4 | 41 (revlib:4gt12-v0_88.real) | **39** | 7 | exact (intended) |
| 4gt4-v0_72 | 4greaterthan4 | 5_1_4 | 54 (revlib:4gt4-v0_72.real) | **41** | 9 | exact (intended) |
| 4gt4-v0_73 | 4greaterthan4 | 5_0_0 | 89 (revlib:4gt4-v0_73.real) | **56** | 16 | exact (intended) |
| 4gt5_77 | 4greaterthan5 | 5_1_4 | 21 (revlib:4gt5_75.real) | **16** | 4 | exact (intended) |
| 4mod5-bdd_287 | Check4modulo5 | 7_3_6 | 24 (revlib:4mod5-bdd_287.real) | **9** | 5 | exact (intended) |
| 5xp1_194 | 5xp1 | 17_10_7 | 1406 (revlib:5xp1_194.real) | **650** | 90 | exact (intended) |
| 9symml_195 | 9symml | 10_1_9 | 14193 (revlib:9symml_195.real) | **176** | 26 | exact (intended) |
| C17_204 | C17 | 7_2_5 | 99 (revlib:C17_204.real) | **74** | 12 | exact (intended) |
| C7552_205 | C7552 | 21_16_5 | 1728 (revlib:C7552_205.real) | **385** | 112 | exact (intended) |
| add6_196 | add6 | 19_7_12 | 6455 (revlib:add6_196.real) | **499** | 45 | exact (intended) |
| adr4_197 | adr4 | 13_5_8 | 727 (revlib:adr4_197.real) | **164** | 25 | exact (intended) |
| alu-bdd_288 | ALUbyGuptaetal. | 7_2_6 | 29 (revlib:alu-bdd_288.real) | **24** | 8 | exact (intended) |
| alu1_198 | alu1 | 20_8_12 | 228 (revlib:alu1_198.real) | **160** | 36 | exact (intended) |
| alu2_199 | alu2 | 16_6_10 | 5654 (revlib:alu2_199.real) | **3763** | 212 | exact (intended) |
| alu3_200 | alu3 | 18_8_10 | 2632 (revlib:alu3_200.real) | **1763** | 105 | exact (intended) |
| alu4_201 | alu4 | 22_8_14 | 55388 (revlib:alu4_201.real) | **40408** | 1082 | exact (intended) |
| alu_16_334 | ALU | 117_83_69 | 258872 (revlib:alu_16_334.real) | **76849** | 2400 | exact_sat_intended |
| alu_16_358 | ALU | 165_131_117 | 146385 (revlib:alu_16_358.real) | **44332** | 2428 | exact_sat_intended |
| alu_318 | ALU | 331_265_235 | 1336477 (revlib:alu_318.real) | **971693** | 11261 | exact_sat_intended |
| alu_32_359 | ALU | 325_259_229 | 1230577 (revlib:alu_32_359.real) | **861042** | 10370 | exact_sat_intended |
| alu_32_431 | ALU | 326_260_230 | 150928 (revlib:alu_32_431.real) | **111306** | 10212 | exact_sat_intended |
| alu_flat_16_336 | FlattenedALU | 118_84_70 | 181662 (revlib:alu_flat_16_336.real) | **53819** | 2400 | exact_sat_intended |
| alu_flat_16_360 | FlattenedALU | 182_148_134 | 146496 (revlib:alu_flat_16_360.real) | **45014** | 2694 | exact_sat_intended |
| alu_flat_16_384 | FlattenedALU | 67_33_19 | 363012 (revlib:alu_flat_16_384.real) | **238084** | 5023 | exact_sat_intended |
| alu_flat_16_408 | FlattenedALU | 119_85_71 | 38657 (revlib:alu_flat_16_408.real) | **13539** | 2469 | exact_sat_intended |
| alu_flat_16_432 | FlattenedALU | 183_149_135 | 35263 (revlib:alu_flat_16_432.real) | **13347** | 2714 | exact_sat_intended |
| alu_flat_16_456 | FlattenedALU | 68_34_20 | 77002 (revlib:alu_flat_16_456.real) | **54039** | 5148 | exact_sat_intended |
| alu_flat_32_337 | FlattenedALU | 230_164_134 | 1380526 (revlib:alu_flat_32_337.real) | **971315** | 10311 | exact_sat_intended |
| alu_flat_32_361 | FlattenedALU | 358_292_262 | 1230784 (revlib:alu_flat_32_361.real) | **862419** | 10911 | exact_sat_intended |
| alu_flat_32_409 | FlattenedALU | 231_165_135 | 158241 (revlib:alu_flat_32_409.real) | **116770** | 10434 | exact_sat_intended |
| alu_flat_32_433 | FlattenedALU | 359_293_263 | 151135 (revlib:alu_flat_32_433.real) | **113150** | 10972 | exact_sat_intended |
| alu_flat_32_457 | FlattenedALU | 132_66_36 | 315850 (revlib:alu_flat_32_457.real) | **239620** | 21869 | exact_sat_intended |
| apex2_289 | apex2 | 498_459_495 | 5922 (revlib:apex2_289.real) | **5620** | 1644 | exact (SAT) |
| apex4_202 | apex4 | 28_19_9 | 237963 (revlib:apex4_202.real) | **56230** | 2748 | exact (intended) |
| apex5_290 | apex5 | 1025_908_939 | 10349 (revlib:apex5_290.real) | **9302** | 2590 | exact (SAT) |
| apla_203 | apla | 22_12_10 | 3438 (revlib:apla_203.real) | **1476** | 119 | exact (intended) |
| arb8_323 | Arbiter(with8clients) | 24_16_16 | 746 (revlib:arb8_323.real) | **276** | 15 | exact (intended) |
| avg16_324 | Averageof16 | 576_64_32 | 7452 (revlib:avg16_324.real) | **7320** | 3448 | exact (SAT) |
| avg8_325 | Averageof8 | 320_64_32 | 3741 (revlib:avg8_325.real) | **3662** | 1734 | exact (SAT) |
| bubblesort_16_340 | Bubblesortcircuit | 318_254_254 | 29327 (revlib:bubblesort_16_340.real) | **4339** | 1871 | exact_sat_intended |
| bubblesort_16_364 | Bubblesortcircuit | 812_748_748 | 21149 (revlib:bubblesort_16_364.real) | **3544** | 2604 | exact_sat_intended |
| bubblesort_16_388 | Bubblesortcircuit | 217_153_153 | 34374 (revlib:bubblesort_16_388.real) | **4643** | 2123 | exact_sat_intended |
| bubblesort_16_436 | Bubblesortcircuit | 813_749_749 | 12653 (revlib:bubblesort_16_436.real) | **3539** | 2599 | exact_sat_intended |
| bubblesort_16_460 | Bubblesortcircuit | 218_154_154 | 13830 (revlib:bubblesort_16_460.real) | **4018** | 2126 | exact_sat_intended |
| bubblesort_32_341 | Bubblesortcircuit | 622_494_230 | 58739 (revlib:bubblesort_32_341.real) | **21917** | 5564 | exact_sat_intended |
| bubblesort_32_365 | Bubblesortcircuit | 1596_1468_1204 | 42281 (revlib:bubblesort_32_365.real) | **30300** | 5606 | exact_sat_intended |
| bubblesort_32_389 | Bubblesortcircuit | 425_297_33 | 68766 (revlib:bubblesort_32_389.real) | **67343** | 5783 | exact (SAT) |
| bubblesort_32_437 | Bubblesortcircuit | 1597_1469_1205 | 24569 (revlib:bubblesort_32_437.real) | **21211** | 5661 | exact_sat_intended |
| bubblesort_32_461 | Bubblesortcircuit | 426_298_34 | 25950 (revlib:bubblesort_32_461.real) | **16207** | 6723 | exact (SAT) |
| bw_291 | bw | 87_82_59 | 943 (revlib:bw_291.real) | **578** | 280 | exact (intended) |
| callif_16_342 | Hierarchicalcircuit | 34_1_2 | 1522 (revlib:callif_16_342.real) | **802** | 210 | exact (SAT) |
| clip_206 | clip | 14_5_9 | 6731 (revlib:clip_206.real) | **899** | 75 | exact (intended) |
| cm150a_210 | cm150a | 22_1_21 | 1096 (revlib:cm150a_210.real) | **449** | 50 | exact (intended) |
| cm151a_211 | cm151a | 28_9_19 | 888 (revlib:cm151a_211.real) | **796** | 28 | exact (intended) |
| cm152a_212 | cm152a | 12_1_11 | 252 (revlib:cm152a_212.real) | **93** | 20 | exact (intended) |
| cm163a_213 | cm163a | 29_13_16 | 756 (revlib:cm163a_213.real) | **322** | 42 | exact (intended) |
| cm42a_207 | cm42a | 14_10_4 | 377 (revlib:cm42a_207.real) | **188** | 36 | exact (intended) |
| cm82a_208 | cm82a | 8_3_5 | 154 (revlib:cm82a_208.real) | **33** | 13 | exact (intended) |
| cm85a_209 | cm85a | 14_3_11 | 2252 (revlib:cm85a_209.real) | **416** | 35 | exact (intended) |
| cmb_214 | cmb | 20_4_16 | 910 (revlib:cmb_214.real) | **688** | 19 | exact (intended) |
| cnt3-5_179 | TernaryCounter | 16_5_5 | 65 (revlib:cnt3-5_179.real) | **60** | 20 | exact |
| co14_215 | co14 | 15_1_14 | 3488 (revlib:co14_215.real) | **1661** | 52 | exact (intended) |
| con1_216 | con1 | 9_2_7 | 206 (revlib:con1_216.real) | **155** | 24 | exact (intended) |
| cordic_218 | cordic | 25_2_23 | 349522 (revlib:cordic_218.real) | **1778** | 45 | exact (intended) |
| cps_292 | cps | 923_899_820 | 8487 (revlib:cps_292.real) | **8331** | 2703 | exact (intended) |
| cpu_alu_16_376 | CPUALUover16bit | 142_103_124 | 1281717 (revlib:cpu_alu_16_376.real) | **437933** | 4315 | exact_sat_intended |
| cpu_alu_16bit_328 | CPUALUover16bit | 404_365_386 | 662531 (revlib:cpu_alu_16bit_328.real) | **205861** | 2209 | exact_sat_intended |
| cpu_alu_16bit_400 | CPUALUover16bit | 405_366_387 | 63025 (revlib:cpu_alu_16bit_400.real) | **32715** | 2838 | exact_sat_intended |
| cpu_alu_16bit_424 | CPUALUover16bit | 2141_2102_2123 | 30208 (revlib:cpu_alu_16bit_424.real) | **425** | 187 | exact_sat_intended |
| cpu_alu_32bit_329 | CPUALUover32bit | 756_685_722 | 2235491 (revlib:cpu_alu_32bit_329.real) | **799629** | 6252 | exact_sat_intended |
| cpu_alu_32bit_353 | CPUALUover32bit | 6204_6133_6170 | 112396 (revlib:cpu_alu_32bit_353.real) | **671** | 344 | exact_sat_intended |
| cpu_alu_32bit_401 | CPUALUover32bit | 757_686_723 | 178783 (revlib:cpu_alu_32bit_401.real) | **107342** | 9164 | exact_sat_intended |
| cpu_control_unit_330 | CPUcontrolunit | 391_228_177 | 40433 (revlib:cpu_control_unit_330.real) | **37525** | 920 | exact_sat_intended |
| cpu_control_unit_402 | CPUcontrolunit | 392_229_178 | 10513 (revlib:cpu_control_unit_402.real) | **6936** | 1020 | exact_sat_intended |
| cpu_control_unit_426 | CPUcontrolunit | 647_484_433 | 7463 (revlib:cpu_control_unit_426.real) | **4166** | 1117 | exact_sat_intended |
| cpu_control_unit_450 | CPUcontrolunit | 291_128_77 | 20756 (revlib:cpu_control_unit_450.real) | **11759** | 1897 | exact_sat_intended |
| cpu_pc_427 | CPUprogramcounter | 93_69_82 | 609 (revlib:cpu_pc_427.real) | **71** | 31 | exact (intended) |
| cpu_register_16_332 | CPUregisterbench | 167_18_39 | 9833 (revlib:cpu_register_16_332.real) | **9716** | 768 | exact_sat_intended |
| cu_219 | cu | 25_11_14 | 1148 (revlib:cu_219.real) | **402** | 40 | exact (intended) |
| cycle10_293 | cycle10_2 | 39_27_27 | 202 (revlib:cycle10_293.real) | **153** | 57 | exact (intended) |
| cycle10_2_110 | cycle10_2 | 12_0_0 | 1202 (maslov:cycle10_2.real) | **1102** | 18 | exact (intended) |
| cycle17_3_112 | cycle17_3 | 20_0_0 | 6063 (maslov:cycle17_3.real) | **6001** | 47 | exact (intended) |
| dc1_221 | dc1 | 11_7_4 | 416 (revlib:dc1_221.real) | **165** | 37 | exact (intended) |
| dc2_222 | dc2 | 15_7_8 | 1886 (revlib:dc2_222.real) | **1175** | 74 | exact (intended) |
| decod24-bdd_294 | Decod24 | 6_4_2 | 27 (revlib:decod24-bdd_294.real) | **11** | 7 | exact (intended) |
| decod24-enable_125 | Decod24withenable | 6_3_2 | 21 (maslov:2-4dec.real) | **19** | 7 | exact (intended) |
| decod_217 | decod | 21_16_5 | 1728 (revlib:decod_217.real) | **385** | 112 | exact (intended) |
| dist_223 | dist | 13_5_8 | 7601 (revlib:dist_223.real) | **4429** | 278 | exact (intended) |
| dk17_224 | dk17 | 21_11_10 | 1559 (revlib:dk17_224.real) | **898** | 50 | exact (intended) |
| dk27_225 | dk27 | 18_9_9 | 248 (revlib:dk27_225.real) | **199** | 24 | exact (intended) |
| e64-bdd_295 | e64 | 195_130_130 | 907 (revlib:e64-bdd_295.real) | **821** | 389 | exact (SAT) |
| ex1010_230 | ex1010 | 20_10_10 | 155534 (revlib:ex1010_230.real) | **95738** | 3343 | exact (intended) |
| ex1_226 | ex1 | 6_1_5 | 7 (revlib:ex1_226.real) | **5** | 5 | exact (intended) |
| ex2_227 | ex2 | 6_1_5 | 141 (revlib:ex2_227.real) | **83** | 11 | exact (intended) |
| ex3_229 | ex3 | 6_1_5 | 79 (revlib:ex3_229.real) | **61** | 9 | exact (intended) |
| ex5p_296 | ex5p | 206_198_143 | 1843 (revlib:ex5p_296.real) | **1790** | 638 | exact (intended) |
| example2_231 | example2 | 16_6_10 | 5654 (revlib:example2_231.real) | **3763** | 212 | exact (intended) |
| f2_232 | f2 | 8_4_4 | 255 (revlib:f2_232.real) | **138** | 18 | exact (intended) |
| f51m_233 | f51m | 22_8_14 | 37400 (revlib:f51m_233.real) | **31932** | 1053 | exact (intended) |
| frg1_234 | frg1 | 31_3_28 | 15265 (revlib:frg1_234.real) | **14821** | 216 | exact (SAT) |
| frg2_297 | frg2 | 1219_1076_1084 | 12468 (revlib:frg2_297.real) | **12039** | 3635 | exact (SAT) |
| ham15_109 | HammingCode(over15variables) | 15_0_0 | 206 (maslov:ham15-109-214.real) | **204** | 107 | exact (intended) |
| ham15_298 | HammingCode(over15variables) | 45_30_30 | 309 (revlib:ham15_298.real) | **187** | 107 | exact (intended) |
| ham7_106 | HammingCode(over7variables) | 7_0_0 | 49 (maslov:ham7-25-49.real) | **48** | 24 | exact (intended) |
| ham7_299 | HammingCode(over7variables) | 21_14_14 | 141 (revlib:ham7_299.real) | **54** | 30 | exact (intended) |
| hwb5_300 | HiddenWeightedBit(over5variables) | 28_23_23 | 276 (revlib:hwb5_300.real) | **230** | 94 | exact (intended) |
| hwb5_53 | HiddenWeightedBit(over5variables) | 5_0_0 | 77 (maslov:hwb5-33-71.real) | **73** | 29 | exact |
| hwb6_301 | HiddenWeightedBit(over6variables) | 46_40_40 | 507 (revlib:hwb6_301.real) | **451** | 167 | exact (intended) |
| hwb6_56 | HiddenWeightedBit(over6variables) | 6_0_0 | 107 (maslov:hwb6_47_107.real) | **103** | 43 | exact |
| hwb7_302 | HiddenWeightedBit(over7variables) | 73_66_66 | 909 (revlib:hwb7_302.real) | **761** | 257 | exact (intended) |
| hwb7_59 | HiddenWeightedBit(over7variables) | 7_0_0 | 1847 (maslov:hwb07-n7-gc612-qc1737.real) | **1539** | 484 | exact |
| hwb8_113 | HiddenWeightedBit(over8variables) | 8_0_0 | 5136 (maslov:hwb08-n8-gc1604-qc4862.real) | **4451** | 1337 | exact |
| hwb8_303 | HiddenWeightedBit(over8variables) | 112_104_104 | 1461 (revlib:hwb8_303.real) | **1171** | 443 | exact (intended) |
| hwb9_119 | HiddenWeightedBit(over9variables) | 9_0_0 | 12707 (maslov:hwb09-n9-gc4014-qc12293.real) | **12047** | 3870 | exact |
| hwb9_304 | HiddenWeightedBit(over9variables) | 170_161_161 | 2275 (revlib:hwb9_304.real) | **1964** | 628 | exact (intended) |
| in0_235 | in0 | 26_11_15 | 20031 (revlib:in0_235.real) | **10334** | 328 | exact (intended) |
| in2_236 | in2 | 29_10_19 | 23802 (revlib:in2_236.real) | **10604** | 350 | exact (intended) |
| inc_237 | inc | 16_9_7 | 2140 (revlib:inc_237.real) | **1104** | 88 | exact (intended) |
| life_238 | life | 10_1_9 | 6766 (revlib:life_238.real) | **1840** | 79 | exact (intended) |
| lu_326 | LU | 299_233_203 | 1223 (revlib:lu_326.real) | **575** | 319 | exact_sat_intended |
| lu_327 | LU | 203_137_107 | 6562 (revlib:lu_327.real) | **3603** | 195 | exact_sat_intended |
| majority_239 | majority | 6_1_5 | 136 (revlib:majority_239.real) | **94** | 10 | exact (intended) |
| max46_240 | max46 | 10_1_9 | 5444 (revlib:max46_240.real) | **4159** | 167 | exact (intended) |
| mini_alu_305 | mini-alu | 10_6_8 | 60 (revlib:mini_alu_305.real) | **53** | 17 | exact (intended) |
| misex1_241 | misex1 | 15_7_8 | 982 (revlib:misex1_241.real) | **364** | 59 | exact (intended) |
| misex3_242 | misex3 | 28_14_14 | 119177 (revlib:misex3_242.real) | **90947** | 1738 | exact (intended) |
| misex3c_244 | misex3c | 28_14_14 | 115190 (revlib:misex3c_244.real) | **94021** | 1708 | exact (intended) |
| mlp4_245 | mlp4 | 16_8_8 | 3753 (revlib:mlp4_245.real) | **2218** | 174 | exact (intended) |
| mod5adder_306 | mod5adder | 32_26_26 | 292 (revlib:mod5adder_306.real) | **247** | 57 | exact (intended) |
| mult_stmts_16_344 | Multiplestatements | 128_32_32 | 6122 (revlib:mult_stmts_16_344.real) | **2930** | 754 | exact_sat_intended |
| mult_stmts_16_464 | Multiplestatements | 112_16_16 | 11572 (revlib:mult_stmts_16_464.real) | **6517** | 1817 | exact (SAT) |
| mult_stmts_32_345 | Multiplestatements | 256_64_64 | 25282 (revlib:mult_stmts_32_345.real) | **12362** | 3290 | exact_sat_intended |
| mult_stmts_32_393 | Multiplestatements | 224_32_32 | 49172 (revlib:mult_stmts_32_393.real) | **49157** | 7225 | exact (SAT) |
| mux_246 | mux | 22_1_21 | 1078 (revlib:mux_246.real) | **447** | 48 | exact (intended) |
| nestedif2_16_372 | Nestedcontrolstructure(larger) | 134_100_102 | 6034 (revlib:nestedif2_16_372.real) | **4799** | 239 | exact_sat_intended |
| nestedif2_16_396 | Nestedcontrolstructure(larger) | 37_3_5 | 8504 (revlib:nestedif2_16_396.real) | **6975** | 188 | exact_sat_intended |
| nestedif2_16_420 | Nestedcontrolstructure(larger) | 39_5_7 | 5243 (revlib:nestedif2_16_420.real) | **3967** | 185 | exact_sat_intended |
| nestedif2_16_468 | Nestedcontrolstructure(larger) | 38_4_6 | 5324 (revlib:nestedif2_16_468.real) | **3992** | 194 | exact_sat_intended |
| nestedif2_32_373 | Nestedcontrolstructure(larger) | 262_196_196 | 26674 (revlib:nestedif2_32_373.real) | **21752** | 484 | exact_sat_intended |
| nestedif2_32_397 | Nestedcontrolstructure(larger) | 69_3_3 | 31784 (revlib:nestedif2_32_397.real) | **26576** | 399 | exact_sat_intended |
| nestedif2_32_445 | Nestedcontrolstructure(larger) | 263_197_197 | 14424 (revlib:nestedif2_32_445.real) | **11644** | 492 | exact_sat_intended |
| nestedif2_32_469 | Nestedcontrolstructure(larger) | 70_4_4 | 17350 (revlib:nestedif2_32_469.real) | **14610** | 422 | exact_sat_intended |
| nestedif_16_370 | Nestedcontrolstructure | 133_99_101 | 1475 (revlib:nestedif_16_370.real) | **884** | 428 | exact_sat_intended |
| nestedif_16_394 | Nestedcontrolstructure | 36_2_4 | 6996 (revlib:nestedif_16_394.real) | **2659** | 423 | exact_sat_intended |
| nestedif_16_418 | Nestedcontrolstructure | 38_4_6 | 3094 (revlib:nestedif_16_418.real) | **1509** | 425 | exact_sat_intended |
| nestedif_16_466 | Nestedcontrolstructure | 37_3_5 | 3108 (revlib:nestedif_16_466.real) | **1518** | 426 | exact_sat_intended |
| nestedif_32_371 | Nestedcontrolstructure | 261_195_195 | 3011 (revlib:nestedif_32_371.real) | **1900** | 908 | exact_sat_intended |
| nestedif_32_395 | Nestedcontrolstructure | 68_2_2 | 14484 (revlib:nestedif_32_395.real) | **5755** | 875 | exact (SAT) |
| nestedif_32_419 | Nestedcontrolstructure | 70_4_4 | 6358 (revlib:nestedif_32_419.real) | **3221** | 881 | exact (SAT) |
| nestedif_32_467 | Nestedcontrolstructure | 69_3_3 | 6372 (revlib:nestedif_32_467.real) | **3239** | 879 | exact (SAT) |
| parity_247 | parity | 17_1_16 | 32 (revlib:parity_247.real) | **16** | 16 | exact (intended) |
| pcler8_248 | pcler8 | 21_5_16 | 327 (revlib:pcler8_248.real) | **303** | 30 | exact (intended) |
| pdc_307 | pdc | 619_603_579 | 6500 (revlib:pdc_307.real) | **6045** | 2053 | exact (intended) |
| plus127mod8192_162 | plus127mod8192 | 13_0_0 | 73357 (revlib:plus127mod8192_162.real) | **66177** | 995 | exact (intended) |
| plus127mod8192_308 | plus127mod8192 | 25_12_12 | 98 (revlib:plus127mod8192_308.real) | **80** | 36 | exact (intended) |
| plus63mod4096_163 | plus63mod4096 | 12_0_0 | 32539 (revlib:plus63mod4096_163.real) | **28161** | 463 | exact (intended) |
| plus63mod4096_309 | plus63mod4096 | 23_11_11 | 89 (revlib:plus63mod4096_309.real) | **72** | 32 | exact (intended) |
| plus63mod8192_164 | plus63mod8192 | 13_0_0 | 45025 (revlib:plus63mod8192_164.real) | **40106** | 521 | exact (intended) |
| plus63mod8192_310 | plus63mod8192 | 25_12_12 | 97 (revlib:plus63mod8192_310.real) | **78** | 34 | exact (intended) |
| pm1_249 | pm1 | 14_10_4 | 377 (revlib:pm1_249.real) | **188** | 36 | exact (intended) |
| radd_250 | radd | 13_5_8 | 676 (revlib:radd_250.real) | **161** | 22 | exact (intended) |
| rd32_271 | 1-bitAdder/rd32 | 5_2_3 | 18 (revlib:rd32_272.real) | **10** | 6 | exact (intended) |
| rd53_311 | rd53 | 13_8_10 | 98 (revlib:rd53_311.real) | **45** | 17 | exact (intended) |
| rd73_312 | rd73 | 25_18_22 | 217 (revlib:rd73_312.real) | **45** | 29 | exact (intended) |
| rd84_253 | rd84 | 12_4_8 | 2749 (revlib:rd84_253.real) | **358** | 49 | exact (intended) |
| rd84_313 | rd84 | 34_26_30 | 304 (revlib:rd84_313.real) | **115** | 42 | exact (intended) |
| root_255 | root | 13_5_8 | 3443 (revlib:root_255.real) | **1659** | 128 | exact (intended) |
| ryy6_256 | ryy6 | 17_1_16 | 4292 (revlib:ryy6_256.real) | **4291** | 43 | exact (intended) |
| sao2_257 | sao2 | 14_4_10 | 7670 (revlib:sao2_257.real) | **4100** | 87 | exact (intended) |
| seq_314 | seq | 1617_1576_1582 | 19362 (revlib:seq_314.real) | **18453** | 5741 | exact (SAT) |
| sf_274 | SampleFunction | 5_1_4 | 51 (revlib:sf_275.real) | **21** | 5 | exact (intended) |
| simple_321 | SimpleALU | 235_169_139 | 152852 (revlib:simple_321.real) | **148470** | 3817 | exact_sat_intended |
| simple_alu_16_338 | SimpleALU | 117_83_69 | 35463 (revlib:simple_alu_16_338.real) | **14660** | 896 | exact_sat_intended |
| simple_alu_16_362 | SimpleALU | 165_131_117 | 6275 (revlib:simple_alu_16_362.real) | **3271** | 943 | exact_sat_intended |
| simple_alu_16_386 | SimpleALU | 69_35_21 | 69810 (revlib:simple_alu_16_386.real) | **11093** | 2123 | exact (SAT) |
| simple_alu_16_410 | SimpleALU | 118_84_70 | 8975 (revlib:simple_alu_16_410.real) | **4257** | 932 | exact_sat_intended |
| simple_alu_320 | SimpleALU | 331_265_235 | 27009 (revlib:simple_alu_320.real) | **24504** | 3564 | exact_sat_intended |
| simple_alu_32_387 | SimpleALU | 133_67_37 | 287346 (revlib:simple_alu_32_387.real) | **287307** | 7813 | exact (SAT) |
| simple_alu_32_411 | SimpleALU | 230_164_134 | 30775 (revlib:simple_alu_32_411.real) | **27611** | 3644 | exact_sat_intended |
| simple_alu_32_435 | SimpleALU | 325_259_229 | 25531 (revlib:simple_alu_32_435.real) | **24519** | 3671 | exact_sat_intended |
| simple_alu_32_459 | SimpleALU | 134_68_38 | 60206 (revlib:simple_alu_32_459.real) | **60118** | 7934 | exact (SAT) |
| spla_315 | spla | 489_473_443 | 5925 (revlib:spla_315.real) | **5590** | 1646 | exact (intended) |
| sqn_258 | sqn | 10_3_7 | 2122 (revlib:sqn_258.real) | **328** | 38 | exact (intended) |
| sqr6_259 | sqr6 | 18_12_6 | 1033 (revlib:sqr6_259.real) | **657** | 79 | exact (intended) |
| sqrt8_260 | sqrt8 | 12_4_8 | 622 (revlib:sqrt8_260.real) | **507** | 48 | exact (intended) |
| squar5_261 | squar5 | 13_8_5 | 442 (revlib:squar5_261.real) | **248** | 42 | exact (intended) |
| sym10_262 | sym10 | 11_1_10 | 25866 (revlib:sym10_262.real) | **8077** | 146 | exact (intended) |
| sym6_145 | sym6 | 7_1_6 | 224 (maslov:symmetric_6-n7-gc41-qc206.real) | **90** | 15 | exact (intended) |
| sym6_316 | sym6 | 14_8_13 | 93 (revlib:sym6_316.real) | **76** | 15 | exact (intended) |
| sym9_147 | sym9 | 12_3_11 | 94 (revlib:sym9_147.real) | **89** | 21 | exact (intended) |
| sym9_148 | sym9 | 10_1_9 | 2077 (maslov:symmetric_9-n10-gc347-qc1975.real) | **176** | 26 | exact (intended) |
| sym9_317 | sym9 | 27_18_26 | 206 (revlib:sym9_317.real) | **176** | 26 | exact (intended) |
| table3_264 | table3 | 28_14_14 | 80039 (revlib:table3_264.real) | **31720** | 1016 | exact (intended) |
| tial_265 | tial | 22_8_14 | 56203 (revlib:tial_265.real) | **41760** | 1096 | exact (intended) |
| uncallif_32_343 | Hierarchicalcircuit | 66_1_1 | 3154 (revlib:uncallif_32_343.real) | **1666** | 434 | exact (SAT) |
| urf1_278 | UnstructuredReversibleFunction1 | 9_0_0 | 11646 (lee2021:urf1_278) | **10979** | 2523 | exact |
| urf2_277 | UnstructuredReversibleFunction2 | 8_0_0 | 4538 (lee2021:urf2_277) | **4189** | 1048 | exact |
| urf3_279 | UnstructuredReversibleFunction3 | 10_0_0 | 30065 (lee2021:urf3_279) | **28671** | 5864 | exact |
| urf5_280 | UnstructuredReversibleFunction5 | 9_0_0 | 8741 (lee2021:urf5_280) | **8207** | 1831 | exact |
| varops_16_350 | Variousoperations | 112_64_96 | 1361 (revlib:varops_16_350.real) | **96** | 44 | exact_sat_intended |
| varops_16_470 | Variousoperations | 96_48_80 | 2032 (revlib:varops_16_470.real) | **1715** | 815 | exact (SAT) |
| varops_32_351 | Variousoperations | 224_128_192 | 2801 (revlib:varops_32_351.real) | **209** | 93 | exact_sat_intended |
| varops_32_471 | Variousoperations | 192_96_160 | 4176 (revlib:varops_32_471.real) | **3645** | 1721 | exact (SAT) |
| wim_266 | wim | 11_7_4 | 217 (revlib:wim_266.real) | **136** | 27 | exact (intended) |
| x2_267 | x2 | 17_7_10 | 625 (revlib:x2_267.real) | **463** | 43 | exact (intended) |
| xor5_254 | xor5 | 6_1_5 | 7 (revlib:xor5_254.real) | **5** | 5 | exact (intended) |
| z4_268 | z4 | 11_4_7 | 642 (revlib:z4_268.real) | **81** | 20 | exact (intended) |
| z4ml_269 | z4ml | 11_4_7 | 642 (revlib:z4ml_269.real) | **81** | 20 | exact (intended) |

## Sampled (not claimed until proven)

| RevLib file | function | lines_const_garbage | best published (source) | ours qc | ours gates | check |
|---|---|---|---|---|---|---|
| alu_319 | ALU | 235_169_139 | 1851487 (revlib:alu_319.real) | **1366569** | 11480 | sampled |
| alu_32_335 | ALU | 229_163_133 | 1704912 (revlib:alu_32_335.real) | **1216256** | 10310 | sampled |
| alu_32_383 | ALU | 133_67_37 | 3407588 (revlib:alu_32_383.real) | **2459417** | 21767 | sampled |
| alu_flat_32_385 | FlattenedALU | 131_65_35 | 2760420 (revlib:alu_flat_32_385.real) | **1943659** | 21683 | sampled |

## Not claimed (a lower figure is published where we have not collected the circuit)

| RevLib file | function | lines_const_garbage | best published (source) | ours qc | ours gates | check |
|---|---|---|---|---|---|---|
| mini-alu_167 | mini-alu | 4_0_0 | 62 (revlib:mini-alu_167.real) | **26** | 10 | exact |
| mod10_176 | Modulo10Counter | 4_0_0 | 43 (revlib:mod10_176.real) | **25** | 9 | exact |
| urf6_281 | UnstructuredReversibleFunction6 | 15_0_0 | 17046 (revlib:urf6_281.real) | **16793** | 4720 | exact |

- mini-alu and mod10 at 4 lines: Szyprowski and Kerntopf (arXiv:1105.5831) list 16 and 21 with Peres gates. Our exact
  search proves 16 optimal for mini-alu in MCT+P: their circuit is optimal there; we tie, we do not beat it.
- urf4, urf6: arXiv:1004.1697 (Table 3) lists 90,696 and 16,687.

A record here is a record against the sources above, not against papers whose circuits we have not downloaded.
`cc-by-nc-sa/` holds circuits derived from Lee et al.'s (CC BY-NC-SA 4.0, see its NOTICE).
