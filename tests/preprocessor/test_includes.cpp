
// ================================================================================================
// -*- C++ -*-
// File: test_includes.cpp
// Author: Guilherme R. Lampert
// Created on: 25/05/16
// License: GNU GPL v3.
// Brief: Testing #include resolution with the preprocessor class.
// ================================================================================================

// Compiles with:
//  c++ -std=c++11 -Wall -Wextra -Weffc++ -Wshadow -pedantic -I../../ test_includes.cpp -o pp_test_inc

#define LEXER_IMPLEMENTATION
#include "lexer.hpp"

#define PREPROCESSOR_IMPLEMENTATION
#include "preprocessor.hpp"

#include <iostream>
#include <cstdlib>

int main()
{
    preprocessor pp;
    pp.add_default_search_path("test_files/");

    if (!pp.init_from_file("test_files/first_script.h", preprocessor::flags::warn_macro_redefinitions))
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
