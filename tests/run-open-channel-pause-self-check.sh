#!/bin/sh
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary="${TMPDIR:-/tmp}/jayd-open-channel-pause-self-check"
trap 'rm -f "$binary"' EXIT

c++ -std=c++11 -Wall -Wextra -Werror \
	-fsanitize=address,undefined -fno-omit-frame-pointer \
	-I"$repo/tests/stubs" -I"$repo/src" \
	"$repo/tests/OpenChannelPauseSelfCheck.cpp" \
	-o "$binary"
(cd "$repo" && "$binary")
