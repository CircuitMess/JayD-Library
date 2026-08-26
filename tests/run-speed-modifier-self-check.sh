#!/bin/sh
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary="${TMPDIR:-/tmp}/jayd-speed-modifier-self-check"
trap 'rm -f "$binary"' EXIT

c++ -std=c++11 -Wall -Wextra -Werror \
	-I"$repo/tests/stubs" -I"$repo/src" \
	"$repo/tests/SpeedModifierSelfCheck.cpp" \
	"$repo/src/AudioLib/SpeedModifier.cpp" \
	"$repo/src/AudioLib/Source.cpp" \
	-o "$binary"
"$binary"
