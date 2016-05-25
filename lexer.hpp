
// ================================================================================================
// -*- C++ -*-
// File: lexer.hpp
// Author: Guilherme R. Lampert
// Created on: 09/05/16
//
// About:
//  Lightweight C and C++ compatible lexer, based on idLexer from id Tech 4.
//
// License:
//  This source code is work derived from a similar class found in the source release
//  of DOOM 3 BFG by id Software, available at <https://github.com/id-Software/DOOM-3-BFG>,
//  and therefore is released under the GNU General Public License version 3 to comply
//  with the original work. See the accompanying LICENSE file for full disclosure.
// ================================================================================================

#ifndef LEXER_HPP
#define LEXER_HPP

#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>

// The use of exceptions for error handling can be disabled via this preprocessor.
#ifndef LEXER_NO_CXX_EXCEPTIONS
    #include <stdexcept>
#endif // LEXER_NO_CXX_EXCEPTIONS

//
// ----------
//  OVERVIEW
// ----------
//
// The following C++ class was extracted and adapted from the
// lexicographical scanner used by the DOOM 3 game and the id Tech 4 engine.
// Original source at: https://github.com/id-Software/DOOM-3-BFG/blob/master/neo/idlib/Lexer.h
//
// You can use it to lex C-like languages as well as configuration
// files and command lines. Check the accompanying samples for some
// of the possible uses.
//
// It does not allocate memory during scanning if you have provided
// an input text buffer on initialization. If loading a script from
// file then the lexer will allocate a memory buffer big enough to
// hold the contents of the whole file. The output tokens will not
// explicitly allocate memory either, but they use a std::string
// internally, which might allocate for long strings. Normally you
// can reuse tokens as you parse a file, so the overall memory
// performance should be pretty decent.
//
// Number literals can be specified in binary, decimal, octal and hexadecimal
// format (0b or 0B prefix for binary). A number directly following the
// escape character '\' in a string is assumed to be in decimal format
// instead of octal. Floating-point numbers are also accepted, as well
// as IP addresses in the form: 172.16.254.1:8080, where the port num
// and the ':' is optional.
//
// License:
// To comply with the license used by DOOM 3 and id Tech 4, this source file
// is released under the terms of the GNU General Public License version 3.
//
// -------------
//  QUICK SETUP
// -------------
//
// In *one* C++ source file, *before* including this file, do this:
//
//   #define LEXER_IMPLEMENTATION
//
// To enable the implementation. Further includes of this
// file *should not* redefine LEXER_IMPLEMENTATION.
// Example:
//
// In my_program.cpp:
//
//   #define LEXER_IMPLEMENTATION
//   #include "lexer.hpp"
//
// In my_program.hpp:
//
//   #include "lexer.hpp"
//
// When the implementation macro is defined, the implementation of
// this class will be emitted in that source file, so all you have
// to do to add the lexer to your projected is copy and #include
// this file, no additional build setup required.
//
class lexer final
{
public:

    //
    // token:
    //
    // Basic output of the lexer. Each token consist of a string and
    // possibly a numerical value scanned from the string. Tokens rely
    // on std::string and so won't allocate memory for small strings,
    // thanks to the Small String Optimization of the standard string.
    //
    // Numerical values and IP addresses are only scanned from the
    // token string if you request the value via one of the as_* methods.
    // Once scanned, the value is cached, so the conversion will only
    // take place once.
    //
    class token final
    {
    public:

        // Main token category:
        enum class type : std::uint8_t
        {
            none,
            number,
            string,
            literal,
            identifier,
            punctuation
        }; // type

        // Subtypes and flags:
        struct flags final
        {
            // Integer types:
            static constexpr std::uint32_t integer            = 1 << 0;
            static constexpr std::uint32_t signed_integer     = 1 << 1;
            static constexpr std::uint32_t unsigned_integer   = 1 << 2;
            // Integer representations:
            static constexpr std::uint32_t binary             = 1 << 3;
            static constexpr std::uint32_t octal              = 1 << 4;
            static constexpr std::uint32_t decimal            = 1 << 5;
            static constexpr std::uint32_t hexadecimal        = 1 << 6;
            // Floating-point types:
            static constexpr std::uint32_t floating_point     = 1 << 7;
            static constexpr std::uint32_t single_precision   = 1 << 8;
            static constexpr std::uint32_t double_precision   = 1 << 9;
            static constexpr std::uint32_t extended_precision = 1 << 10;
            // Floating-point exceptions:
            static constexpr std::uint32_t infinite           = 1 << 11;
            static constexpr std::uint32_t indefinite         = 1 << 12;
            static constexpr std::uint32_t nan                = 1 << 13;
            // IP address and port #:
            static constexpr std::uint32_t ip_address         = 1 << 14;
            static constexpr std::uint32_t ip_port            = 1 << 15;
            // true|false, 0|1 as boolean literals:
            static constexpr std::uint32_t boolean            = 1 << 16;
        }; // flags

        token() = default;
        token(const token & other) = default;
        token & operator = (const token & other) = default;

        // Custom move op/assign:
        token(token && other) noexcept;
        token & operator = (token && other) noexcept;

        // Value access with conversion:
        bool                as_bool()           const noexcept;
        float               as_float()          const noexcept;
        double              as_double()         const noexcept;
        std::int32_t        as_int32()          const noexcept;
        std::int64_t        as_int64()          const noexcept;
        std::uint32_t       as_uint32()         const noexcept;
        std::uint64_t       as_uint64()         const noexcept;
        const std::string & as_string()         const noexcept;

        // Queries:
        bool                is_number()         const noexcept;
        bool                is_integer()        const noexcept;
        bool                is_float()          const noexcept;
        bool                is_boolean()        const noexcept;
        bool                is_string()         const noexcept;
        bool                is_literal()        const noexcept;
        bool                is_identifier()     const noexcept;
        bool                is_punctuation()    const noexcept;
        std::size_t         get_length()        const noexcept;
        std::uint32_t       get_flags()         const noexcept;
        std::uint32_t       get_line_number()   const noexcept;
        std::uint32_t       get_lines_crossed() const noexcept;
        type                get_type()          const noexcept;

        // Comparison with raw text strings and char literals:
        bool operator == (char c) const noexcept;
        bool operator != (char c) const noexcept;
        bool operator == (const char * str) const noexcept;
        bool operator != (const char * str) const noexcept;
        bool operator == (const std::string & str) const noexcept;
        bool operator != (const std::string & str) const noexcept;

        // Setters used by the lexer:
        void set_string(std::string new_text);
        void set_flags(std::uint32_t new_flags) noexcept;
        void set_line_number(std::uint32_t new_line_num) noexcept;
        void set_lines_crossed(std::uint32_t new_lines_crossed) noexcept;
        void set_type(type new_type) noexcept;

        // Miscellaneous:
        char operator[](int index) const;
        void append(const char * str);
        void append(char c);
        void move_to(std::string * dest_str) noexcept;
        void clear() noexcept;
        token stringize() const;
        token trim() const;

        // Helpers to generate debug strings for the token type enum and flags:
        static std::string type_string(type type);
        static std::string flags_string(std::uint32_t flags, bool is_punct = false);

    private:

        std::uint64_t get_value_u64() const noexcept;
        double get_value_double()     const noexcept;
        void update_cached_values()   const noexcept;

        std::string           m_string;
        std::uint32_t         m_flags         = 0;
        std::uint32_t         m_line_num      = 0;
        std::uint32_t         m_lines_crossed = 0;
        type                  m_type          = type::none;
        mutable bool          m_values_valid  = true;
        mutable std::uint64_t m_u64_value     = 0;
        mutable double        m_double_value  = 0.0;
    }; // token

    //
    // punctuation_id:
    //
    // Tags for the C/C++ punctuation set.
    //
    enum class punctuation_id : std::uint8_t
    {
        none,                // no punctuation (0)
        assign,              // =
        add,                 // +
        sub,                 // -
        mul,                 // *
        div,                 // /
        mod,                 // %
        rshift,              // >>
        lshift,              // <<
        add_assign,          // +=
        sub_assign,          // -=
        mul_assign,          // *=
        div_assign,          // /=
        mod_assign,          // %=
        rshift_assign,       // >>=
        lshift_assign,       // <<=
        logic_and,           // &&
        logic_or,            // ||
        logic_not,           // !
        logic_eq,            // ==
        logic_not_eq,        // !=
        logic_greater,       // >
        logic_less,          // <
        logic_greater_eq,    // >=
        logic_less_eq,       // <=
        plus_plus,           // ++
        minus_minus,         // --
        bitwise_and,         // &
        bitwise_or,          // |
        bitwise_xor,         // ^
        bitwise_not,         // ~
        bitwise_and_assign,  // &=
        bitwise_or_assign,   // |=
        bitwise_xor_assign,  // ^=
        dot,                 // .
        arrow,               // ->
        colon_colon,         // ::
        dot_star,            // .*
        comma,               // ,
        semicolon,           // ;
        colon,               // :
        question_mark,       // ?
        ellipsis,            // ...
        backslash,           // '\'
        open_parentheses,    // (
        close_parentheses,   // )
        open_bracket,        // [
        close_bracket,       // ]
        open_curly_bracket,  // {
        close_curly_bracket, // }
        preprocessor,        // #
        preprocessor_merge,  // ##
        dollar_sign          // $
    }; // punctuation_id

    // Checks if the given token is a punctuation and its flags equal the punctuation_id.
    static bool is_punctuation_token(const token & tok, punctuation_id id) noexcept;

    //
    // punctuation_def:
    //
    // Pair of a punctuation character or string (if multi-character)
    // and its equivalent punctuation_id tag.
    //
    struct punctuation_def final
    {
        const char * const   str;
        const punctuation_id id;
    }; // punctuation_def

    // Type used by the punctuations_table[] and punctuations_next[].
    // Must be a signed integer (-1 used to indicate free entries).
    using punct_table_index_type = std::int16_t;

    //
    // Default punctuation tables:
    //
    // Punctuation tables for the C/C++ set.
    // default_punctuations[] has one entry for
    // each of the punctuation_id tags above.
    //
    // Note: punctuations_table[] and punctuations_next[] will
    // be dynamically filled during global initialization, so
    // that setup step is not thread-safe. Setting new punctuation
    // tables is also not thread-safe since it writes to the
    // shared punctuation table pointers.
    //
    static const punctuation_def  default_punctuations[];
    static const std::size_t      default_punctuations_size;
    static punct_table_index_type default_punctuations_table[];
    static punct_table_index_type default_punctuations_next[];
    static bool                   default_punctuations_initialized;

    //
    // set_punctuation_tables():
    //
    // Sets up the internal punctuation tables from the provided punctuation set.
    // By default, lexer uses the default_punctuations for C/C++. If providing a new
    // set, the pointers to punctuations[], punctuations_table[] and punctuations_next[]
    // must point to a memory buffer that outlives the lexer instance (you'll usually want
    // to provide pointers to static arrays).
    //
    // punctuations[] is assumed to have punctuations_size entries.
    // punctuations_table[] must match the ASCII table, so it requires 256 entries at least.
    // punctuations_next[] must have at least punctuations_size entries in it.
    //
    // Note: Not thread-safe.
    //
    static void set_punctuation_tables(const punctuation_def  * punctuations,
                                       punct_table_index_type * punctuations_table,
                                       punct_table_index_type * punctuations_next,
                                       std::size_t punctuations_size);

    //
    // set_default_punctuation_tables():
    //
    // Resets the initial default C/C++ punctuation tables.
    // Same as calling set_punctuation_tables() with the
    // default_punctuations defined above. This operation
    // is not thread-safe.
    //
    static void set_default_punctuation_tables();

    //
    // get_punctuation_from_id()
    // get_punctuation_id_from_str()
    //
    // Lookup the currently set punctuation table.
    //
    static std::string get_punctuation_from_id(punctuation_id id);
    static punctuation_id get_punctuation_id_from_str(const char * punctuation_string);

    //
    // Error / warning output:
    //
    class error_callbacks
    {
    public:
        virtual void error(const std::string & message, bool fatal) = 0;
        virtual void warning(const std::string & message) = 0;
        virtual ~error_callbacks() = default;
    }; // error_callbacks

    // Default error/warning callbacks print to std::cerr.
    // Default error() will throw lexer::exception if errors are fatal and exception are enabled.
    // Setting the callbacks to null restores the default ones.
    // Note 1: These two methods are not thread-safe.
    // Note 2: Lexer will never attempt to deallocate the provided pointer.
    static void set_error_callbacks(error_callbacks * err_callbacks) noexcept;
    static error_callbacks * get_error_callbacks() noexcept;

    //
    // lexer::error() throws lexer::exception if errors are set to fatal.
    // Disabling exceptions globally via the preprocessor causes the default
    // error handler to just return with false.
    //
    #ifndef LEXER_NO_CXX_EXCEPTIONS
    class exception final
        : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    }; // exception
    #endif // LEXER_NO_CXX_EXCEPTIONS

    //
    // Miscellaneous lexing flags. Can be ORed together.
    //
    struct flags final
    {
        static constexpr std::uint32_t no_errors                     = 1 << 0;  // Don't generate any errors.
        static constexpr std::uint32_t no_warnings                   = 1 << 1;  // Don't generate any warnings.
        static constexpr std::uint32_t no_fatal_errors               = 1 << 2;  // Errors aren't fatal. By default all errors are fatal.
        static constexpr std::uint32_t no_string_concat              = 1 << 3;  // Multiple strings separated by whitespace are not concatenated.
        static constexpr std::uint32_t no_string_escape_chars        = 1 << 4;  // No escape characters allowed inside strings.
        static constexpr std::uint32_t allow_path_names              = 1 << 5;  // Allow path separators in names, e.g.: 'my/path/based/name'.
        static constexpr std::uint32_t allow_number_names            = 1 << 6;  // Allow names to start with a number, e.g.: '3lite'.
        static constexpr std::uint32_t allow_ip_addresses            = 1 << 7;  // Allow IP addresses to be parsed as numbers.
        static constexpr std::uint32_t allow_float_exceptions        = 1 << 8;  // Allow float exceptions like 1.#INF or 1.#IND to be parsed.
        static constexpr std::uint32_t allow_multi_char_literals     = 1 << 9;  // Allow multi-character literals.
        static constexpr std::uint32_t allow_backslash_string_concat = 1 << 10; // Allow multiple strings separated by '\' to be concatenated.
        static constexpr std::uint32_t only_strings                  = 1 << 11; // Scan as whitespace delimited strings (quoted strings keep quotes).
    }; // flags

    //
    // Lexer public interface:
    //

    // Not copyable.
    lexer(const lexer & other) = delete;
    lexer & operator = (const lexer & other) = delete;

    // But movable.
    lexer(lexer && other) noexcept;
    lexer & operator = (lexer && other) noexcept;

    // Empty and uninitialized lexer.
    lexer() = default;

    // Init by loading a file from the file system. Allocates memory for the file contents.
    lexer(std::string filename, std::uint32_t flags = 0);

    // Init with a memory buffer. Does not take ownership of the pointer.
    lexer(const char * ptr, std::uint32_t length, std::string filename,
          std::uint32_t flags = 0, std::uint32_t starting_line = 1);

    // Lexer might allocate memory if loading from file. The destructor cleans it up.
    ~lexer();

    // Load a script from the given file in the file system.
    // If 'silent' is true, no warnings or errors will be printed. The function might still fail and return false.
    bool init_from_file(std::string filename, std::uint32_t flags = 0, bool silent = false);

    // Load a script from the given memory buffer with given length and a specified line offset,
    // so source strings extracted from a file can still refer to proper line numbers in the file.
    // Note: 'ptr' is expected to point at a valid C string: ptr[length] == '\0'!
    // The lexer WILL NOT take ownership of the passed pointer. Caller is responsible for freeing it!
    bool init_from_memory(const char * ptr, std::uint32_t length, std::string filename,
                          std::uint32_t flags = 0, std::uint32_t starting_line = 1);

    // Frees the script, filename and any associated data. Flags stay the same.
    // Most internal states are reset to initials.
    void clear() noexcept;

    // Reset the lexer to the beginning of its text input.
    // This will also clear the error and warning counters. Flags stay the same.
    void reset() noexcept;

    // If the lexer has allocated dynamic memory for the script source, free it.
    // This will not clear the filename, flags or error/warn counts, so can still
    // use the lexer instance to print info about the script.
    void free_script_source() noexcept;

    // Lexer flags can be changed at any time during scanning.
    void set_flags(std::uint32_t new_flags) noexcept;

    // Changes the line number but doesn't alter the position within the scrip.
    void set_line_number(std::uint32_t new_line_num) noexcept;

    // Read the next token (or returns a cached token).
    // Returns false if no more tokens are available or any other errors occurred.
    bool next_token(token * out_token);

    // Read a token only if on the same line.
    bool next_token_on_line(token * out_token);

    // Unread the given token / put it back.
    void unget_token(const token & in_token);

    // Expect a certain token, reads the token if available, generates an error otherwise.
    bool expect_token_char(char c);
    bool expect_token_char(char c, token * out_token);
    bool expect_token_string(const char * string);
    bool expect_token_string(const char * string, token * out_token);

    // Expect a certain token type, e.g.: number, name/id, string, etc.
    bool expect_token_type(token::type type, std::uint32_t subtype_flags, token * out_token);
    bool expect_any_token(token * out_token);

    // Returns true if the token is available. Doesn't generate errors if not.
    bool check_token_string(const char * string);
    bool check_token_string(const char * string, token * out_token);

    // Returns true an reads the token when a token with the given type is available.
    bool check_token_type(token::type type, std::uint32_t subtype_flags, token * out_token);

    // Returns true if the next token equals the given string but does not remove the token from the source.
    bool peek_token_string(const char * string);

    // Returns true if the next token equals the given type but does not remove the token from the source.
    bool peek_token_type(token::type type, std::uint32_t subtype_flags, token * out_token);

    // Skip tokens until the given token string is read.
    bool skip_until_string(const char * string);

    // Skip the rest of the current line.
    bool skip_rest_of_line();

    // Skip a {} bracketed section.
    bool skip_bracketed_section(bool scan_first_bracket = true);

    // Skips spaces, tabs, C-style multi-line comments, C++ comments, etc.
    // Returns false if there is no token left to read.
    bool skip_whitespace(bool current_line);

    // Read a boolean token. 'true|false' and '0|1' qualify as booleans.
    // Calls lexer::error() on failure (which might throw) and returns false.
    bool scan_bool();

    // Read a floating-point token. Any signed or unsigned decimal values are supported.
    // Calls lexer::error() on failure (which might throw) and returns 0.0.
    double scan_double();
    float  scan_float();

    // Read an integer token. Signed and unsigned integer values are supported.
    // Calls lexer::error() on failure (which might throw) and returns 0. Warns and
    // truncates the result if the number value encountered was floating-point.
    std::uint64_t scan_uint64();
    std::int64_t  scan_int64();

    // Read a string enclosed in single or double quotes. Quotes not included in the output.
    // Calls lexer::error() on failure (which might throw) and returns an empty string.
    std::string scan_string();

    // Scans any signed or unsigned number, integer, float and also boolean literals.
    // Will select the appropriate method from the above based on the template type.
    template<typename NumType>
    NumType scan_number();

    // Scans matrices/nested-tuples of numbers.
    //
    // 1D matrix:
    // ( 1, 2, 3, ... )
    //
    // 2D matrix:
    // <1D matrices repeated a Y number of times>
    // ( ( 1, 2, 3, ... ), ( 1, 2, 3, ... ), ( 1, 2, 3, ... ), ... )
    //
    // 3D matrix:
    // <2D matrices repeated a Z number of times>
    // ( ( ( 1, 2, 3, ... ), ( 1, 2, 3, ... ), ( 1, 2, 3, ... ), ... ), ... )
    //
    template<typename NumType>
    bool scan_matrix1d(int x, NumType * out_mat,
                       const char * open_delim  = "(",
                       const char * close_delim = ")",
                       bool comma_separated_values = true);

    template<typename NumType>
    bool scan_matrix2d(int y, int x, NumType * out_mat,
                       const char * open_delim  = "(",
                       const char * close_delim = ")",
                       bool comma_separated_values = true);

    template<typename NumType>
    bool scan_matrix3d(int z, int y, int x, NumType * out_mat,
                       const char * open_delim  = "(",
                       const char * close_delim = ")",
                       bool comma_separated_values = true);

    // Read a {} bracketed section into a string.
    std::string scan_bracketed_section();

    // Read a {} bracketed section into a string, maintaining indents and newlines.
    std::string scan_bracketed_section_exact(int tabs = -1);

    // Read the rest of the line via tokenization.
    std::string scan_rest_of_line();

    // Pulls the entire line, including the '\n' at the end.
    std::string scan_complete_line();

    // Retrieves the whitespace characters before the last read token.
    std::string get_last_whitespace() const;

    // Returns the length in characters of whitespace before the last token read.
    std::size_t get_last_whitespace_length() const noexcept;

    // Returns start index into text buffer of last whitespace.
    std::size_t get_last_whitespace_start() const noexcept;

    // Returns end index into text buffer of last whitespace.
    std::size_t get_last_whitespace_end() const noexcept;

    // Error handing:
    bool error(const std::string & message);
    void warning(const std::string & message);

    // Miscellaneous queries:
    bool                is_initialized()      const noexcept;
    bool                is_at_end()           const noexcept;
    std::size_t         get_allocated_bytes() const noexcept;
    std::size_t         get_script_offset()   const noexcept;
    std::size_t         get_script_length()   const noexcept;
    std::uint32_t       get_flags()           const noexcept;
    std::uint32_t       get_line_number()     const noexcept;
    std::uint32_t       get_error_count()     const noexcept;
    std::uint32_t       get_warning_count()   const noexcept;
    const std::string & get_filename()        const noexcept;

    //
    // String / file utilities:
    //

    // Helper used internally to load text files for init_from_file().
    // Can also be used as a standalone way of loading text files into memory.
    // If successful, 'out_file_contents' points to a heap allocated
    // C string with the file contents. Caller must free it with delete[].
    static bool load_text_file(const std::string & filename, char ** out_file_contents, std::uint32_t * out_file_length);

    // Trim leading white-spaces (in-place).
    static std::string & ltrim_string(std::string * s);

    // Trim trailing white-spaces (in-place).
    static std::string & rtrim_string(std::string * s);

    // rtrim_string() + ltrim_string().
    static std::string & trim_string(std::string * s);

private:

    // Internal helpers:
    bool internal_read_whitespace();
    bool internal_read_escape_character(char * out_char);
    bool internal_read_string(int quote, token * out_token);
    bool internal_read_name_ident(token * out_token);
    bool internal_read_number(token * out_token);
    bool internal_read_punctuation(token * out_token);
    bool internal_check_string(const char * string) const;

    // Instance data:
    const char *                          m_buffer_head_ptr      = nullptr; // Buffer containing the script; owned by the lexer if m_allocated == true.
    const char *                          m_script_ptr           = nullptr; // Current pointer in the script.
    const char *                          m_end_ptr              = nullptr; // Pointer to the end of the script.
    const char *                          m_last_script_ptr      = nullptr; // Script pointer before reading last token.
    const char *                          m_whitespace_start_ptr = nullptr; // Start of last white space.
    const char *                          m_whitespace_end_ptr   = nullptr; // End pointer of last white space.
    std::uint32_t                         m_flags                = 0;       // lexer::flags ORed together or zero.
    std::uint32_t                         m_last_line_num        = 0;       // Line number before reading a token.
    std::uint32_t                         m_line_num             = 0;       // Current line in script.
    std::uint32_t                         m_script_length        = 0;       // Length of the script in characters, not counting a null terminator.
    std::uint32_t                         m_error_count          = 0;       // Bumped by lexer::error(), even if errors are suppressed.
    std::uint32_t                         m_warn_count           = 0;       // Bumped by lexer::warning(), even if warnings are suppressed.
    token                                 m_leftover_token       {};        // Available token from unget_token(). May be empty.
    std::string                           m_filename             {};        // Filename of the script being scanned. Used for error reporting.
    bool                                  m_token_available      = false;   // Set by unget_token() if m_leftover_token is available.
    bool                                  m_initialized          = false;   // Set when a script file is loaded from file or memory.
    bool                                  m_allocated            = false;   // True if dynamic memory was allocated. False if external.

    // Shared data:
    static error_callbacks              * m_error_callbacks;                // Error and warning reporting callbacks.
    static const punctuation_def        * m_punctuations;                   // The punctuations used in the script.
    static const punct_table_index_type * m_punctuations_table;             // ASCII table with punctuations (256 entries at least).
    static const punct_table_index_type * m_punctuations_next;              // Next punctuation in chain (same size of m_punctuations).
    static std::size_t                    m_punctuations_size;              // Size in entries of m_punctuations and m_punctuations_next.
};

// ========================================================
// token class inline methods:
// ========================================================

inline lexer::token::token(token && other) noexcept
    : m_string        { std::move(other.m_string) }
    , m_flags         { other.m_flags             }
    , m_line_num      { other.m_line_num          }
    , m_lines_crossed { other.m_lines_crossed     }
    , m_type          { other.m_type              }
    , m_values_valid  { other.m_values_valid      }
    , m_u64_value     { other.m_u64_value         }
    , m_double_value  { other.m_double_value      }
{
    other.clear();
}

inline lexer::token & lexer::token::operator = (token && other) noexcept
{
    m_string        = std::move(other.m_string);
    m_flags         = other.m_flags;
    m_line_num      = other.m_line_num;
    m_lines_crossed = other.m_lines_crossed;
    m_type          = other.m_type;
    m_values_valid  = other.m_values_valid;
    m_u64_value     = other.m_u64_value;
    m_double_value  = other.m_double_value;

    other.clear();
    return *this;
}

inline bool lexer::token::as_bool() const noexcept
{
    return get_value_u64() != 0;
}

inline float lexer::token::as_float() const noexcept
{
    return static_cast<float>(get_value_double());
}

inline double lexer::token::as_double() const noexcept
{
    return get_value_double();
}

inline std::int32_t lexer::token::as_int32() const noexcept
{
    return static_cast<std::int32_t>(get_value_u64());
}

inline std::int64_t lexer::token::as_int64() const noexcept
{
    return static_cast<std::int64_t>(get_value_u64());
}

inline std::uint32_t lexer::token::as_uint32() const noexcept
{
    return static_cast<std::uint32_t>(get_value_u64());
}

inline std::uint64_t lexer::token::as_uint64() const noexcept
{
    return get_value_u64();
}

inline const std::string & lexer::token::as_string() const noexcept
{
    return m_string;
}

inline bool lexer::token::is_number() const noexcept
{
    return m_type == type::number;
}

inline bool lexer::token::is_integer() const noexcept
{
    return m_flags & flags::integer;
}

inline bool lexer::token::is_float() const noexcept
{
    return m_flags & flags::floating_point;
}

inline bool lexer::token::is_boolean() const noexcept
{
    return m_flags & flags::boolean;
}

inline bool lexer::token::is_string() const noexcept
{
    return m_type == type::string;
}

inline bool lexer::token::is_literal() const noexcept
{
    return m_type == type::literal;
}

inline bool lexer::token::is_identifier() const noexcept
{
    return m_type == type::identifier;
}

inline bool lexer::token::is_punctuation() const noexcept
{
    return m_type == type::punctuation;
}

inline std::size_t lexer::token::get_length() const noexcept
{
    return m_string.length();
}

inline std::uint32_t lexer::token::get_flags() const noexcept
{
    return m_flags;
}

inline std::uint32_t lexer::token::get_line_number() const noexcept
{
    return m_line_num;
}

inline std::uint32_t lexer::token::get_lines_crossed() const noexcept
{
    return m_lines_crossed;
}

inline lexer::token::type lexer::token::get_type() const noexcept
{
    return m_type;
}

inline bool lexer::token::operator == (const char c) const noexcept
{
    return m_string.length() == 1 && m_string[0] == c;
}

inline bool lexer::token::operator != (const char c) const noexcept
{
    return m_string.length() == 1 && m_string[0] != c;
}

inline bool lexer::token::operator == (const char * str) const noexcept
{
    return m_string == str;
}

inline bool lexer::token::operator != (const char * str) const noexcept
{
    return m_string != str;
}

inline bool lexer::token::operator == (const std::string & str) const noexcept
{
    return m_string == str;
}

inline bool lexer::token::operator != (const std::string & str) const noexcept
{
    return m_string != str;
}

inline void lexer::token::set_string(std::string new_text)
{
    m_string = std::move(new_text);
    m_values_valid = false;
}

inline void lexer::token::set_flags(const std::uint32_t new_flags) noexcept
{
    m_flags = new_flags;
    m_values_valid = false;
}

inline void lexer::token::set_line_number(const std::uint32_t new_line_num) noexcept
{
    m_line_num = new_line_num;
}

inline void lexer::token::set_lines_crossed(const std::uint32_t new_lines_crossed) noexcept
{
    m_lines_crossed = new_lines_crossed;
}

inline void lexer::token::set_type(const type new_type) noexcept
{
    m_type = new_type;
    m_values_valid = false;
}

inline void lexer::token::append(const char c)
{
    if (c != '\0')
    {
        m_string.push_back(c);
        m_values_valid = false;
    }
}

inline void lexer::token::append(const char * str)
{
    if (str != nullptr && *str != '\0')
    {
        m_string.append(str);
        m_values_valid = false;
    }
}

inline char lexer::token::operator[](const int index) const
{
    assert(index >= 0 && static_cast<std::size_t>(index) < m_string.length());
    return m_string[index];
}

inline void lexer::token::move_to(std::string * dest_str) noexcept
{
    *dest_str = std::move(m_string);
    clear();
}

inline void lexer::token::clear() noexcept
{
    m_string.clear();
    m_flags         = 0;
    m_line_num      = 0;
    m_lines_crossed = 0;
    m_type          = type::none;
    m_values_valid  = true;
    m_u64_value     = 0;
    m_double_value  = 0.0;
}

inline std::uint64_t lexer::token::get_value_u64() const noexcept
{
    if (!is_number() && !is_boolean())
    {
        return 0;
    }
    if (!m_values_valid)
    {
        update_cached_values();
    }
    return m_u64_value;
}

inline double lexer::token::get_value_double() const noexcept
{
    if (!is_number() && !is_boolean())
    {
        return 0.0;
    }
    if (!m_values_valid)
    {
        update_cached_values();
    }
    return m_double_value;
}

// ========================================================
// Internal use helpers needed by the templates below:
// ========================================================

namespace lexer_detail
{

struct scan_bool_func final
{
    static bool do_scan(lexer * lex) { return lex->scan_bool(); }
};

template<typename T>
struct scan_sint_func final
{
    static T do_scan(lexer * lex) { return static_cast<T>(lex->scan_int64()); }
};

template<typename T>
struct scan_uint_func final
{
    static T do_scan(lexer * lex) { return static_cast<T>(lex->scan_uint64()); }
};

template<typename T>
struct scan_float_func final
{
    static T do_scan(lexer * lex) { return static_cast<T>(lex->scan_double()); }
};

// This is used by the matrix scanning methods.
inline bool ignore_trailing_comma(lexer * lex, const int i, const int num)
{
    lexer::token tok;
    if ((i + 1) != num) // Not the last value?
    {
        if (!lex->expect_token_type(lexer::token::type::punctuation,
            static_cast<std::uint32_t>(lexer::punctuation_id::comma), &tok))
        {
            return false;
        }
    }
    else // Last value, a trailing comma is ignored.
    {
        if (!lex->next_token(&tok))
        {
            return false;
        }
        if (!lexer::is_punctuation_token(tok, lexer::punctuation_id::comma))
        {
            lex->unget_token(tok);
        }
    }
    return true;
}

} // namespace lexer_detail {}

// ========================================================
// lexer class inline methods:
// ========================================================

inline void lexer::set_flags(const std::uint32_t new_flags) noexcept
{
    m_flags = new_flags;
}

inline void lexer::set_line_number(const std::uint32_t new_line_num) noexcept
{
    m_line_num      = new_line_num;
    m_last_line_num = new_line_num;
}

inline bool lexer::is_initialized() const noexcept
{
    return m_initialized && m_script_ptr != nullptr;
}

inline bool lexer::is_at_end() const noexcept
{
    return m_script_ptr >= m_end_ptr;
}

inline std::size_t lexer::get_allocated_bytes() const noexcept
{
    return m_allocated ? (m_script_length + 1) : 0;
}

inline std::size_t lexer::get_script_offset() const noexcept
{
    return static_cast<std::size_t>(m_script_ptr - m_buffer_head_ptr);
}

inline std::size_t lexer::get_script_length() const noexcept
{
    return m_script_length;
}

inline std::uint32_t lexer::get_flags() const noexcept
{
    return m_flags;
}

inline std::uint32_t lexer::get_line_number() const noexcept
{
    return m_line_num;
}

inline std::uint32_t lexer::get_error_count() const noexcept
{
    return m_error_count;
}

inline std::uint32_t lexer::get_warning_count() const noexcept
{
    return m_warn_count;
}

inline const std::string & lexer::get_filename() const noexcept
{
    return m_filename;
}

inline std::size_t lexer::get_last_whitespace_length() const noexcept
{
    return static_cast<std::size_t>(m_whitespace_end_ptr - m_whitespace_start_ptr);
}

inline std::size_t lexer::get_last_whitespace_start() const noexcept
{
    return static_cast<std::size_t>(m_whitespace_start_ptr - m_buffer_head_ptr);
}

inline std::size_t lexer::get_last_whitespace_end() const noexcept
{
    return static_cast<std::size_t>(m_whitespace_end_ptr - m_buffer_head_ptr);
}

inline float lexer::scan_float()
{
    return static_cast<float>(scan_double());
}

template<typename NumType>
inline NumType lexer::scan_number()
{
    static_assert(std::is_floating_point<NumType>::value || std::is_integral<NumType>::value,
                  "Floating-point or integer type required!");

    using scan_sint_or_uint = typename std::conditional
                              <
                                  std::is_signed<NumType>::value,
                                  lexer_detail::scan_sint_func<NumType>,
                                  lexer_detail::scan_uint_func<NumType>
                              >::type;

    using scan_int_or_bool = typename std::conditional
                              <
                                  std::is_same<NumType, bool>::value,
                                  lexer_detail::scan_bool_func,
                                  scan_sint_or_uint
                              >::type;

    using scan_func = typename std::conditional
                              <
                                  std::is_floating_point<NumType>::value,
                                  lexer_detail::scan_float_func<NumType>,
                                  scan_int_or_bool
                              >::type;

    return scan_func::do_scan(this);
}

template<typename NumType>
inline bool lexer::scan_matrix1d(const int x, NumType * out_mat,
                                 const char * open_delim, const char * close_delim,
                                 const bool comma_separated_values)
{
    static_assert(std::is_floating_point<NumType>::value || std::is_integral<NumType>::value,
                  "Floating-point or integer type required!");

    assert(out_mat != nullptr);

    if (!expect_token_string(open_delim))
    {
        return false;
    }

    for (int i = 0; i < x; ++i)
    {
        out_mat[i] = scan_number<NumType>();
        if (comma_separated_values && !lexer_detail::ignore_trailing_comma(this, i, x))
        {
            return false;
        }
    }

    if (!expect_token_string(close_delim))
    {
        return false;
    }

    return true;
}

template<typename NumType>
inline bool lexer::scan_matrix2d(const int y, const int x, NumType * out_mat,
                                 const char * open_delim, const char * close_delim,
                                 const bool comma_separated_values)
{
    static_assert(std::is_floating_point<NumType>::value || std::is_integral<NumType>::value,
                  "Floating-point or integer type required!");

    assert(out_mat != nullptr);

    if (!expect_token_string(open_delim))
    {
        return false;
    }

    for (int i = 0; i < y; ++i)
    {
        if (!scan_matrix1d(x, (out_mat + i * x), open_delim, close_delim, comma_separated_values))
        {
            return false;
        }
        if (comma_separated_values && !lexer_detail::ignore_trailing_comma(this, i, y))
        {
            return false;
        }
    }

    if (!expect_token_string(close_delim))
    {
        return false;
    }

    return true;
}

template<typename NumType>
inline bool lexer::scan_matrix3d(const int z, const int y, const int x, NumType * out_mat,
                                 const char * open_delim, const char * close_delim,
                                 const bool comma_separated_values)
{
    static_assert(std::is_floating_point<NumType>::value || std::is_integral<NumType>::value,
                  "Floating-point or integer type required!");

    assert(out_mat != nullptr);

    if (!expect_token_string(open_delim))
    {
        return false;
    }

    for (int i = 0; i < z; ++i)
    {
        if (!scan_matrix2d(y, x, (out_mat + i * x * y), open_delim, close_delim, comma_separated_values))
        {
            return false;
        }
        if (comma_separated_values && !lexer_detail::ignore_trailing_comma(this, i, z))
        {
            return false;
        }
    }

    if (!expect_token_string(close_delim))
    {
        return false;
    }

    return true;
}

inline bool lexer::is_punctuation_token(const token & tok, const punctuation_id id) noexcept
{
    // When a punctuation token, the flags will store the punctuation_id converted to integer.
    return tok.is_punctuation() && (tok.get_flags() == static_cast<std::uint32_t>(id));
}

// ================== End of header file ==================
#endif // LEXER_HPP
// ================== End of header file ==================

// ================================================================================================
//
//                                    Lexer Implementation
//
// ================================================================================================

#ifdef LEXER_IMPLEMENTATION

#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>

// ========================================================
// token class:
// ========================================================

void lexer::token::update_cached_values() const noexcept
{
    const char * p = m_string.c_str();
    std::uint64_t new_u64_val = 0;
    double new_double_val = 0.0;

    if (m_flags & flags::floating_point)
    {
        // Floating point exceptions:
        if (m_flags & (flags::infinite | flags::indefinite | flags::nan))
        {
            if (m_flags & flags::infinite) // 1.#INF
            {
                const std::uint32_t inf = 0x7F800000;
                new_double_val = static_cast<double>(*reinterpret_cast<const float *>(&inf));
            }
            else if (m_flags & flags::indefinite) // 1.#IND
            {
                const std::uint32_t ind = 0xFFC00000;
                new_double_val = static_cast<double>(*reinterpret_cast<const float *>(&ind));
            }
            else if (m_flags & flags::nan) // 1.#NAN
            {
                const std::uint32_t nan = 0x7FC00000;
                new_double_val = static_cast<double>(*reinterpret_cast<const float *>(&nan));
            }
        }
        else
        {
            double m;

            while (*p && *p != '.' && *p != 'e')
            {
                new_double_val = new_double_val * 10.0 + (static_cast<double>(*p - '0'));
                ++p;
            }

            if (*p == '.')
            {
                ++p;
                for (m = 0.1; *p && *p != 'e'; ++p)
                {
                    new_double_val = new_double_val + (static_cast<double>(*p - '0')) * m;
                    m *= 0.1;
                }
            }

            if (*p == 'e')
            {
                bool div;

                ++p;
                if (*p == '-')
                {
                    div = true;
                    ++p;
                }
                else if (*p == '+')
                {
                    div = false;
                    ++p;
                }
                else
                {
                    div = false;
                }

                int i, pow;
                for (pow = 0; *p; ++p)
                {
                    pow = pow * 10 + (static_cast<int>(*p - '0'));
                }
                for (m = 1.0, i = 0; i < pow; ++i)
                {
                    m *= 10.0;
                }

                if (div)
                {
                    new_double_val /= m;
                }
                else
                {
                    new_double_val *= m;
                }
            }
        }
        new_u64_val = static_cast<std::uint64_t>(new_double_val);
    }
    else if (m_flags & flags::decimal) // Decimal integer number
    {
        while (*p != '\0')
        {
            new_u64_val = new_u64_val * 10 + (*p - '0');
            ++p;
        }
        new_double_val = new_u64_val;
    }
    else if (m_flags & flags::octal) // Octal integer
    {
        ++p; // Step over the first zero
        while (*p != '\0')
        {
            new_u64_val = (new_u64_val << 3) + (*p - '0');
            ++p;
        }
        new_double_val = new_u64_val;
    }
    else if (m_flags & flags::hexadecimal) // Hexadecimal integer
    {
        p += 2; // Step over the leading 0x or 0X
        while (*p != '\0')
        {
            new_u64_val <<= 4;
            if (*p >= 'a' && *p <= 'f')
            {
                new_u64_val += *p - 'a' + 10;
            }
            else if (*p >= 'A' && *p <= 'F')
            {
                new_u64_val += *p - 'A' + 10;
            }
            else
            {
                new_u64_val += *p - '0';
            }
            ++p;
        }
        new_double_val = new_u64_val;
    }
    else if (m_flags & flags::binary) // Binary integer number
    {
        p += 2; // Step over the leading 0b or 0B
        while (*p != '\0')
        {
            new_u64_val = (new_u64_val << 1) + (*p - '0');
            ++p;
        }
        new_double_val = new_u64_val;
    }
    else if (m_flags & flags::ip_address)
    {
        std::uint32_t ip[5] = {0}; // 4 octets + port number

        for (int i = 0; *p != '\0'; ++p)
        {
            if (*p == '.' || *p == ':')
            {
                ++i;
                continue;
            }
            ip[i] = ip[i] * 10 + (*p - '0');
        }

        const std::uint64_t ip_word   = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
        const std::uint64_t port_word = ip[4];

        new_u64_val    = (port_word << 32) | ip_word;
        new_double_val = new_u64_val;
    }
    else if (m_flags & flags::boolean) // Boolean literal
    {
        if (std::strcmp(p, "true") == 0)
        {
            new_u64_val = 1;
        }
        new_double_val = new_u64_val;
    }

    // Save the newly computed values and set the ready flag:
    m_u64_value    = new_u64_val;
    m_double_value = new_double_val;
    m_values_valid = true;
}

lexer::token lexer::token::stringize() const
{
    // Flags the token as a string for the C preprocessor stringizing operator (#)
    // and adds double quotes as needed.
    token str_token;

    str_token.m_type          = type::string;
    str_token.m_line_num      = m_line_num;
    str_token.m_lines_crossed = m_lines_crossed;

    if (!m_string.empty() && m_string[0] == '"')
    {
        str_token.m_string.push_back('"');
        str_token.m_string.push_back('\\');
        str_token.m_string.append(m_string);
        lexer::rtrim_string(&str_token.m_string);
        str_token.m_string.back() = '\\';
        str_token.m_string.push_back('"');
        str_token.m_string.push_back('"');
    }
    else
    {
        str_token.m_string.push_back('"');
        str_token.m_string.append(m_string);
        lexer::rtrim_string(&str_token.m_string);
        str_token.m_string.push_back('"');
    }

    return str_token;
}

lexer::token lexer::token::trim() const
{
    token t{ *this };
    lexer::trim_string(&t.m_string);
    return t;
}

std::string lexer::token::type_string(const token::type type)
{
    std::string out;
    switch (type)
    {
    case token::type::number      : { out = "number";      break; }
    case token::type::string      : { out = "string";      break; }
    case token::type::literal     : { out = "literal";     break; }
    case token::type::identifier  : { out = "identifier";  break; }
    case token::type::punctuation : { out = "punctuation"; break; }
    default                       : { out = "(unknown)";   break; }
    } // switch (type)
    return out;
}

std::string lexer::token::flags_string(const std::uint32_t flags, const bool is_punct)
{
    std::string out;

    // When a punctuation token, we actually steal the flags to store the punctuation_id!
    if (is_punct)
    {
        out = lexer::get_punctuation_from_id(static_cast<punctuation_id>(flags));
        return out;
    }

    if (flags & token::flags::infinite)           { out += "infinite ";           }
    if (flags & token::flags::indefinite)         { out += "indefinite ";         }
    if (flags & token::flags::nan)                { out += "nan ";                }
    if (flags & token::flags::binary)             { out += "binary ";             }
    if (flags & token::flags::octal)              { out += "octal ";              }
    if (flags & token::flags::decimal)            { out += "decimal ";            }
    if (flags & token::flags::hexadecimal)        { out += "hexadecimal ";        }
    if (flags & token::flags::signed_integer)     { out += "signed ";             }
    if (flags & token::flags::unsigned_integer)   { out += "unsigned ";           }
    if (flags & token::flags::single_precision)   { out += "single-precision ";   }
    if (flags & token::flags::double_precision)   { out += "double-precision ";   }
    if (flags & token::flags::extended_precision) { out += "extended-precision "; }
    if (flags & token::flags::integer)            { out += "integer ";            }
    if (flags & token::flags::floating_point)     { out += "float ";              }
    if (flags & token::flags::boolean)            { out += "boolean ";            }
    if (flags & token::flags::ip_address)         { out += "IP address ";         }
    if (flags & token::flags::ip_port)            { out += "IP port ";            }

    lexer::rtrim_string(&out);
    return out;
}

// ========================================================
// lexer class:
// ========================================================

lexer::lexer(lexer && other) noexcept
    : m_buffer_head_ptr      { other.m_buffer_head_ptr           }
    , m_script_ptr           { other.m_script_ptr                }
    , m_end_ptr              { other.m_end_ptr                   }
    , m_last_script_ptr      { other.m_last_script_ptr           }
    , m_whitespace_start_ptr { other.m_whitespace_start_ptr      }
    , m_whitespace_end_ptr   { other.m_whitespace_end_ptr        }
    , m_flags                { other.m_flags                     }
    , m_last_line_num        { other.m_last_line_num             }
    , m_line_num             { other.m_line_num                  }
    , m_script_length        { other.m_script_length             }
    , m_error_count          { other.m_error_count               }
    , m_warn_count           { other.m_warn_count                }
    , m_leftover_token       { std::move(other.m_leftover_token) }
    , m_filename             { std::move(other.m_filename)       }
    , m_token_available      { other.m_token_available           }
    , m_initialized          { other.m_initialized               }
    , m_allocated            { other.m_allocated                 }
{
    other.m_buffer_head_ptr = nullptr;
    other.m_allocated       = false;
    other.clear();
}

lexer & lexer::operator = (lexer && other) noexcept
{
    m_buffer_head_ptr      = other.m_buffer_head_ptr;
    m_script_ptr           = other.m_script_ptr;
    m_end_ptr              = other.m_end_ptr;
    m_last_script_ptr      = other.m_last_script_ptr;
    m_whitespace_start_ptr = other.m_whitespace_start_ptr;
    m_whitespace_end_ptr   = other.m_whitespace_end_ptr;
    m_flags                = other.m_flags;
    m_last_line_num        = other.m_last_line_num;
    m_line_num             = other.m_line_num;
    m_script_length        = other.m_script_length;
    m_error_count          = other.m_error_count;
    m_warn_count           = other.m_warn_count;
    m_leftover_token       = std::move(other.m_leftover_token);
    m_filename             = std::move(other.m_filename);
    m_token_available      = other.m_token_available;
    m_initialized          = other.m_initialized;
    m_allocated            = other.m_allocated;

    other.m_buffer_head_ptr = nullptr;
    other.m_allocated       = false;
    other.clear();

    return *this;
}

lexer::lexer(std::string filename, const std::uint32_t flags)
{
    init_from_file(std::move(filename), flags);
}

lexer::lexer(const char * ptr, const std::uint32_t length, std::string filename,
             const std::uint32_t flags, const std::uint32_t starting_line)
{
    init_from_memory(ptr, length, std::move(filename), flags, starting_line);
}

lexer::~lexer()
{
    if (m_allocated && m_buffer_head_ptr != nullptr)
    {
        delete[] m_buffer_head_ptr;
    }
}

bool lexer::init_from_file(std::string filename, const std::uint32_t flags, const bool silent)
{
    if (filename.empty())
    {
        return (!silent ? error("lexer::init_from_file() -> no filename provided!") : false);
    }

    if (m_initialized)
    {
        return (!silent ? error("lexer::init_from_file() -> another script is already loaded!") : false);
    }

    char * file_contents;
    std::uint32_t file_length;

    if (!load_text_file(filename, &file_contents, &file_length))
    {
        return (!silent ? error("lexer::init_from_file() -> failed to load text file \"" + filename + "\".") : false);
    }

    m_filename        = std::move(filename);
    m_buffer_head_ptr = file_contents;
    m_script_length   = file_length;
    m_script_ptr      = m_buffer_head_ptr;
    m_last_script_ptr = m_buffer_head_ptr;
    m_end_ptr         = &m_buffer_head_ptr[file_length];
    m_line_num        = 1;
    m_last_line_num   = 1;
    m_flags           = flags;
    m_allocated       = true;
    m_initialized     = true;

    return true;
}

bool lexer::init_from_memory(const char * ptr, const std::uint32_t length, std::string filename,
                             const std::uint32_t flags, const std::uint32_t starting_line)
{
    assert(ptr != nullptr);

    if (m_initialized)
    {
        return error("lexer::init_from_memory() -> another script is already loaded!");
    }

    m_filename = std::move(filename);
    if (m_filename.empty())
    {
        m_filename = "(memory)";
    }

    // Note that in this case, we DO NOT take ownership of the input buffer!
    m_buffer_head_ptr = ptr;
    m_script_length   = length;
    m_script_ptr      = m_buffer_head_ptr;
    m_last_script_ptr = m_buffer_head_ptr;
    m_end_ptr         = &m_buffer_head_ptr[length];
    m_line_num        = starting_line;
    m_last_line_num   = starting_line;
    m_flags           = flags;
    m_allocated       = false;
    m_initialized     = true;

    return true;
}

void lexer::clear() noexcept
{
    free_script_source();
    m_error_count = 0;
    m_warn_count  = 0;
    m_filename.clear();
    // Note: m_flags remain unchanged.
}

void lexer::reset() noexcept
{
    m_script_ptr           = m_buffer_head_ptr;
    m_last_script_ptr      = m_buffer_head_ptr;
    m_whitespace_start_ptr = nullptr;
    m_whitespace_end_ptr   = nullptr;
    m_last_line_num        = 1;
    m_line_num             = 1;
    m_error_count          = 0;
    m_warn_count           = 0;
    m_token_available      = false;

    m_leftover_token.clear();
}

void lexer::free_script_source() noexcept
{
    if (m_allocated && m_buffer_head_ptr != nullptr)
    {
        delete[] m_buffer_head_ptr;
    }

    m_buffer_head_ptr      = nullptr;
    m_script_ptr           = nullptr;
    m_end_ptr              = nullptr;
    m_last_script_ptr      = nullptr;
    m_whitespace_start_ptr = nullptr;
    m_whitespace_end_ptr   = nullptr;
    m_last_line_num        = 0;
    m_line_num             = 0;
    m_script_length        = 0;
    m_token_available      = false;
    m_initialized          = false;
    m_allocated            = false;

    m_leftover_token.clear();
}

bool lexer::error(const std::string & message)
{
    ++m_error_count;
    if (m_flags & flags::no_errors)
    {
        return false;
    }

    // When using the ANSI color codes the "error" tag prints in red.
#ifdef LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
    const std::string err_tag = "\033[31;1m error: \033[0;1m";
#else // !LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
    const std::string err_tag = " error: ";
#endif // LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES

    const std::string error_str = get_filename() + "(" + std::to_string(m_last_line_num) + "):" + err_tag + message;
    const bool is_fatal_error = !(m_flags & flags::no_fatal_errors);
    m_error_callbacks->error(error_str, is_fatal_error);

    // Always returns false so we can write 'return error("foobar");' on methods returning boolean.
    return false;
}

void lexer::warning(const std::string & message)
{
    ++m_warn_count;
    if (m_flags & flags::no_warnings)
    {
        return;
    }

    // When using the ANSI color codes the "warning" tag prints in magenta.
#ifdef LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
    const std::string warn_tag = "\033[35;1m warning: \033[0;1m";
#else // !LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES
    const std::string warn_tag = " warning: ";
#endif // LEXER_ERROR_WARN_USE_ANSI_COLOR_CODES

    const std::string warn_str = get_filename() + "(" + std::to_string(m_last_line_num) + "):" + warn_tag + message;
    m_error_callbacks->warning(warn_str);
}

bool lexer::load_text_file(const std::string & filename, char ** out_file_contents, std::uint32_t * out_file_length)
{
    assert(!filename.empty());
    assert(out_file_contents != nullptr);
    assert(out_file_length   != nullptr);

    FILE * file_in;

#ifdef _MSC_VER
    if (fopen_s(&file_in, filename.c_str(), "rb") != 0)
    {
        return false;
    }
#else // !_MSC_VER
    if ((file_in = std::fopen(filename.c_str(), "rb")) == nullptr)
    {
        return false;
    }
#endif // _MSC_VER

    std::fseek(file_in, 0, SEEK_END);
    const auto file_length = std::ftell(file_in);
    std::fseek(file_in, 0, SEEK_SET);

    if (file_length <= 0 || std::ferror(file_in))
    {
        std::fclose(file_in);
        return false;
    }

    auto file_contents = new char[file_length + 1];
    if (std::fread(file_contents, 1, file_length, file_in) != static_cast<std::size_t>(file_length))
    {
        delete[] file_contents;
        std::fclose(file_in);
        return false;
    }

    file_contents[file_length] = '\0';
    std::fclose(file_in);

    *out_file_contents = file_contents;
    *out_file_length   = file_length;
    return true;
}

bool lexer::next_token(token * out_token)
{
    assert(out_token != nullptr);

    if (!is_initialized())
    {
        return error("lexer not properly initialized; no script loaded!");
    }

    // If there is a token available (from unget_token)...
    if (m_token_available)
    {
        *out_token = std::move(m_leftover_token);
        m_token_available = false;
        return true;
    }

    // Save script & line pointers:
    m_last_line_num        = m_line_num;
    m_last_script_ptr      = m_script_ptr;
    m_whitespace_start_ptr = m_script_ptr;

    if (!internal_read_whitespace())
    {
        return false;
    }

    m_whitespace_end_ptr = m_script_ptr;

    out_token->clear();                                         // Ensure it is cleared
    out_token->set_line_number(m_line_num);                     // Line the token is on
    out_token->set_lines_crossed(m_line_num - m_last_line_num); // # of lines crossed before token

    int c = *m_script_ptr;

    // If we're keeping everything as whitespace delimited strings...
    if (m_flags & flags::only_strings)
    {
        // If there is a leading quote or double-quote:
        if (c == '\'' || c == '"')
        {
            if (!internal_read_string(c, out_token))
            {
                return false;
            }
        }
        else if (!internal_read_name_ident(out_token))
        {
            return false;
        }
    }
    // If there is a number...
    else if ((c >= '0' && c <= '9') ||
             (c == '.' && (*(m_script_ptr + 1) >= '0' && *(m_script_ptr + 1) <= '9')))
    {
        if (!internal_read_number(out_token))
        {
            return false;
        }

        // If names are allowed to start with a number:
        if (m_flags & flags::allow_number_names)
        {
            c = *m_script_ptr;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
            {
                if (!internal_read_name_ident(out_token))
                {
                    return false;
                }
            }
        }
    }
    // If there is a leading (double) quote...
    else if (c == '\'' || c == '"')
    {
        if (!internal_read_string(c, out_token))
        {
            return false;
        }
    }
    // If there is a name/identifier...
    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
    {
        if (!internal_read_name_ident(out_token))
        {
            return false;
        }
    }
    // Names may also start with a slash or dot when pathnames are allowed...
    else if ((m_flags & flags::allow_path_names) && ((c == '/' || c == '\\') || c == '.'))
    {
        if (!internal_read_name_ident(out_token))
        {
            return false;
        }
    }
    // Finally, check for punctuations:
    else if (!internal_read_punctuation(out_token))
    {
        return error("unknown punctuation character \'" + std::string(1, c) + "\'");
    }

    // Successfully read a token.
    return true;
}

bool lexer::next_token_on_line(token * out_token)
{
    assert(out_token != nullptr);

    token tok;
    if (!next_token(&tok))
    {
        m_script_ptr = m_last_script_ptr;
        m_line_num   = m_last_line_num;
        return false;
    }

    // If no lines were crossed before this token, OK.
    if (tok.get_lines_crossed() == 0)
    {
        *out_token = std::move(tok);
        return true;
    }

    // Restore our previous position:
    m_script_ptr = m_last_script_ptr;
    m_line_num   = m_last_line_num;
    out_token->clear();
    return false;
}

bool lexer::expect_token_char(const char c)
{
    token tok;
    return expect_token_char(c, &tok);
}

bool lexer::expect_token_char(const char c, token * out_token)
{
    assert(out_token != nullptr);

    if (!next_token(out_token))
    {
        return error("couldn't find expected token \'" + std::string(1, c) + "\'");
    }
    if (*out_token != c)
    {
        return error("expected \'" + std::string(1, c) + "\' but found \'" + out_token->as_string() + "\'");
    }
    return true;
}

bool lexer::expect_token_string(const char * const string)
{
    token tok;
    return expect_token_string(string, &tok);
}

bool lexer::expect_token_string(const char * const string, token * out_token)
{
    assert(string    != nullptr);
    assert(out_token != nullptr);

    if (!next_token(out_token))
    {
        return error("couldn't find expected token \'" + std::string(string) + "\'");
    }
    if (*out_token != string)
    {
        return error("expected \'" + std::string(string) + "\' but found \'" + out_token->as_string() + "\'");
    }
    return true;
}

bool lexer::expect_token_type(const token::type type, const std::uint32_t subtype_flags, token * out_token)
{
    assert(out_token != nullptr);

    if (!next_token(out_token))
    {
        return error("couldn't read expected token!");
    }

    if (out_token->get_type() != type)
    {
        return error("expected a " + token::type_string(type) + " but found \'" + out_token->as_string() + "\'");
    }

    if (out_token->get_type() == token::type::number)
    {
        if ((out_token->get_flags() & subtype_flags) != subtype_flags)
        {
            auto str = token::flags_string(subtype_flags);
            if (str.empty())
            {
                str = "number";
            }
            return error("expected " + str + " but found \'" + out_token->as_string() + "\'");
        }
    }
    else if (out_token->get_type() == token::type::punctuation)
    {
        // subtype_flags == some punctuation_id
        if (subtype_flags >= m_punctuations_size)
        {
            return error("bad punctuation index in subtype_flags!");
        }

        if (out_token->get_flags() != subtype_flags)
        {
            auto str = get_punctuation_from_id(static_cast<punctuation_id>(subtype_flags));
            return error("expected \'" + str + "\' but found \'" + out_token->as_string() + "\'");
        }
    }

    return true;
}

bool lexer::expect_any_token(token * out_token)
{
    assert(out_token != nullptr);
    if (!next_token(out_token))
    {
        return error("couldn't read expected token!");
    }
    return true;
}

bool lexer::check_token_string(const char * const string)
{
    token tok;
    return check_token_string(string, &tok);
}

bool lexer::check_token_string(const char * const string, token * out_token)
{
    assert(string    != nullptr);
    assert(out_token != nullptr);

    if (!next_token(out_token))
    {
        return false;
    }

    // If the given string is available:
    if (*out_token == string)
    {
        return true;
    }

    // Unread the token:
    m_script_ptr = m_last_script_ptr;
    m_line_num   = m_last_line_num;
    return false;

}

bool lexer::check_token_type(const token::type type, const std::uint32_t subtype_flags, token * out_token)
{
    assert(out_token != nullptr);

    token tok;
    if (!next_token(&tok))
    {
        return false;
    }

    if ((tok.get_type() == type) && ((tok.get_flags() & subtype_flags) == subtype_flags))
    {
        *out_token = std::move(tok);
        return true;
    }

    // Unread the token:
    m_script_ptr = m_last_script_ptr;
    m_line_num   = m_last_line_num;
    return false;
}

bool lexer::peek_token_string(const char * const string)
{
    assert(string != nullptr);

    token tok;
    if (!next_token(&tok))
    {
        return false;
    }

    // Unread the token:
    m_script_ptr = m_last_script_ptr;
    m_line_num   = m_last_line_num;

    if (tok == string)
    {
        return true;
    }

    return false;
}

bool lexer::peek_token_type(const token::type type, const std::uint32_t subtype_flags, token * out_token)
{
    assert(out_token != nullptr);

    token tok;
    if (!next_token(&tok))
    {
        return false;
    }

    // Unread the token:
    m_script_ptr = m_last_script_ptr;
    m_line_num   = m_last_line_num;

    if ((tok.get_type() == type) && ((tok.get_flags() & subtype_flags) == subtype_flags))
    {
        *out_token = std::move(tok);
        return true;
    }

    return false;
}

bool lexer::skip_until_string(const char * const string)
{
    assert(string != nullptr);

    token tok;
    while (next_token(&tok))
    {
        if (tok == string)
        {
            return true;
        }
    }
    return false;
}

bool lexer::skip_rest_of_line()
{
    token tok;
    while (next_token(&tok))
    {
        if (tok.get_lines_crossed() != 0)
        {
            m_script_ptr = m_last_script_ptr;
            m_line_num   = m_last_line_num;
            return true;
        }
    }
    return false;
}

bool lexer::skip_bracketed_section(const bool scan_first_bracket)
{
    // Skips until a matching close curly bracket is found.
    // Internal bracket depths are properly skipped.

    token tok;
    int depth = (scan_first_bracket ? 0 : 1);

    do
    {
        if (!next_token(&tok))
        {
            return false;
        }

        if (tok.get_type() == token::type::punctuation)
        {
            if (is_punctuation_token(tok, punctuation_id::open_curly_bracket))
            {
                ++depth;
            }
            else if (is_punctuation_token(tok, punctuation_id::close_curly_bracket))
            {
                --depth;
            }
        }
    }
    while (depth);

    return true;
}

bool lexer::skip_whitespace(const bool current_line)
{
    for (;;)
    {
        assert(m_script_ptr <= m_end_ptr);
        if (m_script_ptr == m_end_ptr)
        {
            return false;
        }

        // Skip whitespace:
        while (*m_script_ptr <= ' ')
        {
            if (m_script_ptr == m_end_ptr)
            {
                return false;
            }
            if (!*m_script_ptr)
            {
                return false;
            }
            if (*m_script_ptr == '\n')
            {
                ++m_line_num;
                if (current_line)
                {
                    ++m_script_ptr;
                    return true;
                }
            }
            ++m_script_ptr;
        }

        // Skip comments:
        if (*m_script_ptr == '/')
        {
            // C++-style comments:
            if (*(m_script_ptr + 1) == '/')
            {
                ++m_script_ptr;
                do
                {
                    ++m_script_ptr;
                    if (!*m_script_ptr)
                    {
                        return false;
                    }
                }
                while (*m_script_ptr != '\n');

                ++m_line_num;
                ++m_script_ptr;

                if (current_line)
                {
                    return true;
                }
                if (!*m_script_ptr)
                {
                    return false;
                }
                continue;
            }
            // C-style/multi-line comments:
            else if (*(m_script_ptr + 1) == '*')
            {
                ++m_script_ptr;
                for (;;)
                {
                    ++m_script_ptr;
                    if (!*m_script_ptr)
                    {
                        return false;
                    }
                    if (*m_script_ptr == '\n')
                    {
                        ++m_line_num;
                    }
                    else if (*m_script_ptr == '/')
                    {
                        if (*(m_script_ptr - 1) == '*')
                        {
                            break;
                        }
                        if (*(m_script_ptr + 1) == '*')
                        {
                            warning("nested C-style multi-line comment!");
                        }
                    }
                }

                ++m_script_ptr;
                if (!*m_script_ptr)
                {
                    return false;
                }
                continue;
            }
        }
        break;
    }
    return true;
}

void lexer::unget_token(const token & in_token)
{
    if (m_token_available)
    {
        warning("lexer::unget_token() called twice in a row!");
    }

    m_leftover_token  = in_token;
    m_token_available = true;
}

bool lexer::scan_bool()
{
    token tok;
    if (!next_token(&tok))
    {
        return error("couldn't read expected boolean literal!");
    }

    if (!tok.is_boolean() && !tok.is_number())
    {
        return error("expected boolean literal or number, found \'" + tok.as_string() + "\'.");
    }

    if (tok.as_uint64() > 1)
    {
        warning("expected 0 or 1 for numerical boolean literal!");
    }

    return tok.as_bool();
}

double lexer::scan_double()
{
    token tok;
    if (!next_token(&tok))
    {
        error("couldn't read expected floating-point number!");
        return 0.0;
    }

    // These are numbers but cannot be scanned as float/double.
    constexpr auto bad_flags = (token::flags::binary      |
                                token::flags::octal       |
                                token::flags::hexadecimal |
                                token::flags::ip_address  |
                                token::flags::ip_port);

    if ((tok.get_type() == token::type::number) && (tok.get_flags() & bad_flags))
    {
        error("number format cannot be scanned as a floating-point value!");
        return 0.0;
    }

    if (tok.get_type() == token::type::punctuation && tok == '-') // negative number
    {
        if (!expect_token_type(token::type::number, 0, &tok))
        {
            return 0.0;
        }

        if (tok.get_flags() & bad_flags)
        {
            error("number format cannot be scanned as a floating-point value!");
            return 0.0;
        }

        return -tok.as_double();
    }
    else if (tok.get_type() != token::type::number) // invalid
    {
        error("expected float value, found \'" + tok.as_string() + "\'.");
        return 0.0;
    }
    else // valid positive number
    {
        return tok.as_double();
    }
}

std::uint64_t lexer::scan_uint64()
{
    token tok;
    if (!next_token(&tok))
    {
        error("couldn't read expected unsigned integer number!");
        return 0;
    }

    if (tok.is_float())
    {
        warning("expected unsigned integer number, got float; truncating it to integer...");
    }

    if (tok.get_type() == token::type::punctuation && tok == '-') // negative number
    {
        warning("expected unsigned integer number, got a negative value instead!");

        if (!expect_token_type(token::type::number, 0, &tok))
        {
            return 0;
        }

        if (tok.is_float())
        {
            warning("expected unsigned integer number, got float; truncating it to integer...");
        }

        return static_cast<std::uint64_t>(-tok.as_int64());
    }
    else if (tok.get_type() != token::type::number) // invalid
    {
        error("expected unsigned integer value, found \'" + tok.as_string() + "\'.");
        return 0;
    }
    else // valid positive number
    {
        return tok.as_uint64();
    }
}

std::int64_t lexer::scan_int64()
{
    token tok;
    if (!next_token(&tok))
    {
        error("couldn't read expected integer number!");
        return 0;
    }

    if (tok.is_float())
    {
        warning("expected integer number, got float; truncating it to integer...");
    }

    if (tok.get_type() == token::type::punctuation && tok == '-') // negative number
    {
        if (!expect_token_type(token::type::number, 0, &tok))
        {
            return 0;
        }

        if (tok.is_float())
        {
            warning("expected integer number, got float; truncating it to integer...");
        }

        return -tok.as_int64();
    }
    else if (tok.get_type() != token::type::number) // invalid
    {
        error("expected integer value, found \'" + tok.as_string() + "\'.");
        return 0;
    }
    else // valid positive number
    {
        return tok.as_int64();
    }
}

std::string lexer::scan_string()
{
    token tok;
    std::string out;

    if (!next_token(&tok))
    {
        error("couldn't read expected string!");
        return out;
    }

    if (tok.get_type() != token::type::string &&
        tok.get_type() != token::type::literal)
    {
        error("expected string or character literal, found \'" + tok.as_string() + "\'.");
        return out;
    }

    tok.move_to(&out);
    return out;
}

std::string lexer::scan_bracketed_section()
{
    // The next token should be an open curly bracket.
    // Scans until a matching close bracket is found.
    // Internal bracket depths are properly skipped.

    token tok;
    int depth;
    std::string out;

    if (!expect_token_type(lexer::token::type::punctuation,
        static_cast<std::uint32_t>(lexer::punctuation_id::open_curly_bracket), &tok))
    {
        return out;
    }

    out   = "{";
    depth = 1;

    do
    {
        if (!next_token(&tok))
        {
            error("missing closing \'{\'!");
            return out;
        }

        // If the token is on a new line...
        for (std::uint32_t i = 0; i < tok.get_lines_crossed(); ++i)
        {
            out.push_back('\n');
        }

        if (tok.get_type() == token::type::punctuation)
        {
            if (is_punctuation_token(tok, punctuation_id::open_curly_bracket))
            {
                ++depth;
            }
            else if (is_punctuation_token(tok, punctuation_id::close_curly_bracket))
            {
                --depth;
            }
        }

        if (tok.get_type() == token::type::string)
        {
            out += "\"";
            out += tok.as_string();
            out += "\"";
        }
        else
        {
            out += tok.as_string();
        }
        out += " ";
    }
    while (depth);

    return out;
}

std::string lexer::scan_bracketed_section_exact(int tabs)
{
    std::string out;
    if (!expect_token_char('{'))
    {
        return out;
    }

    out = "{";
    int  depth      = 1;
    bool skip_white = false;
    bool do_tabs    = (tabs >= 0);

    while (depth && *m_script_ptr)
    {
        const char c = *(m_script_ptr++);
        switch (c)
        {
        case '\t' :
        case ' '  :
            if (skip_white)
            {
                continue;
            }
            break;

        case '\n' :
            if (do_tabs)
            {
                out.push_back(c);
                skip_white = true;
                continue;
            }
            break;

        case '{' :
            ++depth;
            ++tabs;
            break;

        case '}' :
            --depth;
            --tabs;
            break;

        default :
            break;
        } // switch (c)

        if (skip_white)
        {
            int i = tabs;
            if (c == '{')
            {
                --i;
            }

            skip_white = false;
            for (; i > 0; --i)
            {
                out.push_back('\t');
            }
        }
        out.push_back(c);
    }

    return out;
}

std::string lexer::scan_rest_of_line()
{
    token tok;
    std::string out;

    while (next_token(&tok))
    {
        if (tok.get_lines_crossed() != 0)
        {
            m_script_ptr = m_last_script_ptr;
            m_line_num   = m_last_line_num;
            break;
        }

        if (!out.empty())
        {
            out.push_back(' ');
        }
        out += tok.as_string();
    }

    return out;
}

std::string lexer::scan_complete_line()
{
    // Returns a string up to the '\n', but doesn't eat any
    // whitespace at the beginning of the next line.

    const char * start_ptr = m_script_ptr;
    for (;; ++m_script_ptr)
    {
        if (*m_script_ptr == '\0')
        {
            break; // End of the buffer
        }

        if (*m_script_ptr == '\n')
        {
            ++m_line_num;
            ++m_script_ptr;
            break;
        }
    }

    std::string out;
    out.assign(start_ptr, static_cast<std::size_t>(m_script_ptr - start_ptr));
    return out;
}

std::string lexer::get_last_whitespace() const
{
    std::string whitespace;
    for (const char * p = m_whitespace_start_ptr; p < m_whitespace_end_ptr; ++p)
    {
        whitespace.push_back(*p);
    }
    return whitespace;
}

bool lexer::internal_read_whitespace()
{
    for (;;)
    {
        // Skip whitespace:
        while (*m_script_ptr <= ' ')
        {
            if (!*m_script_ptr)
            {
                return false;
            }
            if (*m_script_ptr == '\n')
            {
                ++m_line_num;
            }
            ++m_script_ptr;
        }

        // Skip comments:
        if (*m_script_ptr == '/')
        {
            // C++-style comments:
            if (*(m_script_ptr + 1) == '/')
            {
                ++m_script_ptr;
                do
                {
                    ++m_script_ptr;
                    if (!*m_script_ptr)
                    {
                        return false;
                    }
                }
                while (*m_script_ptr != '\n');

                ++m_line_num;
                ++m_script_ptr;

                if (!*m_script_ptr)
                {
                    return false;
                }
                continue;
            }
            // C-style/multi-line comments:
            else if (*(m_script_ptr + 1) == '*')
            {
                ++m_script_ptr;
                for (;;)
                {
                    ++m_script_ptr;
                    if (!*m_script_ptr)
                    {
                        return false;
                    }
                    if (*m_script_ptr == '\n')
                    {
                        ++m_line_num;
                    }
                    else if (*m_script_ptr == '/')
                    {
                        if (*(m_script_ptr - 1) == '*')
                        {
                            break;
                        }
                        if (*(m_script_ptr + 1) == '*')
                        {
                            warning("nested C-style, multi-line comment!");
                        }
                    }
                }

                ++m_script_ptr;
                if (!*m_script_ptr)
                {
                    return false;
                }

                ++m_script_ptr;
                if (!*m_script_ptr)
                {
                    return false;
                }
                continue;
            }
        }
        break;
    }
    return true;
}

bool lexer::internal_read_escape_character(char * out_char)
{
    assert(out_char != nullptr);

    int c, val, i;
    ++m_script_ptr; // Step over the leading '\\'

    // Determine the escape character:
    switch (*m_script_ptr)
    {
    case '0'  : c = '\0'; break;
    case 'n'  : c = '\n'; break;
    case 'r'  : c = '\r'; break;
    case 't'  : c = '\t'; break;
    case 'v'  : c = '\v'; break;
    case 'b'  : c = '\b'; break;
    case 'f'  : c = '\f'; break;
    case 'a'  : c = '\a'; break;
    case '\\' : c = '\\'; break;
    case '\'' : c = '\''; break;
    case '\"' : c = '\"'; break;
    case '\?' : c = '\?'; break;
    case 'x'  : // Scan hexadecimal constant:
        {
            ++m_script_ptr;
            for (i = 0, val = 0; ; ++i, ++m_script_ptr)
            {
                c = *m_script_ptr;
                if (c >= '0' && c <= '9')
                {
                    c = c - '0';
                }
                else if (c >= 'A' && c <= 'Z')
                {
                    c = c - 'A' + 10;
                }
                else if (c >= 'a' && c <= 'z')
                {
                    c = c - 'a' + 10;
                }
                else
                {
                    break;
                }
                val = (val << 4) + c;
            }
            --m_script_ptr;

            if (val > 0xFF)
            {
                warning("hexadecimal value in escape character is too big! Truncating to 0xFF...");
                val = 0xFF;
            }

            c = val;
            break;
        }
    default : // NOTE: decimal ASCII code, NOT octal!
        {
            if (*m_script_ptr < '0' || *m_script_ptr > '9')
            {
                return error("unknown/invalid escape char!");
            }

            for (i = 0, val = 0; ; ++i, ++m_script_ptr)
            {
                c = *m_script_ptr;
                if (c >= '0' && c <= '9')
                {
                    c = c - '0';
                }
                else
                {
                    break;
                }
                val = val * 10 + c;
            }
            --m_script_ptr;

            if (val > 0xFF)
            {
                warning("value in escape character is too big! Truncating to 0xFF...");
                val = 0xFF;
            }

            c = val;
            break;
        }
    } // switch (*m_script_ptr)

    // Step over the escape character or the last digit of the number.
    ++m_script_ptr;

    *out_char = static_cast<char>(c);
    return true;
}

bool lexer::internal_read_string(const int quote, token * out_token)
{
    assert(out_token != nullptr);

    // Escape characters are interpreted.
    // Reads two strings with only a white space between them as one string.

    std::uint32_t tmp_line_num;
    const char * tmp_script_ptr;
    char ch;

    if (quote == '"') // Quoted string
    {
        out_token->set_type(token::type::string);
    }
    else // Character literal (maybe a multi-char literal)
    {
        out_token->set_type(token::type::literal);
    }

    ++m_script_ptr; // Skip leading quote

    for (;;)
    {
        // If there is an escape character and escape characters are allowed...
        if (*m_script_ptr == '\\' && !(m_flags & flags::no_string_escape_chars))
        {
            if (!internal_read_escape_character(&ch))
            {
                return false;
            }
            out_token->append(ch);
        }
        // If a trailing quote...
        else if (*m_script_ptr == quote)
        {
            // Step over the quote:
            ++m_script_ptr;

            // If consecutive strings should not be concatenated...
            if ((m_flags & flags::no_string_concat) &&
              (!(m_flags & flags::allow_backslash_string_concat) || quote != '"'))
            {
                break;
            }

            tmp_script_ptr = m_script_ptr;
            tmp_line_num   = m_line_num;

            // Read white space between possible two consecutive strings.
            // Restore line index on failure.
            if (!internal_read_whitespace())
            {
                m_script_ptr = tmp_script_ptr;
                m_line_num   = tmp_line_num;
                break;
            }

            if (m_flags & flags::no_string_concat)
            {
                if (*m_script_ptr != '\\')
                {
                    m_script_ptr = tmp_script_ptr;
                    m_line_num   = tmp_line_num;
                    break;
                }

                ++m_script_ptr; // Step over the '\\'

                if (!internal_read_whitespace() || *m_script_ptr != quote)
                {
                    return error("expecting string after '\\' terminated line!");
                }
            }

            // If there's no leading quote...
            if (*m_script_ptr != quote)
            {
                m_script_ptr = tmp_script_ptr;
                m_line_num   = tmp_line_num;
                break;
            }

            ++m_script_ptr; // Step over the new leading quote.
        }
        else
        {
            if (*m_script_ptr == '\0')
            {
                return error("missing trailing quote!");
            }
            if (*m_script_ptr == '\n')
            {
                return error("newline inside string!");
            }
            out_token->append(*m_script_ptr++);
        }
    }

    if (out_token->get_type() == token::type::literal)
    {
        if (!(m_flags & flags::allow_multi_char_literals))
        {
            if (out_token->get_length() > 1)
            {
                return error("char literal is not one character long! Set \'lexer::flags::allow_multi_char_literals\' to allow them.");
            }
        }
    }

    return true;
}

bool lexer::internal_read_name_ident(token * out_token)
{
    assert(out_token != nullptr);

    // Names can contain aA-zZ letters, numbers or underscore.
    auto valid_name_char = [](const int c) -> bool
    {
        return ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_');
    };

    // If treating all tokens as strings, don't parse '-' as a separate token.
    auto strings_only = [](const int c, const std::uint32_t lex_flags) -> bool
    {
        return (lex_flags & flags::only_strings) && (c == '-');
    };

    // If special path name characters are allowed.
    auto allowing_path_names = [](const int c, const std::uint32_t lex_flags) -> bool
    {
        return (lex_flags & flags::allow_path_names) && (c == '/' || c == '\\' || c == ':' || c == '.');
    };

    int c;
    out_token->set_type(token::type::identifier);

    do
    {
        out_token->append(*m_script_ptr++);
        c = *m_script_ptr;
    }
    while (valid_name_char(c) || strings_only(c, m_flags) || allowing_path_names(c, m_flags));

    // Names reserved for the boolean constants:
    if (*out_token == "true" || *out_token == "false")
    {
        out_token->set_flags(token::flags::boolean);
    }
    else
    {
        out_token->set_flags(0);
    }

    return true;
}

bool lexer::internal_read_number(token * out_token)
{
    assert(out_token != nullptr);

    char c1 = *m_script_ptr;
    char c2 = *(m_script_ptr + 1);
    std::uint32_t token_flags = 0;

    if (c1 == '0' && c2 != '.') // Integer:
    {
        // Check for a hexadecimal number:
        if (c2 == 'x' || c2 == 'X')
        {
            out_token->append(*m_script_ptr++);
            out_token->append(*m_script_ptr++);
            c1 = *m_script_ptr;

            while ((c1 >= '0' && c1 <= '9') ||
                   (c1 >= 'a' && c1 <= 'f') ||
                   (c1 >= 'A' && c1 <= 'F'))
            {
                out_token->append(c1);
                c1 = *(++m_script_ptr);
            }

            token_flags = token::flags::hexadecimal | token::flags::integer;
        }
        // Check for a binary number:
        else if (c2 == 'b' || c2 == 'B')
        {
            out_token->append(*m_script_ptr++);
            out_token->append(*m_script_ptr++);
            c1 = *m_script_ptr;

            while (c1 == '0' || c1 == '1')
            {
                out_token->append(c1);
                c1 = *(++m_script_ptr);
            }

            token_flags = token::flags::binary | token::flags::integer;
        }
        // Its an octal number:
        else
        {
            out_token->append(*m_script_ptr++);
            c1 = *m_script_ptr;

            while (c1 >= '0' && c1 <= '7')
            {
                out_token->append(c1);
                c1 = *(++m_script_ptr);
            }

            token_flags = token::flags::octal | token::flags::integer;
        }
    }
    else // Decimal integer or floating point number or even an IP address:
    {
        // Count the number of '.' in the number:
        int dot = 0;
        for (;;)
        {
            if (c1 >= '0' && c1 <= '9')
            {
                // Nothing
            }
            else if (c1 == '.')
            {
                ++dot;
            }
            else
            {
                break;
            }

            out_token->append(c1);
            c1 = *(++m_script_ptr);
        }

        if (c1 == 'e' && dot == 0)
        {
            // We have scientific notation without a decimal point.
            ++dot;
        }

        // If a floating point number...
        if (dot == 1)
        {
            token_flags = token::flags::decimal | token::flags::floating_point;

            // Check for floating point exponent:
            if (c1 == 'e')
            {
                // Append the 'e' so that token::update_cached_values() parses the value properly.
                out_token->append(c1);
                c1 = *(++m_script_ptr);

                if (c1 == '-')
                {
                    out_token->append(c1);
                    c1 = *(++m_script_ptr);
                }
                else if (c1 == '+')
                {
                    out_token->append(c1);
                    c1 = *(++m_script_ptr);
                }

                while (c1 >= '0' && c1 <= '9')
                {
                    out_token->append(c1);
                    c1 = *(++m_script_ptr);
                }
            }
            // Check for floating point exceptions -> infinite 1.#INF or indefinite 1.#IND or NaN:
            else if (c1 == '#')
            {
                c2 = 4;
                if (internal_check_string("INF"))
                {
                    token_flags |= token::flags::infinite;
                }
                else if (internal_check_string("IND"))
                {
                    token_flags |= token::flags::indefinite;
                }
                else if (internal_check_string("NAN"))
                {
                    token_flags |= token::flags::nan;
                }
                else if (internal_check_string("QNAN"))
                {
                    token_flags |= token::flags::nan;
                    c2++;
                }
                else if (internal_check_string("SNAN"))
                {
                    token_flags |= token::flags::nan;
                    c2++;
                }

                for (int i = 0; i < c2; ++i)
                {
                    out_token->append(c1);
                    c1 = *(++m_script_ptr);
                }

                while (c1 >= '0' && c1 <= '9')
                {
                    out_token->append(c1);
                    c1 = *(++m_script_ptr);
                }

                if (!(m_flags & flags::allow_float_exceptions))
                {
                    return error("floating-point exception scanned: " + out_token->as_string());
                }
            }
        }
        else if (dot > 1)
        {
            if (!(m_flags & flags::allow_ip_addresses))
            {
                return error("more than one dot in number! Set lexer::flags::allow_ip_addresses to parse IP addresses.");
            }
            if (dot != 3)
            {
                return error("IP address should have three dots!");
            }
            token_flags = token::flags::ip_address;
        }
        else
        {
            token_flags = token::flags::decimal | token::flags::integer;
        }
    }

    if (token_flags & token::flags::floating_point)
    {
        if (c1 > ' ')
        {
            // Single-precision: float
            if (c1 == 'f' || c1 == 'F')
            {
                token_flags |= token::flags::single_precision;
                ++m_script_ptr;
            }
            // Extended-precision: long double
            else if (c1 == 'l' || c1 == 'L')
            {
                token_flags |= token::flags::extended_precision;
                ++m_script_ptr;
            }
            // Default is double-precision: double
            else
            {
                token_flags |= token::flags::double_precision;
            }
        }
        else
        {
            token_flags |= token::flags::double_precision;
        }
    }
    else if (token_flags & token::flags::integer)
    {
        // Default: signed
        std::uint32_t int_flag = token::flags::signed_integer;

        if (c1 > ' ')
        {
            // 1u, 1l, 1ul, 1lu
            for (int i = 0; i < 2; ++i)
            {
                if (c1 == 'u' || c1 == 'U') // unsigned integer supersedes signed
                {
                    int_flag = token::flags::unsigned_integer;
                }
                else if (c1 == 'l' || c1 == 'L') // signed integer
                {
                    // Already set; ignore.
                }
                else
                {
                    break;
                }
                c1 = *(++m_script_ptr);
            }
        }

        token_flags |= int_flag;
    }
    else if (token_flags & token::flags::ip_address)
    {
        if (c1 == ':')
        {
            out_token->append(c1);
            c1 = *(++m_script_ptr);

            while (c1 >= '0' && c1 <= '9')
            {
                out_token->append(c1);
                c1 = *(++m_script_ptr);
            }

            token_flags |= token::flags::ip_port;
        }
    }

    out_token->set_type(token::type::number);
    out_token->set_flags(token_flags);
    return true;
}

bool lexer::internal_read_punctuation(token * out_token)
{
    assert(out_token      != nullptr);
    assert(m_punctuations != nullptr);

    int l, n, i;
    for (n = m_punctuations_table[static_cast<unsigned>(*m_script_ptr)]; n >= 0; n = m_punctuations_next[n])
    {
        const punctuation_def punct = m_punctuations[n];
        const char * const chars    = punct.str;

        if (chars == nullptr) // punctuation_id::none
        {
            continue;
        }

        // Check for this punctuation in the script:
        for (l = 0; chars[l] && m_script_ptr[l]; ++l)
        {
            if (m_script_ptr[l] != chars[l])
            {
                break;
            }
        }

        if (!chars[l])
        {
            for (i = 0; i <= l; ++i)
            {
                out_token->append(chars[i]);
            }

            m_script_ptr += l;
            out_token->set_type(token::type::punctuation);
            out_token->set_flags(static_cast<std::uint32_t>(punct.id)); // Subtype/flags is the punctuation id.
            return true;
        }
    }

    return false;
}

bool lexer::internal_check_string(const char * const string) const
{
    assert(string != nullptr);

    for (std::size_t i = 0; string[i] != '\0'; ++i)
    {
        if (m_script_ptr[i] != string[i])
        {
            return false;
        }
    }
    return true;
}

std::string & lexer::ltrim_string(std::string * s)
{
    assert(s != nullptr);
    const auto first_non_blank = s->find_first_not_of(" \t\r\n\v\f");
    return s->erase(0, first_non_blank);
}

std::string & lexer::rtrim_string(std::string * s)
{
    assert(s != nullptr);
    const auto last_non_blank = s->find_last_not_of(" \t\r\n\v\f");
    return s->erase((last_non_blank != std::string::npos) ? last_non_blank + 1 : 0);
}

std::string & lexer::trim_string(std::string * s)
{
    rtrim_string(s);
    ltrim_string(s);
    return *s;
}

// ========================================================
// Shared error callbacks:
// ========================================================

lexer::error_callbacks * lexer::m_error_callbacks = nullptr;

void lexer::set_error_callbacks(error_callbacks * err_callbacks) noexcept
{
    struct default_error_callbacks : public lexer::error_callbacks
    {
        void error(const std::string & message, const bool fatal) override
        {
            std::cerr << message << std::endl;

            #ifndef LEXER_NO_CXX_EXCEPTIONS
            if (fatal)
            {
                throw lexer::exception{ message };
            }
            #else // ignored
            (void)fatal;
            #endif // LEXER_NO_CXX_EXCEPTIONS
        }

        void warning(const std::string & message) override
        {
            std::cerr << message << std::endl;
        }
    };
    static default_error_callbacks default_err_cbs;

    if (err_callbacks == nullptr)
    {
        m_error_callbacks = &default_err_cbs;
    }
    else
    {
        m_error_callbacks = err_callbacks;
    }
}

lexer::error_callbacks * lexer::get_error_callbacks() noexcept
{
    return m_error_callbacks;
}

// ========================================================
// Shared punctuation tables:
// ========================================================

const lexer::punctuation_def        * lexer::m_punctuations       = nullptr;
const lexer::punct_table_index_type * lexer::m_punctuations_table = nullptr;
const lexer::punct_table_index_type * lexer::m_punctuations_next  = nullptr;
std::size_t                           lexer::m_punctuations_size  = 0;

void lexer::set_punctuation_tables(const punctuation_def * const punctuations,
                                   punct_table_index_type * punctuations_table,
                                   punct_table_index_type * punctuations_next,
                                   const std::size_t punctuations_size)
{
    assert(punctuations       != nullptr);
    assert(punctuations_table != nullptr);
    assert(punctuations_next  != nullptr);
    assert(punctuations_size  != 0);

    // -1 marks the unused entries.
    const punct_table_index_type fill_val = -1;

    std::fill_n(punctuations_table, 256, fill_val);
    std::fill_n(punctuations_next, punctuations_size, fill_val);

    for (std::size_t i = 0; i < punctuations_size; ++i)
    {
        const punctuation_def new_punct = punctuations[i];
        if (new_punct.str == nullptr) // punctuation_id::none
        {
            continue;
        }

        int n;
        int last_punct_id = -1;

        // Sort the punctuations in this table entry on length (longer punctuations first):
        for (n = punctuations_table[static_cast<unsigned>(new_punct.str[0])]; n >= 0; n = punctuations_next[n])
        {
            const punctuation_def punct = punctuations[n];
            if (std::strlen(punct.str) < std::strlen(new_punct.str))
            {
                punctuations_next[i] = n;
                if (last_punct_id >= 0)
                {
                    punctuations_next[last_punct_id] = static_cast<punct_table_index_type>(i);
                }
                else
                {
                    punctuations_table[static_cast<unsigned>(new_punct.str[0])] = static_cast<punct_table_index_type>(i);
                }
                break;
            }
            last_punct_id = n;
        }

        if (n < 0)
        {
            punctuations_next[i] = -1;
            if (last_punct_id >= 0)
            {
                punctuations_next[last_punct_id] = static_cast<punct_table_index_type>(i);
            }
            else
            {
                punctuations_table[static_cast<unsigned>(new_punct.str[0])] = static_cast<punct_table_index_type>(i);
            }
        }
    }

    // Save the input pointers to the shared table pointers.
    // User must ensure the input arrays live long enough!
    m_punctuations       = punctuations;
    m_punctuations_size  = punctuations_size;
    m_punctuations_table = punctuations_table;
    m_punctuations_next  = punctuations_next;
}

void lexer::set_default_punctuation_tables()
{
    if (!default_punctuations_initialized)
    {
        // Also sets the static member pointers.
        set_punctuation_tables(default_punctuations,
                               default_punctuations_table,
                               default_punctuations_next,
                               default_punctuations_size);

        default_punctuations_initialized = true;
    }
    else // Just re-point to the default tables:
    {
        m_punctuations       = default_punctuations;
        m_punctuations_size  = default_punctuations_size;
        m_punctuations_table = default_punctuations_table;
        m_punctuations_next  = default_punctuations_next;
    }
}

std::string lexer::get_punctuation_from_id(const punctuation_id id)
{
    std::string result;
    const auto index = static_cast<unsigned>(id);

    if (id != punctuation_id::none && index < m_punctuations_size)
    {
        assert(m_punctuations[index].id == id);
        result = m_punctuations[index].str;
    }
    else
    {
        result = "(unknown punctuation)";
    }

    return result;
}

lexer::punctuation_id lexer::get_punctuation_id_from_str(const char * const punctuation_string)
{
    assert(punctuation_string != nullptr);

    for (std::size_t i = 0; i < m_punctuations_size; ++i)
    {
        if (std::strcmp(m_punctuations[i].str, punctuation_string) == 0)
        {
            return m_punctuations[i].id;
        }
    }

    return punctuation_id::none;
}

// ========================================================
// Default C/C++ punctuation tables:
// ========================================================

const lexer::punctuation_def lexer::default_punctuations[]
{
    { nullptr, lexer::punctuation_id::none                },
    { "=",     lexer::punctuation_id::assign              },
    { "+",     lexer::punctuation_id::add                 },
    { "-",     lexer::punctuation_id::sub                 },
    { "*",     lexer::punctuation_id::mul                 },
    { "/",     lexer::punctuation_id::div                 },
    { "%",     lexer::punctuation_id::mod                 },
    { ">>",    lexer::punctuation_id::rshift              },
    { "<<",    lexer::punctuation_id::lshift              },
    { "+=",    lexer::punctuation_id::add_assign          },
    { "-=",    lexer::punctuation_id::sub_assign          },
    { "*=",    lexer::punctuation_id::mul_assign          },
    { "/=",    lexer::punctuation_id::div_assign          },
    { "%=",    lexer::punctuation_id::mod_assign          },
    { ">>=",   lexer::punctuation_id::rshift_assign       },
    { "<<=",   lexer::punctuation_id::lshift_assign       },
    { "&&",    lexer::punctuation_id::logic_and           },
    { "||",    lexer::punctuation_id::logic_or            },
    { "!",     lexer::punctuation_id::logic_not           },
    { "==",    lexer::punctuation_id::logic_eq            },
    { "!=",    lexer::punctuation_id::logic_not_eq        },
    { ">",     lexer::punctuation_id::logic_greater       },
    { "<",     lexer::punctuation_id::logic_less          },
    { ">=",    lexer::punctuation_id::logic_greater_eq    },
    { "<=",    lexer::punctuation_id::logic_less_eq       },
    { "++",    lexer::punctuation_id::plus_plus           },
    { "--",    lexer::punctuation_id::minus_minus         },
    { "&",     lexer::punctuation_id::bitwise_and         },
    { "|",     lexer::punctuation_id::bitwise_or          },
    { "^",     lexer::punctuation_id::bitwise_xor         },
    { "~",     lexer::punctuation_id::bitwise_not         },
    { "&=",    lexer::punctuation_id::bitwise_and_assign  },
    { "|=",    lexer::punctuation_id::bitwise_or_assign   },
    { "^=",    lexer::punctuation_id::bitwise_xor_assign  },
    { ".",     lexer::punctuation_id::dot                 },
    { "->",    lexer::punctuation_id::arrow               },
    { "::",    lexer::punctuation_id::colon_colon         },
    { ".*",    lexer::punctuation_id::dot_star            },
    { ",",     lexer::punctuation_id::comma               },
    { ";",     lexer::punctuation_id::semicolon           },
    { ":",     lexer::punctuation_id::colon               },
    { "?",     lexer::punctuation_id::question_mark       },
    { "...",   lexer::punctuation_id::ellipsis            },
    { "\\",    lexer::punctuation_id::backslash           },
    { "(",     lexer::punctuation_id::open_parentheses    },
    { ")",     lexer::punctuation_id::close_parentheses   },
    { "[",     lexer::punctuation_id::open_bracket        },
    { "]",     lexer::punctuation_id::close_bracket       },
    { "{",     lexer::punctuation_id::open_curly_bracket  },
    { "}",     lexer::punctuation_id::close_curly_bracket },
    { "#",     lexer::punctuation_id::preprocessor        },
    { "##",    lexer::punctuation_id::preprocessor_merge  },
    { "$",     lexer::punctuation_id::dollar_sign         }
};

const std::size_t lexer::default_punctuations_size =
(
    sizeof(lexer::default_punctuations) / sizeof(lexer::default_punctuations[0])
);

// These are initialized once by lexer::set_default_punctuation_tables().
lexer::punct_table_index_type lexer::default_punctuations_table[256];
lexer::punct_table_index_type lexer::default_punctuations_next[lexer::default_punctuations_size];
bool lexer::default_punctuations_initialized = false;

// ========================================================
// Default punct tables and error callbacks initializer:
// ========================================================

namespace lexer_detail
{

static struct global_lex_initializer
{
    global_lex_initializer()
    {
        lexer::set_error_callbacks(nullptr); // sets the default_error_callbacks
        lexer::set_default_punctuation_tables();
    }
} g_global_lex_init;

} // namespace lexer_detail {}

// ================ End of implementation =================
#endif // LEXER_IMPLEMENTATION
// ================ End of implementation =================
