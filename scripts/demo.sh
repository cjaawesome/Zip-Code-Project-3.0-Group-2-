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

TEST_CSV_PT2="$DATA_DIR/PT2_Randomized.csv"
BLOCK_PT2="$DATA_DIR/PT2_Randomized.zcb"

LOGFILE="$SCRIPTS_DIR/dump_output.txt"

mkdir -p "$OUT_DIR" "$SCRIPTS_DIR"

# Log everything to console + file
exec > >(tee "$LOGFILE") 2>&1

echo "=================================================================="
echo "BUILDING BINARIES"
echo "=================================================================="

# Main application (uses ZCDUtility main)
$CXX $CXXFLAGS src/*.cpp Utilities/ZipSearchApp.cpp Utilities/ZCDUtility.cpp -o "$BIN_ZIPSEARCH"

# Instructor test drivers (each has its own main; DO NOT link ZCDUtility here)
$CXX $CXXFLAGS src/*.cpp Utilities/AddTest.cpp    -o "$BIN_ADD"
$CXX $CXXFLAGS src/*.cpp Utilities/RemoveTest.cpp -o "$BIN_REMOVE"

echo
echo "Binaries built:"
ls -1 "$BIN_ZIPSEARCH" "$BIN_ADD" "$BIN_REMOVE"
echo

# ---------------------------------------------------------------------
# STEP 1 – PREPARE BLOCKED SEQUENCE SET FOR PT2_Randomized
# ---------------------------------------------------------------------
echo "=================================================================="
echo "STEP 1: BUILD BLOCKED SEQUENCE SET (PT2_Randomized)"
echo "=================================================================="

if [[ ! -f "$TEST_CSV_PT2" ]]; then
  echo "ERROR: $TEST_CSV_PT2 not found. Put PT2_Randomized.csv in ./data."
  exit 1
fi

echo "Using CSV: $TEST_CSV_PT2"
echo "Target blocked file: $BLOCK_PT2"

# Use a block size that holds ~6 records (tune if needed)
BLOCK_SIZE_SIX=512
MIN_BLOCK_SIZE_SIX=256

echo
echo "== 1A: Convert CSV to blocked sequence set with small block size =="
"$BIN_ZIPSEARCH" convert-blocked "$TEST_CSV_PT2" "$BLOCK_PT2" "$BLOCK_SIZE_SIX" "$MIN_BLOCK_SIZE_SIX"

echo
echo "== 1B: Show header for blocked file =="
"$BIN_ZIPSEARCH" header "$BLOCK_PT2"

# ---------------------------------------------------------------------
# STEP 2 – ADD RECORDS (NO SPLIT, THEN SPLIT + AVAIL LIST)
# ---------------------------------------------------------------------
echo
echo "=================================================================="
echo "STEP 2: ADDING RECORDS – NO SPLIT, THEN SPLIT + AVAIL LIST"
echo "=================================================================="

echo "Running AddTest on data/PT2_Randomized.zcb"
echo "(AddTest is instructor-provided and hard-coded to that path.)"
echo

"./AddTest"

# ---------------------------------------------------------------------
# STEP 3 – DELETE RECORDS (NO REDIS, REDIS, MERGE)
# ---------------------------------------------------------------------
echo
echo "=================================================================="
echo "STEP 3: DELETING RECORDS – NO REDIS, REDIS, MERGE"
echo "=================================================================="

echo "Running RemoveTest on data/PT2_Randomized.zcb"
echo "(RemoveTest is instructor-provided and uses its own delete scenarios.)"
echo

"./RemoveTest"

echo
echo "=================================================================="
echo "DEMO COMPLETE"
echo "Output saved to: $LOGFILE"
echo "=================================================================="
