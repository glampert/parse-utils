#!/bin/sh
#
# Builds each test program and runs it from its own directory. A test passes when
# it exits with status 0 - the programs check themselves with assert() - and, if
# it has expected output under tests/expected/, its stdout and stderr match that
# output once the values __DATE__ and __TIME__ expand to are masked.
#
# The two sample parsers have no expected output: they print the contents of an
# std::unordered_map, whose order differs between standard libraries.
#
# Usage: tests/run_tests.sh [--update]
#   --update  Rewrite the expected output from this run instead of comparing.
#
# CXX, CXXFLAGS and BUILD_DIR can be overridden from the environment.
#

set -u
cd "$(dirname "$0")/.." || exit 1

CXX=${CXX:-c++}
CXXFLAGS=${CXXFLAGS:--std=c++17 -O0 -g -Wall -Wextra -pedantic -Werror}
BUILD_DIR=${BUILD_DIR:-build/tests}

update=0
if [ "${1:-}" = "--update" ]; then
    update=1
fi

mkdir -p "$BUILD_DIR" || exit 1
build_dir=$(cd "$BUILD_DIR" && pwd)
failures=0

mask_dates() {
    sed -E -e 's/"[A-Z][a-z]{2} [ 0-9][0-9] [0-9]{4}"/"<__DATE__>"/g' \
           -e 's/"[0-9]{2}:[0-9]{2}:[0-9]{2}"/"<__TIME__>"/g'
}

# run_test <tests subdirectory> <program name> <compare output: yes|no> [program arguments...]
run_test() {
    dir=$1
    name=$2
    compare=$3
    shift 3

    exe="$build_dir/$name"
    if ! $CXX $CXXFLAGS -I. "tests/$dir/$name.cpp" -o "$exe"; then
        echo "FAIL $name: does not build"
        failures=$((failures + 1))
        return
    fi

    (cd "tests/$dir" && "$exe" "$@" > "$exe.stdout" 2> "$exe.stderr")
    status=$?
    if [ $status -ne 0 ]; then
        echo "FAIL $name: exit status $status"
        cat "$exe.stderr"
        failures=$((failures + 1))
        return
    fi

    if [ "$compare" = yes ]; then
        for stream in stdout stderr; do
            expected="tests/expected/$name.$stream"
            mask_dates < "$exe.$stream" > "$exe.$stream.masked"
            if [ $update -eq 1 ]; then
                cp "$exe.$stream.masked" "$expected"
            elif ! diff -u "$expected" "$exe.$stream.masked"; then
                echo "FAIL $name: $stream differs from $expected"
                failures=$((failures + 1))
                return
            fi
        done
    fi

    echo "ok   $name"
}

mkdir -p tests/expected

run_test lexer        misc_lex_tests        yes
run_test lexer        simple_ini_parser     no
run_test lexer        simple_cmdline_parser no \
    -x --foo1 --foo2-bar --foo3=42 --xyz='"hello world"' --ip=172.16.254.1:8080 --file='"some/file/path.txt"' -1z
run_test preprocessor test_eval_macros      yes
run_test preprocessor test_includes         yes
run_test preprocessor test_misc             yes

if [ $failures -ne 0 ]; then
    echo "$failures test(s) failed."
    exit 1
fi
echo "All tests passed."
