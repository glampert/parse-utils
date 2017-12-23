//
// Basic preprocessor and lexer tests for Windows
//

#define LEXER_IMPLEMENTATION
#include "../../lexer.hpp"

#define PREPROCESSOR_IMPLEMENTATION
#include "../../preprocessor.hpp"

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

    // Built-in macros, scripts:
    {
        const char scr[] = "#define ONE   1\n"
                           "#define TWO   2\n"
                           "#define THREE 3\n"
                           "#define FOUR  4\n";
        pp.init_from_memory(scr, sizeof(scr) - 1, "test_script.txt",
                            preprocessor::flags::no_fatal_errors);

        std::string result;
        if (!pp.preprocess(&result))
        {
            std::cerr << "Failed to preprocess script!\n";
            return EXIT_FAILURE;
        }

        assert(pp.is_defined("__FILE__"));
        assert(pp.is_defined("__LINE__"));
        assert(pp.is_defined("__DATE__"));
        assert(pp.is_defined("__TIME__"));
        assert(pp.is_defined("__VA_ARGS__"));

        const bool allow_built_ins = true;
        lexer::token file, line, date, time, args;
        assert(pp.find_macro_token("__FILE__", &file, allow_built_ins) == true); // current script loc
        assert(pp.find_macro_token("__LINE__", &line, allow_built_ins) == true); // current script loc
        assert(pp.find_macro_token("__DATE__", &date, allow_built_ins) == true); // current system date
        assert(pp.find_macro_token("__TIME__", &time, allow_built_ins) == true); // current system time
        assert(pp.find_macro_token("__VA_ARGS__", &args, allow_built_ins) == false); // can't expand in this context

        std::cout << "__FILE__: " << file.as_string() << '\n';
        std::cout << "__LINE__: " << line.as_string() << '\n';
        std::cout << "__DATE__: " << date.as_string() << '\n';
        std::cout << "__TIME__: " << time.as_string() << '\n';
        std::cout << "__VA_ARGS__: " << args.as_string() << '\n';

        std::int64_t num = 0;
        assert(pp.find_macro_value("ONE",   &num) && num == 1);
        assert(pp.find_macro_value("TWO",   &num) && num == 2);
        assert(pp.find_macro_value("THREE", &num) && num == 3);
        assert(pp.find_macro_value("FOUR",  &num) && num == 4);
    }
}
