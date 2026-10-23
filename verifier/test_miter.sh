#!/bin/sh
# test_miter.sh SAT_SOLVER [DRAT_TRIM]: controls for revverify's independent miter encoder.
#   equal      t3 a b c and its two-gate equivalent (t3 a b c; t2 a d; t2 a d) is UNSAT, and drat-trim checks the proof
#   planted    a dropped control (t2 b c instead of t3 a b c) is SAT
#   peres      p a b c against t3 a b c; t2 a b (its definition) is UNSAT
#   mutant     REVVERIFY_MUTANT=1 drops one XOR clause: the equal pair turns SAT, a disagreement the dual check reports
#   vgates     t3 a b c and its V/V+/CNOT decomposition are UNSAT; one V and V+ swapped is SAT; a CNOT controlled by a
#              line left in a V state (a superposition) is refused as SAT, never vouched for
# Exit 0 only when every control holds.
set -u
S=${1:?usage: test_miter.sh SAT_SOLVER [DRAT_TRIM]}; D=${2:-}
d=$(mktemp -d); trap 'rm -rf "$d"' EXIT
cc -O2 -o "$d/rv" "$(dirname "$0")/revverify.c" || exit 1
hdr='.version 1.0\n.numvars 4\n.variables a b c d\n.inputs a b c d\n.outputs a b c d\n.constants ----\n.garbage ----\n.begin\n'
printf "${hdr}t3 a b c\n.end\n" > "$d/a.real"
printf "${hdr}t3 a b c\nt2 a d\nt2 a d\n.end\n" > "$d/b.real"
printf "${hdr}t2 b c\n.end\n" > "$d/bad.real"
printf "${hdr}p a b c\n.end\n" > "$d/p.real"
printf "${hdr}t3 a b c\nt2 a b\n.end\n" > "$d/pdef.real"
printf "${hdr}v b c\nt2 a b\nv+ b c\nt2 a b\nv a c\n.end\n" > "$d/vn.real"
printf "${hdr}v b c\nt2 a b\nv b c\nt2 a b\nv a c\n.end\n" > "$d/vb.real"
printf "${hdr}v b c\nt2 c a\nv+ b c\n.end\n" > "$d/vq.real"
printf "${hdr}t2 c a\n.end\n" > "$d/vqr.real"
fails=0
run() { "$d/rv" miter "$1" "$2" "$d/m.cnf" > /dev/null || { echo "  FAIL encoder refused $3"; fails=$((fails+1)); return 9; }; "$S" -q "$d/m.cnf" "$d/m.drat" > /dev/null 2>&1; }
run "$d/a.real" "$d/b.real" equal; r=$?
if [ $r = 20 ]; then
  if [ -n "$D" ]; then v=$("$D" "$d/m.cnf" "$d/m.drat" 2>&1 | tr -d '\r' | grep '^s ' | tail -1); [ "$v" = "s VERIFIED" ] && echo "  ok   equal pair UNSAT, proof checked by drat-trim" || { echo "  FAIL drat-trim: $v"; fails=$((fails+1)); }
  else echo "  ok   equal pair UNSAT (drat-trim NOT RUN)"; fi
else echo "  FAIL equal pair: rc $r"; fails=$((fails+1)); fi
run "$d/a.real" "$d/bad.real" planted; r=$?; [ $r = 10 ] && echo "  ok   planted dropped control is SAT" || { echo "  FAIL planted: rc $r"; fails=$((fails+1)); }
run "$d/pdef.real" "$d/p.real" peres; r=$?; [ $r = 20 ] && echo "  ok   Peres gate equals its definition (UNSAT)" || { echo "  FAIL peres: rc $r"; fails=$((fails+1)); }
run "$d/a.real" "$d/vn.real" vgates; r=$?; [ $r = 20 ] && echo "  ok   a Toffoli equals its V/V+/CNOT decomposition (UNSAT)" || { echo "  FAIL vgates equal: rc $r"; fails=$((fails+1)); }
run "$d/a.real" "$d/vb.real" vgates; r=$?; [ $r = 10 ] && echo "  ok   one V and V+ swapped is SAT" || { echo "  FAIL vgates swapped: rc $r"; fails=$((fails+1)); }
run "$d/vqr.real" "$d/vq.real" vgates; r=$?; [ $r = 10 ] && echo "  ok   a control left in a V state is refused (SAT)" || { echo "  FAIL vgates superposed control: rc $r"; fails=$((fails+1)); }
REVVERIFY_MUTANT=1 run "$d/a.real" "$d/b.real" mutant; r=$?; [ $r = 10 ] && echo "  ok   mutant encoder (one XOR clause dropped) calls the equal pair SAT: the disagreement shows" || { echo "  FAIL mutant not visible: rc $r"; fails=$((fails+1)); }
# intended-g: a real output named g, .garbage marks nothing: --intended must compare it (old rule made every g garbage)
gh='.version 1.0\n.numvars 2\n.variables a b\n.inputs a b\n.outputs a g\n.constants --\n.garbage --\n.begin\n'
printf "${gh}t2 a b\n.end\n" > "$d/g1.real"; printf "${gh}.end\n" > "$d/g0.real"
"$d/rv" --intended equiv "$d/g1.real" "$d/g0.real" 2>&1 | grep -q 'equivalent=NO' && echo "  ok   --intended: a real output named g is compared (wrong circuit answers NO)" || { echo "  FAIL intended-g: wrong circuit not rejected"; fails=$((fails+1)); }
[ $fails = 0 ] && { echo "PASS miter controls"; exit 0; }; echo "FAIL miter controls: $fails"; exit 1
