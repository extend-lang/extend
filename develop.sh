#!/usr/bin/env sh
set -e

while true ; do

  first_failed=$(cmake --build . | awk -F ': ' '$1 == "FAILED" { print $2 }' | sort | head -n 1)

  if [[ "$first_failed" != "" ]] ; then
    build_result=$(ninja $first_failed 2>&1 || true)
  else
    build_result=$(./tests 2>&1 || true)
  fi

  if [[ "$last_less" != "" ]]; then
    kill $last_less
  fi
  clear
  echo "$build_result" | less -X &

  last_less=$!

  inotifywait -e modify -r ../../src ../../libs >/dev/null 2>&1
done
