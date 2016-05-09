
// ================================================================================================
// -*- C++ -*-
// File: simple_cmdline_parser.cpp
// Author: Guilherme R. Lampert
// Created on: 09/05/16
// License: GNU GPL v3.
// Brief: Basic sample demonstrating how to use the lexer class to implement a command-line parser.
// ================================================================================================

// Compiles with:
//  c++ -std=c++11 -Wall -Wextra -Weffc++ -pedantic -I../../ -o simple_cmdline_parser simple_cmdline_parser.cpp

#define LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
#define LEXER_IMPLEMENTATION
#include "lexer.hpp"

#include <cstdint>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

namespace cmdline
{

//
// Accepted types of command-line flags:
// -x        : boolean flag, true if present, false otherwise.
// --foo     : long version of the boolean flag.
// --foo=bar : long flag with an explicit value.
//
// The long flags can contain one or more '-' in the middle:
// --foo-bar=baz
//
// The value can be quoted to denote a string with whitespace:
// --foo="hello world"
//

enum class value_type
{
    none,
    string,
    number,
    ip_addr
};

// This could also be a tagged union but since std::string has
// constructor/destructor we use a struct to keep things simple.
struct value_holder final
{
    std::string   string_val;
    double        number_val  = 0;
    std::uint64_t ip_addr_val = 0;
    value_type    type        = value_type::none;
};

// [flag-name, value] pairs. If the flag has no value the type tag is set to value_type::none.
using flag_pair  = std::pair<std::string, value_holder>;
using flags_list = std::unordered_map<std::string, value_holder>;

// Test if the given flag is defined, regardless of its value.
bool has_flag(const flags_list & flags, const std::string & flag_wanted)
{
    return flags.find(flag_wanted) != std::end(flags);
}

// Parses the traditional main-style command line using a lexer.
// Might throw lexer::exceptions on error.
void parse_argc_argv(const int argc, const char * argv[], flags_list * out_flags)
{
    lexer lex;
    lexer::token tok;

    // Each entry of the argv[] is a string, skipping the first
    // which is always the program name, so we lex each individual
    // command string to make things simpler.
    for (int i = 1; i < argc; ++i)
    {
        lex.init_from_memory(argv[i], std::strlen(argv[i]), "(cmdline)",
                             lexer::flags::allow_number_names |
                             lexer::flags::allow_ip_addresses |
                             lexer::flags::allow_multi_char_literals);

        while (lex.read_next_token(&tok))
        {
            if (tok == '-')
            {
                lex.expect_token_type(lexer::token::type::identifier, 0, &tok);
                (*out_flags)[tok.as_string()] = {};
            }
            else if (tok == "--")
            {
                lex.expect_token_type(lexer::token::type::identifier, 0, &tok);
                std::string flag_name = tok.as_string();

                lexer::token next_tok;
                if (lex.read_next_token(&next_tok))
                {
                    if (next_tok == '-') // Composite name, like "foo-bar"
                    {
                        lex.expect_token_type(lexer::token::type::identifier, 0, &next_tok);

                        flag_name += "-";
                        flag_name += next_tok.as_string();
                        (*out_flags)[flag_name] = {};
                    }
                    else if (next_tok == '=') // Value part follows
                    {
                        if (!lex.read_next_token(&next_tok))
                        {
                            lex.error("expected value after \'" + flag_name + "=\' in cmdline!");
                        }

                        value_holder & current_flag = (*out_flags)[flag_name];

                        if (next_tok.is_string())
                        {
                            current_flag.type = value_type::string;
                            current_flag.string_val = next_tok.as_string();
                        }
                        else if (next_tok.is_number())
                        {
                            if (next_tok.get_flags() & lexer::token::flags::ip_address)
                            {
                                current_flag.type = value_type::ip_addr;
                                current_flag.ip_addr_val = next_tok.as_uint64();
                            }
                            else // Integer or float number.
                            {
                                current_flag.type = value_type::number;
                                current_flag.number_val = next_tok.as_double();
                            }
                        }
                        else
                        {
                            lex.error("cmdline flag type is unsupported!");
                        }
                    }
                    else
                    {
                        lex.error("unexpected token in cmdline: \'" + next_tok.as_string() + "\".");
                    }
                }
                else
                {
                    (*out_flags)[flag_name] = {};
                }
            }
        }

        lex.clear();
    }
}

// Debug printing of a flags_list built by parse_argc_argv().
void print_flags_list(const flags_list & flags, std::ostream * out_str)
{
    for (const auto & f : flags)
    {
        (*out_str) << "FLAG: " << f.first;

        if (f.second.type != value_type::none)
        {
            (*out_str) << "=";

            switch (f.second.type)
            {
            case value_type::string :
                (*out_str) << "\"" << f.second.string_val << "\"";
                break;

            case value_type::number :
                (*out_str) << f.second.number_val;
                break;

            case value_type::ip_addr :
                (*out_str) << ((f.second.ip_addr_val & 0xFF000000) >> 24) << "."
                           << ((f.second.ip_addr_val & 0x00FF0000) >> 16) << "."
                           << ((f.second.ip_addr_val & 0x0000FF00) >>  8) << "."
                           << ((f.second.ip_addr_val & 0x000000FF) >>  0) << ":"
                           << (f.second.ip_addr_val >> 32); // Port # follows in the leftmost 32 bits.
                break;

            default :
                break;
            } // switch (f.second.type)
        }

        (*out_str) << "\n";
    }

    (*out_str) << "\n";
}

} // namespace cmdline {}

// ========================================================
// main():
// ========================================================

int main(int argc, const char * argv[])
{
    // Sample command line:
    //
    // ./simple_cmdline_parser -x --foo1 --foo2-bar --foo3=42 --xyz='"hello world"' --ip=172.16.254.1:8080 --file='"some/file/path.txt"' -1z
    //

    cmdline::flags_list cmd_flags;
    cmdline::parse_argc_argv(argc, argv, &cmd_flags);

    std::cout << "\nFlags parsed from the command-line:\n\n";
    cmdline::print_flags_list(cmd_flags, &std::cout);

    assert(cmdline::has_flag(cmd_flags, "x"));
    assert(cmdline::has_flag(cmd_flags, "foo1"));
    assert(cmdline::has_flag(cmd_flags, "foo2-bar"));
    assert(cmdline::has_flag(cmd_flags, "foo3"));
    assert(cmdline::has_flag(cmd_flags, "xyz"));
    assert(cmdline::has_flag(cmd_flags, "ip"));
    assert(cmdline::has_flag(cmd_flags, "file"));
    assert(cmdline::has_flag(cmd_flags, "1z"));
}
