#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CACHE_DIR="${LUAU_CACHE_DIR:-$ROOT/.build/luau-upstream}"
LUAU_REF="${LUAU_REF:-master}"
VENDOR_DIR="$ROOT/vendor/luau"
TMP_DIR="$ROOT/.build/luau-vendor"

mkdir -p "$ROOT/.build"

if [ ! -d "$CACHE_DIR/.git" ]; then
    git clone --filter=blob:none https://github.com/luau-lang/luau.git "$CACHE_DIR"
fi

git -C "$CACHE_DIR" fetch --depth 1 origin "$LUAU_REF"
COMMIT="$(git -C "$CACHE_DIR" rev-parse FETCH_HEAD)"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

git -C "$CACHE_DIR" archive FETCH_HEAD Common Ast Bytecode Compiler VM LICENSE.txt | tar -x -C "$TMP_DIR"

rm -rf "$VENDOR_DIR"
mkdir -p "$VENDOR_DIR"
rsync -a "$TMP_DIR"/ "$VENDOR_DIR"/

{
    printf 'Vendored from luau-lang/luau.\n\n'
    printf 'Repository: https://github.com/luau-lang/luau\n'
    printf 'Commit: %s\n\n' "$COMMIT"
    printf 'Only the source needed by the Python extension is included:\n'
    printf 'Common, Ast, Bytecode, Compiler, VM, and LICENSE.txt.\n'
} > "$VENDOR_DIR/UPSTREAM.txt"

printf 'Vendored Luau %s into %s\n' "$COMMIT" "$VENDOR_DIR"
