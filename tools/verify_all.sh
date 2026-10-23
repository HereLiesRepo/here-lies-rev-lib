#!/bin/sh
# verify_all.sh REVLIB_DIR [SAT_SOLVER] [OUT.tsv] [DRAT_TRIM]: re-check every claimed record with the public verifier alone.
#   REVLIB_DIR: a directory holding RevLib's .real files (records/RECORDS.tsv column 8 names the reference for each row).
#   SAT_SOLVER: any DIMACS solver that exits 20 on UNSAT and writes a DRAT proof to its second argument (kissat, cadical);
#   without one the SAT rows are skipped.
#   DRAT_TRIM: drat-trim. Given, a SAT row passes only as UNSAT_DRAT_VERIFIED, when drat-trim verifies the solver's proof
#   of the miter; without it a SAT row reports UNSAT_EQUAL, which is the solver's exit code and not a checked certificate.
#   Environment: VERIFY_JOBS (parallel checks, default 1), VERIFY_SAT_TIMEOUT (seconds per SAT proof, default none; a proof
#   that runs out is UNDECIDED, never passed).
# Each row is checked as labelled in column 9: exact -> equal on every input (strict garbage reading); exact_intended ->
# the same under the intended reading; exact_sat / exact_sat_intended -> the verifier's SAT miter under that reading is
# UNSAT. The record file's SHA-256 is printed beside the verdict, so the result names the exact bytes it judged. Rerunning
# with the same OUT keeps rows already decided for the same bytes and checks only the rest; a fresh result replaces any
# older row for the same file.
set -u
if [ "${1:-}" = --one ]; then   # internal: one row (V REF SAT TMP in the environment), run in parallel by the main mode
  shift; idx=$1; fn=$2; key=$3; file=$4; h=$5; chk=$6; ref=$7; s0=$(date +%s); iv=; case "$chk" in *intended) iv=--intended;; esac
  case "$chk" in
    exact|exact_intended) r=$($V $iv equiv "$REF/$ref" "$file" 2>&1 | tail -1); case "$r" in *=yes) v=EQUAL_EVERY_INPUT;; *) v="FAILED:$r";; esac ;;
    exact_sat|exact_sat_intended)
      if [ -z "$SAT" ]; then v=SKIPPED_NO_SOLVER
      else c=$TMP/m.$idx.cnf; p=; [ -n "$DRAT" ] && p=$TMP/m.$idx.drat
           $V $iv miter "$REF/$ref" "$file" $c > /dev/null 2>&1 && { if [ -n "${VERIFY_SAT_TIMEOUT:-}" ]; then timeout "$VERIFY_SAT_TIMEOUT" "$SAT" -q $c $p > /dev/null 2>&1; else "$SAT" -q $c $p > /dev/null 2>&1; fi; e=$?; } || e=-1
           case $e in 20) v=UNSAT_EQUAL
                 # with a proof checker, UNSAT counts only when drat-trim verifies the solver's proof; the exit code alone is not a certificate
                 if [ -n "$DRAT" ]; then d=$("$DRAT" $c $p 2>&1 | tr -d '\r' | grep '^s ' | tail -1); [ "$d" = "s VERIFIED" ] && v=UNSAT_DRAT_VERIFIED || v="DRAT_FAILED:${d:-no verdict}"; fi ;;
             10) v=SAT_DIFFERENT;; *) v="UNDECIDED:$e";; esac; rm -f $c $p; fi ;;
    *) v=SKIPPED_NOT_EXACT ;;
  esac
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$fn" "$key" "$file" "$h" "$chk" "$v" "$(( $(date +%s)-s0 ))" > $TMP/r.$idx
  exit 0
fi
REF=$1; SAT=${2:-}; OUT=${3:-/dev/stdout}; DRAT=${4:-}; cd "$(dirname "$0")/.."
V=/tmp/revverify.$$; cc -std=c11 -O2 -o $V verifier/revverify.c || exit 2
TMP=/tmp/verify_all.$$; mkdir -p $TMP
[ -f "$OUT" ] && [ "$OUT" != /dev/stdout ] && awk -v d="$DRAT" -F'\t' 'NR>1 && ($6=="EQUAL_EVERY_INPUT" || ($6=="UNSAT_EQUAL" && d=="") || $6=="UNSAT_DRAT_VERIFIED" || $6=="SAT_DIFFERENT" || $6 ~ /^FAILED/) {print $3 " " $4}' "$OUT" > $TMP/done || : > $TMP/done
# a kept row must still name the file's current bytes; an older version's row is dropped, never carried forward
if [ -f "$OUT" ] && [ "$OUT" != /dev/stdout ]; then
  awk -v d="$DRAT" -F'\t' 'NR>1 && ($6=="EQUAL_EVERY_INPUT" || ($6=="UNSAT_EQUAL" && d=="") || $6=="UNSAT_DRAT_VERIFIED" || $6=="SAT_DIFFERENT" || $6 ~ /^FAILED/)' "$OUT" | while IFS= read -r row; do
    f=$(printf '%s' "$row" | cut -f3); h=$(printf '%s' "$row" | cut -f4)
    [ -f "$f" ] && [ "$(sha256sum "$f" | cut -c1-64)" = "$h" ] && printf '%s\n' "$row"
  done > $TMP/kept
else : > $TMP/kept; fi
export V REF SAT DRAT TMP
tail -n +2 records/RECORDS.tsv | awk -F'\t' '$10=="yes" {print NR "\t" $0}' | while IFS="$(printf '\t')" read idx fn key pq psrc oq og file ref chk cl; do
  h=$(sha256sum "$file" | cut -c1-64); grep -qxF "$file $h" $TMP/done && continue   # by file AND bytes: identical copies of
  # one circuit can stand for different RevLib references, and each is checked against its own
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$idx" "$fn" "$key" "$file" "$h" "$chk" "$ref"
done > $TMP/todo
SELF="$(pwd)/tools/verify_all.sh"
while IFS="$(printf '\t')" read idx fn key file h chk ref; do printf '%s\0%s\0%s\0%s\0%s\0%s\0%s\0' "$idx" "$fn" "$key" "$file" "$h" "$chk" "$ref"; done < $TMP/todo |
  xargs -0 -n 7 -P "${VERIFY_JOBS:-1}" sh "$SELF" --one
{ printf 'function\tlines_constants_garbage\tfile\tsha256\tcheck\tverdict\tseconds\n'; cat $TMP/r.* 2>/dev/null; cat $TMP/kept; } | awk -F'\t' 'NR==1 || !seen[$3]++' > $TMP/out
[ "$OUT" = /dev/stdout ] && cat $TMP/out || { cp $TMP/out "$OUT"; awk -F'\t' 'NR>1{print $6}' "$OUT" | sed 's/:.*//' | sort | uniq -c; }
rm -rf $TMP $V
