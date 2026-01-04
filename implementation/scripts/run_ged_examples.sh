#!/usr/bin/env zsh
set -euo pipefail

# Runs all files in examples/ged/*.txt with both:
#   - ./bin/main.o ged exact <file>
#   - ./bin/main.o ged approx <file> <K>
# and prints a compact summary including wall time.
#
# Usage:
#   zsh scripts/run_ged_examples.sh            # K=20
#   K=50 zsh scripts/run_ged_examples.sh       # override K
#   FLAGS="v p" zsh scripts/run_ged_examples.sh  # pass flags (v/p)

K="${K:-1}"
FLAGS_STR="${FLAGS:-}"

if [[ ! -x ./bin/main.o ]]; then
  echo "error: ./bin/main.o not found or not executable; run 'make' first" >&2
  exit 1
fi

if ! command -v /usr/bin/time >/dev/null 2>&1; then
  echo "error: /usr/bin/time not found; install time or adjust script" >&2
  exit 1
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

echo "# GED examples runner"
echo "# K=${K} FLAGS='${FLAGS_STR}'"
echo

print_summary() {
  local label="$1"
  local outFile="$2"
  local timeFile="$3"

  local gedLine completeLine real user sys
  gedLine="$(grep -m1 '^GED: ' "$outFile" || true)"
  completeLine="$(grep -m1 '^Exact: ' "$outFile" || true)"

  real="$(awk '/^real /{print $2}' "$timeFile" 2>/dev/null || true)"
  user="$(awk '/^user /{print $2}' "$timeFile" 2>/dev/null || true)"
  sys="$(awk '/^sys /{print $2}' "$timeFile" 2>/dev/null || true)"

  echo "  ${label}:"
  [[ -n "$gedLine" ]] && echo "    ${gedLine}" || echo "    GED: (missing)"
  [[ -n "$completeLine" ]] && echo "    ${completeLine}"
  echo "    time: real=${real:-?}s user=${user:-?}s sys=${sys:-?}s"
}

examples=()
while IFS= read -r f; do
  examples+=("$f")
done < <(find ./examples/ged -maxdepth 1 -type f -name '*.txt' | LC_ALL=C sort)

if (( ${#examples[@]} == 0 )); then
  echo "error: no examples found under ./examples/ged/*.txt" >&2
  exit 1
fi

for f in "${examples[@]}"; do
  echo "== $(basename "$f") =="

  outExact="$tmpdir/out_exact.txt"
  tExact="$tmpdir/time_exact.txt"
  : >"$outExact"; : >"$tExact"

  /usr/bin/time -p -o "$tExact" ./bin/main.o ged exact "$f" ${=FLAGS_STR} >"$outExact" 2>&1 || true
  print_summary "exact" "$outExact" "$tExact"

  outApprox="$tmpdir/out_approx.txt"
  tApprox="$tmpdir/time_approx.txt"
  : >"$outApprox"; : >"$tApprox"

  /usr/bin/time -p -o "$tApprox" ./bin/main.o ged approx "$f" "$K" ${=FLAGS_STR} >"$outApprox" 2>&1 || true
  print_summary "approx" "$outApprox" "$tApprox"

  echo

done
