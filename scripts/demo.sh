 #!/usr/bin/env bash
set -euo pipefail

# Build a tiny CSV and run the full pipeline.
# This version avoids invalid ZIPs and unnecessary repeats.

DATA_DIR="data"
OUT_DIR="out"
mkdir -p "$DATA_DIR" "$OUT_DIR"

CSV="$DATA_DIR/small.csv"
ZCB="$OUT_DIR/demo.zcb"

# Create a small, sorted dataset that forces splits with small block size.
cat > "$CSV" <<'EOF'
56300,Saint Cloud,MN,Stearns,45.540000,-94.180000
56301,Saint Cloud,MN,Stearns,45.541000,-94.181000
56302,Saint Cloud,MN,Stearns,45.542000,-94.182000
56303,Saint Cloud,MN,Stearns,45.543000,-94.183000
56304,Saint Cloud,MN,Stearns,45.544000,-94.184000
56305,Saint Cloud,MN,Stearns,45.545000,-94.185000
56306,Saint Cloud,MN,Stearns,45.546000,-94.186000
56307,Saint Cloud,MN,Stearns,45.547000,-94.187000
56308,Saint Cloud,MN,Stearns,45.548000,-94.188000
56309,Saint Cloud,MN,Stearns,45.549000,-94.189000
EOF

echo "== STEP 1: Convert CSV → blocked sequence set =="
./bss_build convert-blocked "$CSV" "$ZCB" 256 128

echo "== STEP 2: Dump physical order =="
./bss_build dump-physical "$ZCB"

echo "== STEP 3: Add a record (no split expected) =="
cat > "$OUT_DIR/add1.csv" <<'EOF'
56310,Saint Cloud,MN,Stearns,45.550000,-94.190000
EOF
./bss_build add "$ZCB" "$OUT_DIR/add1.csv"
./bss_build dump-logical "$ZCB"

echo "== STEP 4: Add three more (force block split) =="
cat > "$OUT_DIR/add2.csv" <<'EOF'
56311,Saint Cloud,MN,Stearns,45.551000,-94.191000
56312,Saint Cloud,MN,Stearns,45.552000,-94.192000
56313,Saint Cloud,MN,Stearns,45.553000,-94.193000
EOF
./bss_build add "$ZCB" "$OUT_DIR/add2.csv"
./bss_build dump-logical "$ZCB"

echo "== STEP 5: Delete a record (no merge expected) =="
echo "56310" > "$OUT_DIR/del1.txt"
./bss_build del "$ZCB" "$OUT_DIR/del1.txt"
./bss_build dump-logical "$ZCB"

echo "== STEP 6: Delete another (potential redistribution) =="
echo "56302" > "$OUT_DIR/del2.txt"
./bss_build del "$ZCB" "$OUT_DIR/del2.txt"
./bss_build dump-logical "$ZCB"

echo "== STEP 7: Delete last to trigger merge & avail use =="
echo "56313" > "$OUT_DIR/del3.txt"
./bss_build del "$ZCB" "$OUT_DIR/del3.txt"
./bss_build dump-physical "$ZCB"

echo "== DONE: Check logs above for SPLIT, MERGE, and INDEX UPDATE messages =="
