#!/usr/bin/env bash
set -euo pipefail

CXX=g++
CXXFLAGS="-std=c++11 -I src"

BIN_ZIPSEARCH=./ZipSearch
BIN_ADD=./AddTest
BIN_REMOVE=./RemoveTest

DATA_DIR=./data
OUT_DIR=./out
SCRIPTS_DIR=./scripts

# Large randomized test file
TEST_CSV_PT2="$DATA_DIR/PT2_Randomized.csv"
BLOCK_PT2_SIX="$DATA_DIR/PT2_Randomized.zcb"         # ~6-record block size version

# Small “minimum useful size” file (you likely already have data/small.csv)
TEST_CSV_SMALL="$DATA_DIR/small.csv"
BLOCK_SMALL_DEFAULT="$OUT_DIR/small_default.zcb"     # default block size
BLOCK_SMALL_SIX="$OUT_DIR/small_sixrec.zcb"          # ~6-record block size

# If small.csv doesn't exist, auto-generate a small test file
# from the first few records of PT2_Randomized.csv
if [[ ! -f "$TEST_CSV_SMALL" ]]; then
  if [[ -f "$TEST_CSV_PT2" ]]; then
    echo "small.csv not found – generating a small test file from PT2_Randomized.csv"
    # Keep header + first 10 data rows (adjust '10' if you want more/less)
    head -n 11 "$TEST_CSV_PT2" > "$TEST_CSV_SMALL"
  else
    echo "WARNING: Cannot generate small.csv – $TEST_CSV_PT2 not found."
  fi
fi

LOGFILE="$SCRIPTS_DIR/dump_output.txt"

mkdir -p "$OUT_DIR" "$SCRIPTS_DIR"

# Log everything to dump_output.txt
exec > >(tee "$LOGFILE") 2>&1

echo "=================================================================="
echo "BUILDING BINARIES"
echo "=================================================================="

# Main ZipSearch binary
$CXX $CXXFLAGS src/*.cpp Utilities/ZipSearchApp.cpp Utilities/ZCDUtility.cpp -o "$BIN_ZIPSEARCH"

# Test binaries
$CXX $CXXFLAGS src/*.cpp Utilities/AddTest.cpp    -o "$BIN_ADD"
$CXX $CXXFLAGS src/*.cpp Utilities/RemoveTest.cpp -o "$BIN_REMOVE"

echo
echo "Binaries built:"
ls -1 "$BIN_ZIPSEARCH" "$BIN_ADD" "$BIN_REMOVE"
echo

# ---------------------------------------------------------------------
# STEP 1 – BUILD BLOCKED SEQUENCE SETS (SMALL+DEFAULT, SMALL+SIX, PT2+SIX)
# ---------------------------------------------------------------------
echo "=================================================================="
echo "STEP 1: BUILD BLOCKED SEQUENCE SETS (DEFAULT & ~SIX-RECORD BLOCKS)"
echo "=================================================================="

# -----------------------------
# 1A. Small file, default block size
# -----------------------------
if [[ -f "$TEST_CSV_SMALL" ]]; then
  echo
  echo "== 1A: Convert SMALL CSV with DEFAULT block size =="
  echo "Data file    : $TEST_CSV_SMALL"
  echo "Blocked file : $BLOCK_SMALL_DEFAULT"

  # Default block size parameters
  DEFAULT_BLOCK_SIZE=4096
  DEFAULT_MIN_BLOCK_SIZE=2048

  "$BIN_ZIPSEARCH" convert-blocked \
      "$TEST_CSV_SMALL" "$BLOCK_SMALL_DEFAULT" \
      "$DEFAULT_BLOCK_SIZE" "$DEFAULT_MIN_BLOCK_SIZE"

  echo
  echo "-- Header (small, default block size) --"
  "$BIN_ZIPSEARCH" header "$BLOCK_SMALL_DEFAULT"
else
  echo "WARNING: $TEST_CSV_SMALL not found – skipping small+default demo."
fi

# -----------------------------
# 1B. Small file, ~six-record block size
# -----------------------------
if [[ -f "$TEST_CSV_SMALL" ]]; then
  echo
  echo "== 1B: Convert SMALL CSV with ~SIX-RECORD block size =="
  echo "Data file    : $TEST_CSV_SMALL"
  echo "Blocked file : $BLOCK_SMALL_SIX"

  # Block size tuned so a block holds roughly 6 records
  BLOCK_SIZE_SIX=512
  MIN_BLOCK_SIZE_SIX=256

  "$BIN_ZIPSEARCH" convert-blocked \
      "$TEST_CSV_SMALL" "$BLOCK_SMALL_SIX" \
      "$BLOCK_SIZE_SIX" "$MIN_BLOCK_SIZE_SIX"

  echo
  echo "-- Header (small, ~six-record block size) --"
  "$BIN_ZIPSEARCH" header "$BLOCK_SMALL_SIX"
fi

# -----------------------------
# 1C. PT2_Randomized with ~six-record block size (instructor tests)
# -----------------------------
echo
echo "== 1C: Convert PT2_Randomized CSV with ~SIX-RECORD block size =="
if [[ ! -f "$TEST_CSV_PT2" ]]; then
  echo "ERROR: $TEST_CSV_PT2 not found. Put PT2_Randomized.csv in $DATA_DIR."
  exit 1
fi

echo "Using CSV: $TEST_CSV_PT2"
echo "Target blocked file: $BLOCK_PT2_SIX"

BLOCK_SIZE_SIX=512
MIN_BLOCK_SIZE_SIX=256

"$BIN_ZIPSEARCH" convert-blocked \
    "$TEST_CSV_PT2" "$BLOCK_PT2_SIX" \
    "$BLOCK_SIZE_SIX" "$MIN_BLOCK_SIZE_SIX"

echo
echo "-- Header (PT2_Randomized, ~six-record block size) --"
"$BIN_ZIPSEARCH" header "$BLOCK_PT2_SIX"

# ---------------------------------------------------------------------
# STEP 2 – ADD RECORDS (NO SPLIT, THEN SPLIT + AVAIL LIST)
# ---------------------------------------------------------------------
echo
echo "=================================================================="
echo "STEP 2: ADDING RECORDS – NO SPLIT, THEN SPLIT + AVAIL LIST"
echo "=================================================================="

echo "Running AddTest on data/PT2_Randomized.zcb"
echo

"./AddTest"

# ---------------------------------------------------------------------
# STEP 3 – DELETE RECORDS (NO REDIS, REDIS, MERGE)
# ---------------------------------------------------------------------
echo
echo "=================================================================="
echo "STEP 3: DELETING RECORDS – NO REDISTRIBUTION, REDISTRIBUTION, MERGE"
echo "=================================================================="

echo "Running RemoveTest on data/PT2_Randomized.zcb"
echo "(RemoveTest with 3 subtests:"
echo "  - Test 1: Delete with NO redistribution or merge"
echo "  - Test 2: Delete that triggers REDISTRIBUTION ONLY"
echo "  - Test 3: Delete that triggers a MERGE, with logically rightmost block"
echo "            cleared and added to the avail list.)"
echo

"./RemoveTest"

echo
echo "=================================================================="
echo "DEMO COMPLETE"
echo "Output saved to: $LOGFILE"
echo "=================================================================="
