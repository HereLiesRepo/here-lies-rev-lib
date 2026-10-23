#!/bin/sh
# test_cost.sh: revverify's quantum-cost arithmetic at its edges (REVLIB-COST-OVERFLOW-20261022). Each case is a one- or
#   two-gate circuit with an expectation computed here by bc, independently of revverify's formulas:
#     c = 0, 1, 2 controls (the table), c = 10 and 62 with no empty line (2^(c+1) - 3), c = 62 with one empty line
#     (24 (c+1) - 88), c = 63 with none (2^64 - 3, the largest that fits), c = 64 with none (2^65 - 3: must be REFUSED),
#     a Fredkin whose MCT part is 2^64 - 3 (+2 = 2^64 - 1, fits), a Fredkin one control wider (REFUSED), and two
#     c = 63 gates whose sum passes 2^64 - 1 (REFUSED). A refusal must also exit nonzero.
#   Built with gcc and clang (when present), each with -fsanitize=undefined -fno-sanitize-recover: any undefined shift
#   or overflow fails the build's run. Exit 0 only when every case holds on every build.
set -u
d=$(mktemp -d); trap 'rm -rf "$d"' EXIT
src="$(dirname "$0")/revverify.c"
mk() {   # mk FILE NLINES GATE...   one .real with NLINES lines l0.. and the given gate lines
  f=$1; n=$2; shift 2; vs=""; i=0; while [ $i -lt $n ]; do vs="$vs l$i"; i=$((i+1)); done
  dash=$(printf '%*s' "$n" '' | tr ' ' '-')
  { echo ".version 1.0"; echo ".numvars $n"; echo ".variables$vs"; echo ".inputs$vs"; echo ".outputs$vs"
    echo ".constants $dash"; echo ".garbage $dash"; echo ".begin"; for g in "$@"; do echo "$g"; done; echo ".end"; } > "$f"
}
ops() { i=$1; o=""; while [ $i -lt $2 ]; do o="$o l$i"; i=$((i+1)); done; echo "$o"; }   # ops FROM TO: l_FROM .. l_(TO-1)
mk "$d/c0.real" 1 "t1$(ops 0 1)";           e_c0=1
mk "$d/c1.real" 2 "t2$(ops 0 2)";           e_c1=1
mk "$d/c2.real" 3 "t3$(ops 0 3)";           e_c2=5
mk "$d/c10.real" 11 "t11$(ops 0 11)";       e_c10=$(echo '2^11-3' | bc)
mk "$d/c62.real" 63 "t63$(ops 0 63)";       e_c62=$(echo '2^63-3' | bc)
mk "$d/c62e1.real" 64 "t63$(ops 0 63)";     e_c62e1=$(echo '24*63-88' | bc)
mk "$d/c63.real" 64 "t64$(ops 0 64)";       e_c63=$(echo '2^64-3' | bc)
mk "$d/c64.real" 65 "t65$(ops 0 65)";       e_c64=REFUSED
mk "$d/f62.real" 64 "f64$(ops 0 64)";       e_f62=$(echo '2^64-3+2' | bc)
mk "$d/f63.real" 65 "f65$(ops 0 65)";       e_f63=REFUSED
mk "$d/sum.real" 64 "t64$(ops 0 64)" "t64$(ops 0 64)"; e_sum=REFUSED
fails=0; builds=0
for cc in gcc clang; do
  command -v $cc > /dev/null 2>&1 || { echo "  NOT RUN  $cc not installed"; continue; }
  if ! $cc -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -o "$d/rv_$cc" "$src" 2> "$d/build.err"; then
    if grep -q "ubsan" "$d/build.err" && $cc -std=c11 -O2 -Wall -Wextra -Werror -o "$d/rv_$cc" "$src"; then echo "  NOT RUN  $cc UBSan (its runtime library is not installed); $cc built without the sanitizer"
    else echo "  FAIL $cc build: $(head -2 "$d/build.err" | tr '\n' ' ')"; fails=$((fails+1)); continue; fi
  fi
  builds=$((builds+1))
  for c in c0 c1 c2 c10 c62 c62e1 c63 c64 f62 f63 sum; do
    out=$("$d/rv_$cc" stat "$d/$c.real" 2>&1); rc=$?
    got=$(echo "$out" | sed -n 's/.* qc=\([^ ]*\).*/\1/p'); case "$got" in REFUSED*) got=REFUSED;; esac
    eval want=\$e_$c
    ok=0; if [ "$got" = "$want" ]; then if [ "$want" = REFUSED ]; then [ $rc != 0 ] && ok=1; else [ $rc = 0 ] && ok=1; fi; fi
    if [ $ok = 1 ]; then echo "  ok   $cc $c qc=$got (rc $rc)"
    else echo "  FAIL $cc $c want $want got '${got:-none}' rc $rc: $(echo "$out" | head -2 | tr '\n' ' ')"; fails=$((fails+1)); fi
  done
done
[ $builds -gt 0 ] || { echo "test_cost: no compiler ran"; exit 1; }
[ $fails = 0 ] && echo "test_cost: every case holds on $builds build(s)" || echo "test_cost: $fails FAILED"
[ $fails = 0 ]
