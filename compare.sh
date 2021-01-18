#!/bin/bash
#
# Build a clean binary with:
#   qmake openscad.pro CONFIG-=debug CONFIG+=experimental CONFIG+=snapshot CONFIG+=info && make -j12
#
# Then run with:
#   ./compare.sh Maze.scad
#   ./compare.sh Bricks.scad
#   ./compare.sh Benchy.scad -DN=5
#   ./compare.sh testdata/scad/3D/feature/module_recursion.scad
#     examples/Advanced/module_recursion.scad
#
set -eu

gdate 2>&1 >/dev/null || echo "Please install gdate with: brew install coreutils"

OPENSCAD_DEV="./OpenSCAD.app/Contents/MacOS/OpenSCAD"
OPENSCAD_SNAPSHOT="/Applications/OpenSCAD-2021.01.16.app/Contents/MacOS/OpenSCAD"
OPENSCAD_STABLE="/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD"
COMPARISONS_DIR="$PWD/comparisons"
STATS_CSV="$COMPARISONS_DIR/stats.csv"

mkdir -p "$COMPARISONS_DIR"

# -o BenchyDebug.stl --enable=parallelize --enable=lazy-union --enable=flatten-children Benchy.scad
function describe_features() {
  local lines=( $( ( for arg in "$@"  ; do echo "$arg" ; done ) | grep -e "--enable=" | sed 's/.*=//' | sort ) )
  IFS=" " ; echo "${lines[*]}"
}

function describe_params() {
  local lines=( $( ( for arg in "$@"  ; do echo "$arg" ; done ) | grep -e "-D" | sed 's/-D//' | sort ) )
  IFS=" " ; echo "${lines[*]}"
}

function add_stats() {
  local Timestamp=$( gdate '+%Y%m%d-%H%M' )
  if [[ ! -f "$STATS_CSV" ]]; then
    echo "File,Params,Features,Binary,ExecutionTime,Timestamp" > "$STATS_CSV"
  fi

  local File="$1"
  local Params="$2"
  local Features="$3"
  local Binary="$4"
  local ExecutionTime="$5"
  echo "$Timestamp,$File,$Params,$Features,$Binary,$ExecutionTime" >> "$STATS_CSV"
}

function export_stl() {
  local bin="$1"
  local file="$2"
  shift 2
  local bin_name
  if [[ "$bin" == "$OPENSCAD_DEV" ]]; then
    bin_name="DEV"
  elif [[ "$bin" == "$OPENSCAD_STABLE" ]]; then
    bin_name="STABLE"
  elif [[ "$bin" == "$OPENSCAD_SNAPSHOT" ]]; then
    bin_name="SNAPSHOT"
  else
    echo "Unexpected bin: $bin" >&2
    exit 1
  fi
  local features="$( describe_features "$@" )"
  local params="$( describe_params "$@" )"
  local output_base="$COMPARISONS_DIR/$( basename "${file%.scad}" )"
  if [[ -n "$params" ]]; then
    output_base="$output_base - ${params}"
  fi
  output_base="$output_base - ${features} - ${bin_name}"

  local output="${output_base}.stl"
  # local output="${output_base}.png"
  local log="${output_base}.log"

  echo "# Converting $(basename file) ($params / $features / $bin_name)..."
  local START=$( gdate +%s.%N )
  # sleep 1
  # time \
  "$bin" "$@" "$file" -o "$output" --render 2>&1 | tee "${log}"

  local END=$( gdate +%s.%N )
  local DIFF=$(echo "$END - $START" | bc)
  echo "# [${DIFF} sec] $output"
  echo ""

  add_stats "$file" "$params" "$features" "$bin_name" "$DIFF"
  # { time sleep 1 2> sleep.stderr ; } 2> time.txt
}

echo "Outputing stats in $STATS_CSV"
# export_stl "$OPENSCAD_DEV" "$@" --enable=lazy-union --enable=parallelize
# export_stl "$OPENSCAD_DEV" "$@" --enable=lazy-union --enable=parallelize --enable=flatten-children
# export_stl "$OPENSCAD_DEV" "$@" --enable=lazy-union --enable=parallelize --enable=flatten-children --enable=lazy-module
export_stl "$OPENSCAD_DEV" "$@" --enable=lazy-union --enable=parallelize --enable=flatten-children --enable=lazy-module --enable=difference-union
# export_stl "$OPENSCAD_DEV" "$@" --enable=lazy-union
export_stl "$OPENSCAD_SNAPSHOT" "$@" --enable=lazy-union
# export_stl "$OPENSCAD_STABLE" "$@"

echo "Stats are in $STATS_CSV"
# Beep!
printf "\a"
