#!/usr/bin/env sh
set -eu

BIN="${1:-./minishell}"

if [ ! -x "$BIN" ]; then
  echo "smoke-test: binary is not executable: $BIN" >&2
  exit 1
fi

OUTPUT=$(printf 'echo hello\npwd\nexit\n' | "$BIN" 2>&1 || true)

printf '%s\n' "$OUTPUT" | grep -q 'hello'
printf '%s\n' "$OUTPUT" | grep -q '/'

echo "smoke-test: ok"
