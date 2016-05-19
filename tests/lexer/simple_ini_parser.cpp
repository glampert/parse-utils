
// ================================================================================================
// -*- C++ -*-
// File: simple_ini_parser.cpp
// Author: Guilherme R. Lampert
// Created on: 09/05/16
// License: GNU GPL v3.
// Brief: Basic sample demonstrating how to use the lexer class to parse INI configuration files.
// ================================================================================================

// Compiles with:
//  c++ -std=c++11 -Wall -Wextra -Weffc++ -pedantic -I../../ -o simple_ini_parser simple_ini_parser.cpp

#define LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
#define LEXER_IMPLEMENTATION
#include "lexer.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

namespace ini
{

enum class value_type
{
    none,
    string,
    number,
    ip_addr,
    boolean
};

// This could also be a tagged union but since std::string has
// constructor/destructor we use a struct to keep things simple.
struct value_holder final
{
    std::string   string_val;
    double        number_val  = 0;
    std::uint64_t ip_addr_val = 0;
    bool          bool_val    = false;
    value_type    type        = value_type::none;
};

// [key, value] pairs.
using section = std::unordered_map<std::string, value_holder>;

// List of sections indexed by name.
using sections_list = std::unordered_map<std::string, section>;

// Parses the .ini file filling the sections_list.
// Might throw lexer::exceptions on error.
void parse_file(std::string filename, sections_list * out_sections)
{
    lexer lex;
    lexer::token tok;
    section * current_section = nullptr;

    lex.init_from_file(std::move(filename), lexer::flags::allow_ip_addresses);

    while (lex.next_token(&tok))
    {
        if (tok.is_punctuation())
        {
            if (tok == '#' || tok == ';')
            {
                // ini-style comments.
                // The lexer only skips C and C++ style comments by default.
                lex.skip_rest_of_line();
            }
            else if (tok == '[') // New section
            {
                lex.expect_token_type(lexer::token::type::identifier, 0, &tok);
                lex.expect_token_char(']'); // Closing the section

                current_section = &(*out_sections)[tok.as_string()];
            }
        }
        else if (tok.is_identifier()) // Key=value pair
        {
            value_holder ini_val;
            lexer::token value_tok;

            lex.expect_token_char('=');
            lex.expect_any_token(&value_tok);

            if (value_tok.is_boolean() || value_tok.is_number() || value_tok.is_string())
            {
                if (current_section == nullptr) // Make a default if none was set yet.
                {
                    current_section = &(*out_sections)["global"];
                }

                if (value_tok.is_string())
                {
                    ini_val.type = value_type::string;
                    ini_val.string_val = value_tok.as_string();
                }
                else if (value_tok.is_number())
                {
                    if (value_tok.get_flags() & lexer::token::flags::ip_address)
                    {
                        ini_val.type = value_type::ip_addr;
                        ini_val.ip_addr_val = value_tok.as_uint64();
                    }
                    else
                    {
                        ini_val.type = value_type::number;
                        ini_val.number_val = value_tok.as_double();
                    }
                }
                else if (value_tok.is_boolean())
                {
                    ini_val.type = value_type::boolean;
                    ini_val.bool_val = value_tok.as_bool();
                }

                (*current_section)[tok.as_string()] = ini_val;
            }
        }
    }
}

// Debug printing of a sections_list built by parse_file().
void print_sections_list(const sections_list & sections, std::ostream * out_str)
{
    for (const auto & sect : sections)
    {
        (*out_str) << "[" << sect.first << "]\n";

        for (const auto & var : sect.second)
        {
            (*out_str) << var.first << " = ";

            switch (var.second.type)
            {
            case value_type::string :
                (*out_str) << "\"" << var.second.string_val << "\"";
                break;

            case value_type::number :
                (*out_str) << var.second.number_val;
                break;

            case value_type::ip_addr :
                (*out_str) << ((var.second.ip_addr_val & 0xFF000000) >> 24) << "."
                           << ((var.second.ip_addr_val & 0x00FF0000) >> 16) << "."
                           << ((var.second.ip_addr_val & 0x0000FF00) >>  8) << "."
                           << ((var.second.ip_addr_val & 0x000000FF) >>  0) << ":"
                           << (var.second.ip_addr_val >> 32); // Port # follows in the leftmost 32 bits.
                break;

            case value_type::boolean :
                (*out_str) << (var.second.bool_val ? "true" : "false");
                break;

            default :
                (*out_str) << "none";
                break;
            } // switch (var.second.type)

            (*out_str) << "\n";
        }

        (*out_str) << "\n";
    }
}

} // namespace ini {}

// ========================================================
// main():
// ========================================================

int main()
{
    ini::sections_list sections;
    ini::parse_file("lex_test_5.ini", &sections);

    std::cout << "\nContents of the parsed INI file:\n\n";
    ini::print_sections_list(sections, &std::cout);
}
