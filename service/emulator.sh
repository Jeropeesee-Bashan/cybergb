#!/bin/sh

if ! command -v socat >/dev/null
then
  echo "'socat' not found!" >&2
  exit 1
fi

if [ "$#" -lt 2 ]
then
  echo "USAGE: <pts-link-name> <visualizer-cmdline>" >&2
  exit 1
fi

exec sh -c "socat PTY,link=\"$1\",echo=0,raw EXEC:\"${*:2}\" 2>/dev/null || true"
