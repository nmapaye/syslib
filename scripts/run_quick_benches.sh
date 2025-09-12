#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build_quick"
DOCS_DIR="$ROOT_DIR/docs/benchmarks"
mkdir -p "$BUILD_DIR" "$DOCS_DIR"

# Compile
echo "Compiling quick benches with clang++..."
clang++ -std=c++20 -O3 -march=native -I"$ROOT_DIR/include" \
  "$ROOT_DIR/scripts/bench_quick_spsc.cpp" -o "$BUILD_DIR/bench_quick_spsc"
clang++ -std=c++20 -O3 -march=native -I"$ROOT_DIR/include" \
  "$ROOT_DIR/scripts/bench_quick_mpmc.cpp" -o "$BUILD_DIR/bench_quick_mpmc"

# Run SPSC
echo "Running SPSC quick bench..."
SPSC_OUT="$DOCS_DIR/spsc_results.csv"
echo "ops_per_sec,ops,duration_s,capacity" > "$SPSC_OUT"
"$BUILD_DIR/bench_quick_spsc" >> "$SPSC_OUT"
cat "$SPSC_OUT"

# Run MPMC sweep
echo "Running MPMC quick bench..."
MPMC_OUT="$DOCS_DIR/mpmc_results.csv"
echo "producers,consumers,total_msgs,duration_s,throughput_ops_per_sec,p50_ns,p90_ns,p99_ns" > "$MPMC_OUT"
"$BUILD_DIR/bench_quick_mpmc" >> "$MPMC_OUT"
cat "$MPMC_OUT"

echo "Results saved under $DOCS_DIR"

