
# parse-utils is header-only, so the only things to build are the tests.
# 'make test' builds every test program and runs it (see tests/run_tests.sh);
# 'make update-test-output' rewrites the expected output the tests compare against.

CXXFLAGS ?= -std=c++20 -O0 -g -Wall -Wextra -pedantic -Werror

test:
	@CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" sh tests/run_tests.sh

update-test-output:
	@CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" sh tests/run_tests.sh --update

clean:
	rm -rf build

.PHONY: test update-test-output clean
