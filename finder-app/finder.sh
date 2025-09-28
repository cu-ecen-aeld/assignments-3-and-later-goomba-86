#!/bin/bash

filesdir=${1}
searchstr=${2}

if [ $# -lt 2 ]; then
  echo "Not enough arguments"
  exit 1
fi

if [ ! -d "$filesdir" ]; then
  echo "Directory does not exist"
  exit 1
fi

matching_files=$(grep -rlI "$searchstr" "$filesdir" | wc -l)
matching_strings=$(grep -rI "$searchstr" "$filesdir" | wc -l)

echo "The number of files are "$matching_files" and the number of matching lines are "$matching_strings""
