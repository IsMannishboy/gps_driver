#!/usr/bin/env bash
# Downloads the minmea NMEA-parsing library into gps_driver/Inc (headers)
# and gps_driver/Src (sources), matching the Inc/Src layout used by Core.
set -euo pipefail

BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INC_DIR="$BASE/Inc"
SRC_DIR="$BASE/Src"
REPO_RAW="https://raw.githubusercontent.com/kosma/minmea/master"

mkdir -p "$INC_DIR" "$SRC_DIR"

declare -A FILES=(
    [minmea.h]="$INC_DIR"
    [minmea.c]="$SRC_DIR"
)

for f in "${!FILES[@]}"; do
    dest="${FILES[$f]}/$f"
    if [ -f "$dest" ]; then
        echo "skip: $f already present"
        continue
    fi
    echo "fetching $f..."
    curl -fsSL "$REPO_RAW/$f" -o "$dest"
done

echo "minmea installed in $INC_DIR and $SRC_DIR"
