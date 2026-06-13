#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

rm -rf build dist wheelhouse ./*.egg-info

uv run --with pyodide-build pyodide build .
