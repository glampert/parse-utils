
// ================================================================================================
// -*- C++ -*-
// File: misc_lex_tests.cpp
// Author: Guilherme R. Lampert
// Created on: 09/05/16
// License: GNU GPL v3.
// Brief: Miscellaneous tests for the lexer class.
// ================================================================================================

// Compiles with:
//  c++ -std=c++11 -Wall -Wextra -Weffc++ -pedantic -I../../ -o misc_lex_tests misc_lex_tests.cpp

#define LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
#define LEXER_IMPLEMENTATION
#include "lexer.hpp"

#include <iostream>
#include <string>
#include <cmath>

// Verbose unless specified otherwise.
#ifndef LEX_TESTS_VERBOSE
    #define LEX_TESTS_VERBOSE 1
#endif // LEX_TESTS_VERBOSE

#if LEX_TESTS_VERBOSE
static void print_token(const lexer::token & tok)
{
    std::string type_str  = lexer::token::type_string(tok.get_type());
    std::string flags_str = lexer::token::flags_string(tok.get_flags(), tok.is_punctuation());

    if (flags_str.empty())
    {
        flags_str = "0";
    }

    std::cout << "\"" << tok.as_string() << "\" => " << "(" << type_str << ", " << flags_str << ")\n";
}
#endif // LEX_TESTS_VERBOSE

static bool floats_equivalent(const double a, const double b, const double epsilon = 0.0001)
{
    return std::fabs(a - b) < epsilon;
}

static void lex_test_scan_num_and_string_values()
{
    #if LEX_TESTS_VERBOSE
    std::cout << "\nScanning values as numbers and strings...\n";
    #endif // LEX_TESTS_VERBOSE

    // no_string_concat so neighboring strings on separate lines are not merged.
    lexer lex{ "lex_test_1.txt", lexer::flags::no_string_concat | lexer::flags::allow_multi_char_literals };

    std::string str;
    str = lex.scan_string(); assert(str == "x");
    str = lex.scan_string(); assert(str == "4cc");
    str = lex.scan_string(); assert(str == "hello world");

    bool b;
    b = lex.scan_number<bool>(); assert(b == false);
    b = lex.scan_number<bool>(); assert(b == true);
    b = lex.scan_number<bool>(); assert(b == false);
    b = lex.scan_number<bool>(); assert(b == true);

    const char  c = lex.scan_number<char>();  assert(c == -120);
    const short s = lex.scan_number<short>(); assert(s == -31000);
    const int   i = lex.scan_number<int>();   assert(i == -4096);
    const long  l = lex.scan_number<long>();  assert(l == 0x11AABBCC);

    const unsigned int  ui = lex.scan_number<unsigned int>();  assert(ui == 6789UL);
    const unsigned long ul = lex.scan_number<unsigned long>(); assert(ul == 0x9908B0DFul);

    const std::uint8_t  u8  = lex.scan_number<std::uint8_t >(); assert(u8  == 0x12);
    const std::uint16_t u16 = lex.scan_number<std::uint16_t>(); assert(u16 == 0xEEFF);
    const std::uint32_t u32 = lex.scan_number<std::uint32_t>(); assert(u32 == 0xDEADBEEF);
    const std::uint64_t u64 = lex.scan_number<std::uint64_t>(); assert(u64 == 0xCAFED00DCAFEBABE);

    const float  f = lex.scan_number<float>();  assert(floats_equivalent(f, 2.718281828f));
    const double d = lex.scan_number<double>(); assert(floats_equivalent(d, 3.14159265358979323846L));

    // Another pass just for printing:
    #if LEX_TESTS_VERBOSE
    lexer::token tok;
    lex.reset();
    while (lex.read_next_token(&tok))
    {
        print_token(tok);
    }
    #endif // LEX_TESTS_VERBOSE
}

static void lex_test_scan_matrices()
{
    #if LEX_TESTS_VERBOSE
    std::cout << "\nScanning matrices of floats...\n";
    #endif // LEX_TESTS_VERBOSE

    lexer lex{ "lex_test_4.txt" };

    // 1D matrix:
    {
        double mat_1d[10] = {0};
        lex.scan_matrix1d(10, mat_1d, "[", "]");

        // 10 values for 9 to 0:
        double expected_val = 9.0;
        for (int i = 0; i < 10; ++i)
        {
            assert(mat_1d[i] == expected_val);
            expected_val -= 1.0;
        }
    }

    // 2D matrix:
    {
        double mat_2d[3 * 3] = {0};
        lex.scan_matrix2d(3, 3, mat_2d, "{", "}");

        // 9 values from 1 to 9:
        double expected_val = 1.0;
        for (int i = 0; i < 3 * 3; ++i)
        {
            assert(mat_2d[i] == expected_val);
            expected_val += 1.0;
        }
    }

    // 3D matrix:
    {
        int mat_3d[2 * 2 * 3] = {0};
        lex.scan_matrix3d(2, 2, 3, mat_3d, "(", ")");

        // 12 values from 1 to 12:
        int expected_val = 1;
        for (int i = 0; i < 2 * 2 * 3; ++i)
        {
            assert(mat_3d[i] == expected_val);
            expected_val += 1;
        }
    }

    #if LEX_TESTS_VERBOSE
    std::cout << "Values matched the expected.\n";
    #endif // LEX_TESTS_VERBOSE
}

static void lex_test_scan_punctuations()
{
    #if LEX_TESTS_VERBOSE
    std::cout << "\nScanning punctuations...\n";
    #endif // LEX_TESTS_VERBOSE

    // The incoming file should have one instance of each punctuation.
    lexer lex{ "lex_test_2.txt" };
    lexer::token tok;

    // Skip the first entry (punctuation_id::none)
    for (std::size_t i = 1; i < lexer::default_punctuations_size; ++i)
    {
        lex.expect_token_type(lexer::token::type::punctuation,
                              static_cast<std::uint32_t>(lexer::default_punctuations[i].id),
                              &tok);

        #if LEX_TESTS_VERBOSE
        print_token(tok);
        #endif // LEX_TESTS_VERBOSE
    }
}

static void lex_test_custom_punct_table()
{
    #if LEX_TESTS_VERBOSE
    std::cout << "\nTesting a custom punctuation table...\n";
    #endif // LEX_TESTS_VERBOSE

    // Testing a "verbose" punctuation set.
    // Note: Order is strict and must match the lexer::punctuation_id enumeration.
    static const lexer::punctuation_def custom_punctuations[]
    {
        { nullptr,                 lexer::punctuation_id::none                },
        { "<assign>",              lexer::punctuation_id::assign              },
        { "<add>",                 lexer::punctuation_id::add                 },
        { "<sub>",                 lexer::punctuation_id::sub                 },
        { "<mul>",                 lexer::punctuation_id::mul                 },
        { "<div>",                 lexer::punctuation_id::div                 },
        { "<mod>",                 lexer::punctuation_id::mod                 },
        { "<rshift>",              lexer::punctuation_id::rshift              },
        { "<lshift>",              lexer::punctuation_id::lshift              },
        { "<add_assign>",          lexer::punctuation_id::add_assign          },
        { "<sub_assign>",          lexer::punctuation_id::sub_assign          },
        { "<mul_assign>",          lexer::punctuation_id::mul_assign          },
        { "<div_assign>",          lexer::punctuation_id::div_assign          },
        { "<mod_assign>",          lexer::punctuation_id::mod_assign          },
        { "<rshift_assign>",       lexer::punctuation_id::rshift_assign       },
        { "<lshift_assign>",       lexer::punctuation_id::lshift_assign       },
        { "<logic_and>",           lexer::punctuation_id::logic_and           },
        { "<logic_or>",            lexer::punctuation_id::logic_or            },
        { "<logic_not>",           lexer::punctuation_id::logic_not           },
        { "<logic_eq>",            lexer::punctuation_id::logic_eq            },
        { "<logic_not_eq>",        lexer::punctuation_id::logic_not_eq        },
        { "<logic_greater>",       lexer::punctuation_id::logic_greater       },
        { "<logic_less>",          lexer::punctuation_id::logic_less          },
        { "<logic_greater_eq>",    lexer::punctuation_id::logic_greater_eq    },
        { "<logic_less_eq>",       lexer::punctuation_id::logic_less_eq       },
        { "<plus_plus>",           lexer::punctuation_id::plus_plus           },
        { "<minus_minus>",         lexer::punctuation_id::minus_minus         },
        { "<bitwise_and>",         lexer::punctuation_id::bitwise_and         },
        { "<bitwise_or>",          lexer::punctuation_id::bitwise_or          },
        { "<bitwise_xor>",         lexer::punctuation_id::bitwise_xor         },
        { "<bitwise_not>",         lexer::punctuation_id::bitwise_not         },
        { "<bitwise_and_assign>",  lexer::punctuation_id::bitwise_and_assign  },
        { "<bitwise_or_assign>",   lexer::punctuation_id::bitwise_or_assign   },
        { "<bitwise_xor_assign>",  lexer::punctuation_id::bitwise_xor_assign  },
        { "<dot>",                 lexer::punctuation_id::dot                 },
        { "<arrow>",               lexer::punctuation_id::arrow               },
        { "<colon_colon>",         lexer::punctuation_id::colon_colon         },
        { "<dot_star>",            lexer::punctuation_id::dot_star            },
        { "<comma>",               lexer::punctuation_id::comma               },
        { "<semicolon>",           lexer::punctuation_id::semicolon           },
        { "<colon>",               lexer::punctuation_id::colon               },
        { "<question_mark>",       lexer::punctuation_id::question_mark       },
        { "<ellipsis>",            lexer::punctuation_id::ellipsis            },
        { "<backslash>",           lexer::punctuation_id::backslash           },
        { "<open_parentheses>",    lexer::punctuation_id::open_parentheses    },
        { "<close_parentheses>",   lexer::punctuation_id::close_parentheses   },
        { "<open_bracket>",        lexer::punctuation_id::open_bracket        },
        { "<close_bracket>",       lexer::punctuation_id::close_bracket       },
        { "<open_curly_bracket>",  lexer::punctuation_id::open_curly_bracket  },
        { "<close_curly_bracket>", lexer::punctuation_id::close_curly_bracket },
        { "<preprocessor>",        lexer::punctuation_id::preprocessor        },
        { "<preprocessor_merge>",  lexer::punctuation_id::preprocessor_merge  },
        { "<dollar_sign>",         lexer::punctuation_id::dollar_sign         }
    };

    // The additional table buffers must remain valid until the next set_punctuation_tables() or
    // set_default_punctuation_tables() call, so the best way to go about it is to provide pointers to static arrays.
    constexpr std::size_t custom_punctuations_size = sizeof(custom_punctuations) / sizeof(custom_punctuations[0]);
    static lexer::punct_table_index_type custom_punctuations_next[custom_punctuations_size];
    static lexer::punct_table_index_type custom_punctuations_table[256];

    // Now all lexer instances will share this punctuation set until a new one is set.
    lexer::set_punctuation_tables(custom_punctuations,
                                  custom_punctuations_table,
                                  custom_punctuations_next,
                                  custom_punctuations_size);

    lexer lex{ "lex_test_6.txt" };
    lexer::token tok;

    // Skip the first entry (punctuation_id::none)
    for (std::size_t i = 1; i < custom_punctuations_size; ++i)
    {
        lex.expect_token_type(lexer::token::type::punctuation,
                              static_cast<std::uint32_t>(custom_punctuations[i].id),
                              &tok);

        #if LEX_TESTS_VERBOSE
        print_token(tok);
        #endif // LEX_TESTS_VERBOSE
    }

    // Restore for the other tests.
    lexer::set_default_punctuation_tables();
}

static void lex_test_line_count()
{
    #if LEX_TESTS_VERBOSE
    std::cout << "\nCounting lines in file...\n";
    #endif // LEX_TESTS_VERBOSE

    // Scan as space-delimited strings (only_strings).
    lexer lex{ "lex_test_3.txt", lexer::flags::only_strings };

    int line_count = 0;
    std::string line;

    for (; !lex.is_at_end(); ++line_count)
    {
        line = lex.scan_complete_line();

        #if LEX_TESTS_VERBOSE
        std::cout << "LINE: " << line;
        #endif // LEX_TESTS_VERBOSE
    }

    #if LEX_TESTS_VERBOSE
    std::cout << "Counted " << line_count << " lines.\n";
    #endif // LEX_TESTS_VERBOSE

    assert(line_count == 42);
}

static void lex_test_word_count()
{
    #if LEX_TESTS_VERBOSE
    std::cout << "\nCounting words in file...\n";
    #endif // LEX_TESTS_VERBOSE

    // Words will be lexed as identifiers. Punctuations are preserved.
    // (only_strings flag would also lex punctuation as identifiers).
    lexer lex{ "lex_test_3.txt", lexer::flags::no_string_concat };

    lexer::token word;
    int word_count = 0;

    while (lex.read_next_token(&word))
    {
        if (word.is_identifier())
        {
            ++word_count;
        }

        #if LEX_TESTS_VERBOSE
        print_token(word);
        #endif // LEX_TESTS_VERBOSE
    }

    #if LEX_TESTS_VERBOSE
    std::cout << "Counted " << word_count << " words.\n";
    #endif // LEX_TESTS_VERBOSE

    assert(word_count == 527);
}

// ========================================================
// main():
// ========================================================

int main()
{
    std::cout << "\nRunning lexer tests...\n";

    lex_test_scan_num_and_string_values();
    lex_test_scan_matrices();
    lex_test_scan_punctuations();
    lex_test_custom_punct_table();
    lex_test_line_count();
    lex_test_word_count();

    std::cout << "\nAll tests passed!\n";
}
