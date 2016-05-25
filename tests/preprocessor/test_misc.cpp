
// ================================================================================================
// -*- C++ -*-
// File: test_misc.cpp
// Author: Guilherme R. Lampert
// Created on: 25/05/16
// License: GNU GPL v3.
// Brief: Testing some of the miscellaneous member methods of the preprocessor class.
// ================================================================================================

// Compiles with:
//  c++ -std=c++11 -Wall -Wextra -Weffc++ -Wshadow -pedantic -I../../ test_misc.cpp -o pp_test_misc

#define LEXER_IMPLEMENTATION
#include "lexer.hpp"

#define PREPROCESSOR_IMPLEMENTATION
#include "preprocessor.hpp"

#include <iostream>
#include <cassert>

int main()
{
    preprocessor pp;

    // Global definitions:
    {
        const std::int64_t ival = 1337;
        const double       dval = 3.141592;
        const std::string  sval = "Who is John Galt?";

        assert(pp.define("TEST_BUILTIN_INT", ival, false) == true);
        assert(pp.define("TEST_BUILTIN_FLT", dval, false) == true);
        assert(pp.define("TEST_BUILTIN_STR", sval, false) == true);

        assert(pp.is_defined("TEST_BUILTIN_INT") == true);
        assert(pp.is_defined("TEST_BUILTIN_FLT") == true);
        assert(pp.is_defined("TEST_BUILTIN_STR") == true);

        // A function-like macro:
        assert(pp.define("#define SQUARE(x) ((x) * (x))", false) == true);
        assert(pp.is_defined("SQUARE") == true);

        // Don't allow it to be redefined.
        assert(pp.define("#define SQUARE 2", false) == false);
    }

    // Check the values of the above definitions:
    {
        std::int64_t ival = 0;
        assert(pp.find_macro_value("TEST_BUILTIN_INT", &ival) == true);
        assert(ival == 1337);

        double dval = 0;
        assert(pp.find_macro_value("TEST_BUILTIN_FLT", &dval) == true);
        assert(dval == 3.141592);

        std::string sval;
        assert(pp.find_macro_value("TEST_BUILTIN_STR", &sval) == true);
        assert(sval == "Who is John Galt?");

        int num_tokens = 0;
        const auto p_macro_tokens = pp.find_macro_tokens("SQUARE", &num_tokens);

        const char * tokens_expected[]{ "(", "(", "x", ")", "*", "(", "x", ")", ")" };
        assert(static_cast<unsigned>(num_tokens) == (sizeof(tokens_expected) / sizeof(tokens_expected[0])));

        for (int t = 0; t < num_tokens; ++t)
        {
            assert(p_macro_tokens[t] == tokens_expected[t]);
        }
    }

    // Expression evaluator:
    {
        std::int64_t iresult = 0;
        double       dresult = 0;
        bool         success;

        success = pp.eval("(1 + 1) * (1 + 1) == 4", &iresult, &dresult, false, false, false);
        assert(success == true);
        assert(iresult == 1);
        assert(dresult == 1.0);

        success = pp.eval("(1 << 1) ^ (1 << 2)", &iresult, &dresult, false, false, false);
        assert(success == true);
        assert(iresult == 6);
        assert(dresult == 6.0);

        success = pp.eval("1.5 + 3.0", &iresult, &dresult, false, false, false);
        assert(success == true);
        assert(iresult == 4);
        assert(dresult == 4.5);

        // We can also test if macros are defined in the preprocessor instance:
        success = pp.eval("defined(SQUARE)", &iresult, &dresult, false, false, false);
        assert(success == true);
        assert(iresult == 1);
        assert(dresult == 1);
    }
}
