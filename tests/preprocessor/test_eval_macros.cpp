
// ================================================================================================
// -*- C++ -*-
// File: test_eval_macros.cpp
// Author: Guilherme R. Lampert
// Created on: 25/05/16
// License: GNU GPL v3.
// Brief: Test preprocessing a file with lots of $eval()s and some #defined macros.
// ================================================================================================

// Compiles with:
//  c++ -std=c++11 -Wall -Wextra -Weffc++ -Wshadow -pedantic -I../../ test_eval_macros.cpp -o pp_test_eval_macros

#define LEXER_IMPLEMENTATION
#include "lexer.hpp"

#define PREPROCESSOR_IMPLEMENTATION
#include "preprocessor.hpp"

#include <iostream>
#include <cstdlib>

int main()
{
    preprocessor pp;

    if (!pp.init_from_file("test_files/evals_and_macros.h", preprocessor::flags::warn_macro_redefinitions))
    {
        std::cerr << "Can't open " << pp.get_current_script()->get_filename() << "\n";
        return EXIT_FAILURE;
    }

    std::string result;
    if (!pp.preprocess(&result))
    {
        std::cerr << "Failed to preprocess file " << pp.get_current_script()->get_filename() << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "\n";
    std::cout << result << "\n";
    std::cout << "\n";
}
