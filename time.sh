#!/bin/bash
#
# Default build: Benchy = 30s
#
set -eu

input_file=$1
output_file=${input_file%.scad}.stl

common_feature_flags=(
  --enable=lazy-union
)
parallel_feature_flags=(
  "${common_feature_flags[@]}"
   --enable=parallelize
   --enable=flatten-children
   --enable=lazy-module
   --enable=push-transforms
   --enable=difference-union
)

echo "Local Dev"
time ./OpenSCAD.app/Contents/MacOS/OpenSCAD \
   "${parallel_feature_flags[@]}" \
   "$input_file" -o "${input_file%.scad}_dev.stl"

echo "Installed 2021.01.16"
time /Applications/OpenSCAD-2021.01.16.app/Contents/MacOS/OpenSCAD \
   "${common_feature_flags[@]}" \
   "$input_file" -o "${input_file%.scad}_installed.stl"
