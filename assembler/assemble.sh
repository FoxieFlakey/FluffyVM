#!/bin/bash
ROOT="$(dirname $(realpath "$0"))"
exec "$ROOT/assembler/run.sh" "$ROOT/start.lua" "$ROOT/bytecode.json"

