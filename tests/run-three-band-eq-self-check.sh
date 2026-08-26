#!/bin/sh
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary="${TMPDIR:-/tmp}/jayd-three-band-eq-self-check"
trap 'rm -f "$binary"' EXIT

c++ -std=c++11 -Wall -Wextra -Werror \
	-fsanitize=address,undefined -fno-omit-frame-pointer \
	-I"$repo/tests/stubs" -I"$repo/src" \
	"$repo/tests/ThreeBandEQSelfCheck.cpp" \
	"$repo/src/AudioLib/Effects/ThreeBandEQ.cpp" \
	-o "$binary"
"$binary"
