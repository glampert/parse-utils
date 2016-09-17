
// ================================================================================================
// -*- C++ -*-
// File: preprocessor.hpp
// Author: Guilherme R. Lampert
// Created on: 10/05/16
//
// About:
//  Preprocessor compatible with C-like languages.
//  Loosely based on idParser from DOOM 3 / id Tech 4.
//
// License:
//  This source file is released under the terms of the GNU General Public License version 3.
//  See the accompanying LICENSE file for full disclosure.
// ================================================================================================

#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

// Defining this before including the file prevents pulling the Standard headers.
// Useful to be able to place this file inside a user-defined namespace or to simply
// avoid redundant inclusions. User is responsible for providing all the necessary
// Standard headers before #including this one.
#ifndef PREPROCESSOR_NO_STD_INCLUDES
    #include <cstdint>
    #include <string>
    #include <vector>
#endif // PREPROCESSOR_NO_STD_INCLUDES

// Hook to allow providing a custom assert() before including this file.
#ifndef PREPROCESSOR_ASSERT
    #ifndef PREPROCESSOR_NO_STD_INCLUDES
        #include <cassert>
    #endif // PREPROCESSOR_NO_STD_INCLUDES
    #define PREPROCESSOR_ASSERT assert
#endif // PREPROCESSOR_ASSERT

//
// (Mostly) compatible C and C++ source code preprocessor.
// Supports all conditional compilation commands (#if, #ifdef, #else, etc)
// as well as #define, #undef, #include, #error, #warning, and others.
// Variadic macros as also supported. Some of the common built-in macros
// (like __FILE__ and __LINE__) are available as well. You can also register
// globally visible macros at your leisure.
//
// The extended $eval(), $evalint() and $evalfloat() preprocessor functions
// can be used to perform simple logical and arithmetical expression
// evaluation. They support a few built-in math constants (like PI and E)
// as well as common math functions like sqrt(), sin(), cos(), tan(), etc.
//
// This class relies on the lexer class to parse the source, so lexer
// must be included BEFORE including preprocessor.hpp. Your usage
// should look something like this:
//
//  #define LEXER_IMPLEMENTATION
//  #include "lexer.hpp"
//
//  #define PREPROCESSOR_IMPLEMENTATION
//  #include "preprocessor.hpp"
//
//  ... Code using the preprocessor follows ...
//
class preprocessor final
{
public:

    //
    // Miscellaneous preprocessor flags. Can be ORed together.
    //
    struct flags final
    {
        // Error/warn flags will be passed on to the lexer. If initializing
        // from a lexer, they also overwrite the lexer's error settings.
        static constexpr std::uint32_t no_errors                = 1 << 0; // Don't generate any errors.
        static constexpr std::uint32_t no_warnings              = 1 << 1; // Don't generate any warnings.
        static constexpr std::uint32_t no_fatal_errors          = 1 << 2; // Errors aren't fatal. By default all errors are fatal.

        // Additional preprocessor-specific settings:
        static constexpr std::uint32_t no_dollar_preproc        = 1 << 3; // Don't accept the '$' sign as a preprocessor.
        static constexpr std::uint32_t no_base_includes         = 1 << 4; // Don't include files embraced with < >.
        static constexpr std::uint32_t no_includes              = 1 << 5; // Disabling file inclusion via the #include directive.
        static constexpr std::uint32_t warn_macro_redefinitions = 1 << 6; // Issue a warning if a previously #defined macro is #defined again.
    }; // flags

    //
    // Preprocessor public interface:
    //

    // Default constructor makes an empty and uninitialized preprocessor.
    preprocessor();
    ~preprocessor();

    // Not copyable.
    preprocessor(const preprocessor & other) = delete;
    preprocessor & operator = (const preprocessor & other) = delete;

    // But movable.
    preprocessor(preprocessor && other) noexcept;
    preprocessor & operator = (preprocessor && other) noexcept;

    // Load a script from the file system. Fails if a script file is already loaded.
    // You must call clear() first before loading a new file. If 'silent' is set,
    // error() is not called upon failure but the function still returns false.
    bool init_from_file(std::string filename, std::uint32_t flags = 0, bool silent = false);

    // Preprocess the contents of an external C-style string.
    // Ownership of the string pointer is not acquired. Caller still owns that memory.
    bool init_from_memory(const char * ptr, std::uint32_t length, std::string filename,
                          std::uint32_t flags = 0, std::uint32_t starting_line = 1);

    // Take a non-owning reference to an external lexer and preprocess its tokens.
    bool init_from_lexer(lexer * initial_script, std::uint32_t flags = 0);

    // Runs the preprocessor in the current script/source writing to the output string buffer.
    // The output will be minified. The original spacing and indenting of the script is not preserved.
    bool preprocess(std::string * out_text_buffer);

    // Flags, max output line length and search paths are preserved.
    // Macros cleared, except for the built-ins.
    void clear();

    // Error handing forwards to the current lexer/script.
    bool error(const std::string & message);
    void warning(const std::string & message);

    // Hash function used internally to hash macro names. Publicly visible.
    static std::uint32_t hash_string(const char * str, std::size_t count);

    //
    // Getters/setters:
    //

    bool is_initialized()       const noexcept;
    bool allow_dollar_preproc() const noexcept;
    bool allow_base_includes()  const noexcept;
    bool allow_includes()       const noexcept;

    void enable_warnings()  noexcept;
    void disable_warnings() noexcept;

    lexer * get_current_script() noexcept;
    const lexer * get_current_script() const noexcept;

    // Control the max line length of the output text. Lines are only
    // broken at semicolons, so this value is a hint. Initially = 128;
    std::uint32_t get_max_output_line_length() const noexcept;
    void set_max_output_line_length(const std::uint32_t new_value) noexcept;

    // Default search paths are prepended to filenames #included with < >
    bool add_default_search_path(std::string path, char path_separator = '/');
    void clear_default_search_paths();

    //
    // Preprocessor macros:
    //

    // Define a globally visible macro in the NAME=VALUE form, e.g.: '#define FOO 42'.
    bool define(const std::string & macro_name, lexer::token value, bool allow_redefinition);
    bool define(const std::string & macro_name, std::string  value, bool allow_redefinition);
    bool define(const std::string & macro_name, std::int64_t value, bool allow_redefinition);
    bool define(const std::string & macro_name, double       value, bool allow_redefinition);

    // Define a globally visible macro from a code string, e.g.: "#define SQUARE(x) ((x) * (x))".
    // You can use this to define NAME=VALUE macros, function-like macros and variadic macros.
    bool define(const std::string & define_string, bool allow_redefinition);

    // Check if the string names a macro.
    bool is_defined(const std::string & macro_name) const;

    // Undefine the given macro if it exists.
    // Note: This will also undef the built-in macros if a built-in name is given.
    void undef(const std::string & macro_name);

    // Undefine all macros. Optionally you can keep or also remove the built-ins.
    void undef_all(bool keep_built_ins = true);

    // Get the value of a NAME=VALUE macro as a lexer token. Fails for multi-token macros.
    // Only returns true for the '#define FOO bar' kind of macros.
    bool find_macro_token(const std::string & macro_name, lexer::token * out_token) const;

    // Finds the list of tokens belonging to the body of a macro. Returns null if the macro is not defined.
    // The returned pointer belongs to the preprocessor, so you should not free it. It remains valid while
    // the preprocessor instance is alive or until the macro is undefined.
    const lexer::token * find_macro_tokens(const std::string & macro_name, int * out_num_tokens) const;

    // Find the value of a NAME=VALUE kind of macro. Fails if the macro cannot be resolved to a number or string.
    bool find_macro_value(const std::string & macro_name, std::string  * out_value) const;
    bool find_macro_value(const std::string & macro_name, std::int64_t * out_value) const;
    bool find_macro_value(const std::string & macro_name, double       * out_value) const;

    //
    // Expression evaluation:
    //

    // Evaluates a string consisting of logical and/or arithmetical expressions, using the same expr_evaluator
    // the preprocessor uses internally to resolve conditional directive and the $eval() extensions. The string
    // should not be prefixed with $eval[int|float]. The output values are optional. You can pass either or none.
    // Returns true if the expression was successfully evaluated and had no syntax errors.
    bool eval(const std::string & expression, std::int64_t * out_i_result, double * out_f_result,
              bool math_consts, bool math_funcs, bool undefined_consts_are_zero);

private:

    //
    // Miscellaneous internal helpers:
    //

    class macro_parameter_pack;

    bool next_token(lexer::token * out_token);
    bool next_token_on_line(lexer::token * out_token);
    void unget_token(const lexer::token & tok);
    bool read_line(std::string * out_line);
    bool skip_rest_of_line();

    bool check_preproc(const lexer::token & tok) const noexcept;
    bool resolve_preproc_and_append(const lexer::token & tok, std::string * out_text_buffer);
    bool resolve_hash_directive();
    bool resolve_dollar_directive(std::string * out_text_buffer);
    bool try_open_include_file(const std::string & filename);
    void string_append_token(const lexer::token & tok, std::string * out_str);

    void output_append_token_text(const lexer::token & tok, std::string * out_text_buffer,
                                  bool no_string_escape = false, bool no_whitespace = false);

    bool expand_macro_and_append(int macro_index, std::string * out_text_buffer,
                                 macro_parameter_pack * param_pack, macro_parameter_pack * parent_pack);

    int expand_recursive_macro_and_append(int macro_index, int other_macro_index, int token_index,
                                          const std::vector<lexer::token> * params_provided,
                                          std::string * out_text_buffer);

    //
    // Directive handlers:
    //

    bool resolve_if_directive();
    bool resolve_ifdef_directive();
    bool resolve_ifndef_directive();
    bool resolve_elif_directive();
    bool resolve_else_directive();
    bool resolve_endif_directive();
    bool resolve_define_directive();
    bool resolve_undef_directive();
    bool resolve_line_directive();
    bool resolve_error_directive();
    bool resolve_warning_directive();
    bool resolve_pragma_directive();
    bool resolve_include_directive();

    //
    // Preprocessor macros (#define/#undef):
    //

    #pragma pack(push, 1)
    struct macro_def final
    {
        // Indexes are into the m_macro_tokens vector.
        std::uint32_t hashed_name           : 32;
        std::uint32_t first_param_token     : 31;
        std::uint32_t first_body_token      : 31;
        std::uint32_t param_token_count     : 16;
        std::uint32_t body_token_count      : 16;
        bool          empty_func_like_macro : 1;
        bool          va_args_macro         : 1;
    };
    #pragma pack(pop)

    // We don't need more than the above specified bit widths, so packing the structure
    // helps save memory and allows fitting more macro_defs into each cache line.
    static_assert(sizeof(macro_def) == 16, "Wrong size for macro_def struct!");

    // Constants generated with hash_string() using the corresponding text string.
    static constexpr std::uint32_t builtin_macro_file    = 0x07215FFC; // __FILE__
    static constexpr std::uint32_t builtin_macro_line    = 0x5DB1B324; // __LINE__
    static constexpr std::uint32_t builtin_macro_date    = 0x70D6DAE9; // __DATE__
    static constexpr std::uint32_t builtin_macro_time    = 0xC32DC18B; // __TIME__
    static constexpr std::uint32_t builtin_macro_va_args = 0x9EE0B9AA; // __VA_ARGS__

    void macro_define_builtins();
    void macro_expand_builtin(const macro_def & macro, std::string * out_text_buffer, macro_parameter_pack * va_args);
    bool macro_is_builtin(const macro_def & macro) const noexcept;
    int  macro_find_index(std::uint32_t hashed_macro_name) const noexcept;
    void macro_define(const std::string & macro_name, macro_def * new_macro);
    void macro_undefine(const std::string & macro_name);
    void macro_clear_tokens(const macro_def & macro);

    //
    // Preprocessor conditionals:
    //

    enum class conditional_type : std::uint8_t
    {
        cond_if,
        cond_ifdef,
        cond_ifndef,
        cond_elif,
        cond_else
    };

    struct conditional_def final
    {
        conditional_type type;
        bool             skip_body;
        bool             parent_state;
    };

    void push_conditional(conditional_type type, bool skip_body, const bool * opt_parent_state = nullptr);
    bool pop_conditional(conditional_type * out_type = nullptr, bool * out_skip = nullptr, bool * out_parent_state = nullptr);
    bool evaluate_preproc_conditional(bool * out_result);

private:

    lexer *                      m_current_script       = nullptr; // Pointer to an external lexer or to m_dynamic_scripts if at an #included file.
    std::uint32_t                m_flags                = 0;       // preprocessor::flags ORed together or zero.
    std::int32_t                 m_skipping_conditional = 0;       // > 0 when skipping code inside #ifs/#ifdefs/etc.
    std::uint32_t                m_output_line_len      = 0;       // Current length of output. Reset whenever a newline character is emitted.
    std::uint32_t                m_output_max_line_len  = 128;     // Max length hint for the output text. Lines only effectively broke at semicolons.
    lexer::token::type           m_prev_token_type{};              // Used for the output minifier to figure out where to insert spaces.
    std::vector<macro_def>       m_macros;                         // Macros currently defined via script code or preprocessor::define().
    std::vector<lexer::token>    m_macro_tokens;                   // macro_def indexes point into this vector.
    std::vector<conditional_def> m_cond_stack;                     // #if/#ifdef/#else/etc preprocessor conditionals.
    std::vector<lexer *>         m_include_stack;                  // Stack top is the previous script before entering an #include.
    std::vector<lexer *>         m_dynamic_scripts;                // Stuff allocated by the preprocessor (#includes, init_from_file(), etc).
    std::vector<std::string>     m_search_paths;                   // User-provided search paths for #includes enclosed in < >.
};

// ================== End of header file ==================
#endif // PREPROCESSOR_HPP
// ================== End of header file ==================

// ================================================================================================
//
//                                 Preprocessor Implementation
//
// ================================================================================================

#ifdef PREPROCESSOR_IMPLEMENTATION

#ifndef PREPROCESSOR_NO_STD_INCLUDES
    #include <cmath>
    #include <ctime>
    #include <cstdio>
    #include <cstring>
    #include <utility>
#endif // PREPROCESSOR_NO_STD_INCLUDES

#define PREPROC_FLOAT_FMT   "%.20lf"
#define PREPROC_INT64_FMT   "%lli"
#define PREPROC_NUMBUF_SIZE 128

// ========================================================
// expr_evaluator helper class:
// ========================================================

//
// This helper class is used by the preprocessor to resolve
// #if/#ifdef directives as well as $eval() extensions.
// Since it operates on lexer::tokens and is not a generic
// library class, it is not exposed in the public preprocessor
// header file. You can still use it as a standalone class
// though, if it suits your needs. It requires a preprocessor
// instance for the error/warning callbacks and to perform
// macro constant expansion and 'defined()' expansion.
//
class expr_evaluator final
{
public:

    struct eval_flags final
    {
        static constexpr std::uint32_t detect_type               = 1 << 0;
        static constexpr std::uint32_t force_int_type            = 1 << 1;
        static constexpr std::uint32_t force_float_type          = 1 << 2;
        static constexpr std::uint32_t allow_math_funcs          = 1 << 3;
        static constexpr std::uint32_t allow_math_consts         = 1 << 4;
        static constexpr std::uint32_t undefined_consts_are_zero = 1 << 5;
    };

    enum eval_value_type
    {
        eval_type_int,
        eval_type_double
    };

    struct eval_value final
    {
        union
        {
            std::int64_t as_int;
            double       as_double;
        };
        eval_value_type type;
    };

    //
    // Evaluator interface:
    //

    explicit expr_evaluator(preprocessor * pp)
        : m_preproc{ pp }
        , m_next_token_index{ 0 }
    {
        PREPROCESSOR_ASSERT(pp != nullptr);
    }

    void push_token(lexer::token && tok)
    {
        m_eval_tokens.push_back(std::forward<lexer::token>(tok));
    }

    std::size_t get_token_count() const noexcept
    {
        return m_eval_tokens.size();
    }

    bool evaluate(lexer::token * result_token, eval_value * result_value, const std::uint32_t flags)
    {
        eval_value expr_result{};

        if (m_eval_tokens.empty())
        {
            if (result_token != nullptr)
            {
                result_token->clear();
            }
            if (result_value != nullptr)
            {
                *result_value = expr_result;
            }
            m_preproc->warning("empty preprocessor eval directive.");
            return true;
        }

        m_next_token_index = 0; // Point to the first token in m_eval_tokens[].
        if (!process_tokens(&expr_result, flags))
        {
            return false;
        }

        if (result_token != nullptr)
        {
            value_to_token(result_token, expr_result, flags);
        }
        if (result_value != nullptr)
        {
            *result_value = expr_result;
        }
        return true;
    }

private:

    preprocessor *            m_preproc;          // Preprocessor with the error/warning callbacks and #define expansion.
    std::uint32_t             m_next_token_index; // Next token in the vector to process.
    std::vector<lexer::token> m_eval_tokens;      // List of lexer::tokens making up the expression, including parenthesis.

    //
    // Helper structures / types:
    //

    struct math_func final
    {
        const char * const name;
        double (*fptr)(double);
    };

    struct math_const final
    {
        const char * const name;
        const double       value;
    };

    static const math_func  builtin_math_funcs[];
    static const math_const builtin_math_consts[];

    struct operator_link final
    {
        operator_link   *     prev;
        operator_link   *     next;
        const math_func *     mathfunc;
        int                   precedence;
        int                   parentheses;
        lexer::punctuation_id op;
    };

    struct value_link final
    {
        value_link * prev;
        value_link * next;
        eval_value   value;
        int          parentheses;
    };

    class fixed_stack final
    {
    public:
        fixed_stack() = default;

        operator_link * new_operator() noexcept
        {
            if (operators_count < max_operators)
            {
                return &ops_stack[operators_count++];
            }
            return nullptr;
        }
        value_link * new_value() noexcept
        {
            if (values_count < max_values)
            {
                return &val_stack[values_count++];
            }
            return nullptr;
        }

    private:
        static constexpr int max_operators = 128;
        static constexpr int max_values    = 128;

        int operators_count = 0;
        int values_count    = 0;

        operator_link ops_stack[max_operators];
        value_link    val_stack[max_values];
    };

    //
    // Internal methods:
    //

    static const math_func * find_math_func(const std::string & wanted)
    {
        for (int i = 0; builtin_math_funcs[i].name != nullptr; ++i)
        {
            if (wanted == builtin_math_funcs[i].name)
            {
                return &builtin_math_funcs[i];
            }
        }
        return nullptr;
    }

    static const math_const * find_math_const(const std::string & wanted)
    {
        for (int i = 0; builtin_math_consts[i].name != nullptr; ++i)
        {
            if (wanted == builtin_math_consts[i].name)
            {
                return &builtin_math_consts[i];
            }
        }
        return nullptr;
    }

    template<typename ListNode>
    static void list_add_link(ListNode ** first, ListNode ** last, ListNode * new_link) noexcept
    {
        new_link->next = nullptr;
        new_link->prev = *last;
        if (*last != nullptr)
        {
            (*last)->next = new_link;
        }
        else
        {
            *first = new_link;
        }
        *last = new_link;
    }

    template<typename ListNode>
    static void list_remove_link(ListNode ** first, ListNode ** last, ListNode * rem_link) noexcept
    {
        if (rem_link->prev != nullptr)
        {
            rem_link->prev->next = rem_link->next;
        }
        else
        {
            *first = rem_link->next;
        }

        if (rem_link->next != nullptr)
        {
            rem_link->next->prev = rem_link->prev;
        }
        else
        {
            *last = rem_link->prev;
        }
    }

    bool process_tokens(eval_value * result_value, const std::uint32_t flags)
    {
        PREPROCESSOR_ASSERT(result_value != nullptr);

        // Nodes for the below lists are sourced from this fixed-length static stack,
        // so we avoid allocating any extra heap memory for the expression evaluation.
        fixed_stack stack;

        // Value and operator lists:
        operator_link * first_operator = nullptr;
        operator_link * last_operator  = nullptr;
        value_link    * first_value    = nullptr;
        value_link    * last_value     = nullptr;

        // Temp evaluator states:
        bool last_was_value    = false;
        bool negative_value    = false;
        int  parentheses_count = 0;

        // Evaluate the tokens belonging to the expression:
        for (auto t = next_token(); t != nullptr; t = next_token())
        {
            switch (t->get_type())
            {
            case lexer::token::type::identifier :
                {
                    if (last_was_value)
                    {
                        return m_preproc->error("syntax error in preprocessor expression!");
                    }

                    if (negative_value)
                    {
                        // Multiply the resulting expression or value by -1 to negate it.
                        if (!emit_mul_by_minus1(&first_value,    &last_value,
                                                &first_operator, &last_operator,
                                                &stack, parentheses_count))
                        {
                            return false;
                        }
                        negative_value = false;
                    }

                    if (*t == "defined") // 'defined FOO' directive
                    {
                        if (!resolve_defined_subexpr(&first_value, &last_value, &stack,
                                                     parentheses_count, flags))
                        {
                            return false;
                        }
                    }
                    else if (t->is_boolean()) // Boolean literals
                    {
                        value_link * v = stack.new_value();
                        if (v == nullptr)
                        {
                            return m_preproc->error("preprocessor expression is too long! No more stack space.");
                        }

                        v->value.type   = eval_type_int;
                        v->value.as_int = t->as_bool();
                        v->parentheses  = parentheses_count;
                        list_add_link(&first_value, &last_value, v);
                    }
                    else
                    {
                        // Could be a reference to one of the built-in math functions.
                        if (flags & eval_flags::allow_math_funcs)
                        {
                            if (const math_func * mf = find_math_func(t->as_string()))
                            {
                                value_link    * v = stack.new_value();
                                operator_link * o = stack.new_operator();

                                if (v == nullptr || o == nullptr)
                                {
                                    return m_preproc->error("preprocessor expression is too long! No more stack space.");
                                }

                                // Dummy operator for the function call:
                                o->mathfunc     = mf;
                                o->precedence   = 999;
                                o->parentheses  = parentheses_count;
                                o->op           = lexer::punctuation_id::none;

                                // The dummy operator also needs a following dummy value.
                                v->value.as_int = 0;
                                v->value.type   = eval_type_int;
                                v->parentheses  = parentheses_count;

                                list_add_link(&first_value,    &last_value,    v);
                                list_add_link(&first_operator, &last_operator, o);

                                last_was_value = false; // No value emitted.
                                break;
                            }
                        }

                        // Could be a reference to a macro constant:
                        lexer::token macro_token;
                        if (!m_preproc->find_macro_token(t->as_string(), &macro_token))
                        {
                            // Try a built-in math constant:
                            const math_const * mc;
                            if ((flags & eval_flags::allow_math_consts) &&
                                ((mc = find_math_const(t->as_string())) != nullptr))
                            {
                                char num_str[PREPROC_NUMBUF_SIZE] = {'\0'};
                                std::snprintf(num_str, sizeof(num_str), PREPROC_FLOAT_FMT, mc->value);
                                macro_token.set_string(num_str);
                                macro_token.set_type(lexer::token::type::number);
                                macro_token.set_flags(lexer::token::flags::floating_point  |
                                                      lexer::token::flags::double_precision);
                            }
                            else
                            {
                                // Optionally don't emit errors for undefined macro constants, assume zero instead.
                                if (flags & eval_flags::undefined_consts_are_zero)
                                {
                                    macro_token.set_string("0");
                                    macro_token.set_type(lexer::token::type::number);
                                    macro_token.set_flags(lexer::token::flags::integer |
                                                          lexer::token::flags::decimal |
                                                          lexer::token::flags::signed_integer);
                                }
                                else
                                {
                                    return m_preproc->error("reference to undefined preprocessor constant \'" +
                                                            t->as_string() + "\'.");
                                }
                            }
                        }

                        value_link * v = stack.new_value();
                        if (v == nullptr)
                        {
                            return m_preproc->error("preprocessor expression is too long! No more stack space.");
                        }

                        // Use the constant value (must be a number):
                        if (!token_to_value(&v->value, &macro_token, false))
                        {
                            return false;
                        }

                        v->parentheses = parentheses_count;
                        list_add_link(&first_value, &last_value, v);
                    }

                    last_was_value = true; // Macro expansion creates a value.
                    break;
                }
            case lexer::token::type::number :
                {
                    if (last_was_value)
                    {
                        return m_preproc->error("syntax error in preprocessor expression!");
                    }

                    value_link * v = stack.new_value();
                    if (v == nullptr)
                    {
                        return m_preproc->error("preprocessor expression is too long! No more stack space.");
                    }

                    if (!token_to_value(&v->value, t, negative_value))
                    {
                        return false;
                    }

                    v->parentheses = parentheses_count;
                    list_add_link(&first_value, &last_value, v);

                    last_was_value = true;
                    negative_value = false;
                    break;
                }
            case lexer::token::type::punctuation :
                {
                    if (lexer::is_punctuation_token(*t, lexer::punctuation_id::open_parentheses))
                    {
                        // If a minus sign precedes an opening parentheses '-(...)' the whole
                        // subexpression must be negated. The simples way is to just add a prefix
                        // multiplication by -1, so we create two additional nodes for the value and * operator.
                        if (negative_value)
                        {
                            if (!emit_mul_by_minus1(&first_value,    &last_value,
                                                    &first_operator, &last_operator,
                                                    &stack, parentheses_count))
                            {
                                return false;
                            }
                            last_was_value = false;
                            negative_value = false;
                        }
                        ++parentheses_count;
                        break;
                    }
                    else if (lexer::is_punctuation_token(*t, lexer::punctuation_id::close_parentheses))
                    {
                        if (--parentheses_count < 0)
                        {
                            return m_preproc->error("too many \'" +
                                                    lexer::get_punctuation_from_id(lexer::punctuation_id::close_parentheses) +
                                                    "\' in preprocessor directive!");
                        }
                        break;
                    }

                    bool unary_plus = false;
                    const auto punct_id = static_cast<lexer::punctuation_id>(t->get_flags());

                    if (negative_value)
                    {
                        // Multiple unary negations in sequence?
                        if (punct_id == lexer::punctuation_id::sub)
                        {
                            // One nullifies the other.
                            negative_value = !negative_value;
                            break;
                        }
                        else if (punct_id == lexer::punctuation_id::add)
                        {
                            // No-op unary +
                            break;
                        }
                        else if (punct_id == lexer::punctuation_id::logic_not ||
                                 punct_id == lexer::punctuation_id::bitwise_not)
                        {
                            // Unary negation permitted before ! or ~
                            //
                            // Same trick as used for the unary - before a ().
                            // Negate the final subexpression by multiplying by -1.
                            //
                            if (!emit_mul_by_minus1(&first_value,    &last_value,
                                                    &first_operator, &last_operator,
                                                    &stack, parentheses_count))
                            {
                                return false;
                            }
                            last_was_value = false;
                            negative_value = false;
                            // Allow the ! or ~ op to be emitted below.
                        }
                        else
                        {
                            return m_preproc->error("misplaced minus sign in preprocessor expression!");
                        }
                    }

                    switch (punct_id)
                    {
                    // Unary operators:
                    case lexer::punctuation_id::logic_not :
                    case lexer::punctuation_id::bitwise_not :
                        if (last_was_value)
                        {
                            return m_preproc->error("invalid logic not or two's complement after "
                                                    "value in preprocessor expression.");
                        }
                        break;
                    case lexer::punctuation_id::sub :
                        if (!last_was_value)
                        {
                            negative_value = true;
                            break;
                        }
                        /* fall-through */
                    case lexer::punctuation_id::add :
                        if (!last_was_value)
                        {
                            // Unary plus is a legal no-op.
                            unary_plus = true;
                            break;
                        }
                        /* fall-through */
                    case lexer::punctuation_id::mul :
                    case lexer::punctuation_id::div :
                    case lexer::punctuation_id::mod :
                    case lexer::punctuation_id::rshift :
                    case lexer::punctuation_id::lshift :
                    case lexer::punctuation_id::logic_greater :
                    case lexer::punctuation_id::logic_less :
                    case lexer::punctuation_id::logic_greater_eq :
                    case lexer::punctuation_id::logic_less_eq :
                    case lexer::punctuation_id::logic_eq :
                    case lexer::punctuation_id::logic_not_eq :
                    case lexer::punctuation_id::logic_and :
                    case lexer::punctuation_id::logic_or :
                    case lexer::punctuation_id::bitwise_and :
                    case lexer::punctuation_id::bitwise_or :
                    case lexer::punctuation_id::bitwise_xor :
                    case lexer::punctuation_id::colon :
                    case lexer::punctuation_id::question_mark :
                        if (!last_was_value)
                        {
                            return m_preproc->error("invalid operator \'" + t->as_string() +
                                                    "\' after operator in preprocessor expression.");
                        }
                        break;

                    default :
                        return m_preproc->error("invalid operator \'" + t->as_string() +
                                                "\' in preprocessor expression.");
                    } // switch (punct_id)

                    // Make a new operator node if not a unary negation or unary plus:
                    if (!negative_value && !unary_plus)
                    {
                        operator_link * o = stack.new_operator();
                        if (o == nullptr)
                        {
                            return m_preproc->error("preprocessor expression is too long! No more stack space.");
                        }

                        o->op          = punct_id;
                        o->mathfunc    = nullptr;
                        o->parentheses = parentheses_count;
                        o->precedence  = get_operator_precedence(punct_id);
                        list_add_link(&first_operator, &last_operator, o);

                        last_was_value = false;
                    }
                    break;
                }
            default :
                return m_preproc->error("unexpected token \'" + t->as_string() + "\' in preprocessor directive!");
            } // switch (t->get_type())
        } // for each token

        if (!last_was_value)
        {
            return m_preproc->error("trailing operator in preprocessor expression!");
        }

        if (parentheses_count > 0)
        {
            return m_preproc->error("too many \'" +
                                    lexer::get_punctuation_from_id(lexer::punctuation_id::open_parentheses) +
                                    "\' in preprocessor directive!");
        }

        // Ternary operator helpers:
        eval_value ternary_op_condition{};
        bool got_ternary_op_condition = false;

        // Now execute the subexpressions with correct precedence:
        while (first_operator != nullptr)
        {
            value_link    * v = first_value;
            operator_link * o = first_operator;

            for (; o->next != nullptr; o = o->next)
            {
                // If the current operator is nested deeper in
                // parentheses than the next operator...
                if (o->parentheses > o->next->parentheses)
                {
                    break;
                }

                // If the current and next operator are nested equally deep in parentheses:
                if (o->parentheses == o->next->parentheses)
                {
                    // If the precedence of the current operator is equal
                    // or higher than the precedence of the next operator:
                    if (o->precedence >= o->next->precedence)
                    {
                        break;
                    }
                }

                // If the arity of the operator isn't equal to 1 (unary op):
                if (o->op != lexer::punctuation_id::logic_not &&
                    o->op != lexer::punctuation_id::bitwise_not)
                {
                    v = v->next;
                }

                // If there's no value or no next value it's an error.
                if (v == nullptr)
                {
                    return m_preproc->error("expected more values in preprocessor expression!");
                }
            } // for each operator

            value_link * v1 = v;
            value_link * v2 = v->next;

            switch (o->op)
            {
            //
            // Unary operators:
            //
            case lexer::punctuation_id::logic_not :
                {
                    if (v1->value.type == eval_type_int)
                    {
                        v1->value.as_int = !v1->value.as_int;
                    }
                    else
                    {
                        v1->value.as_double = !v1->value.as_double;
                    }
                    break;
                }
            case lexer::punctuation_id::bitwise_not :
                {
                    if (v1->value.type == eval_type_int) // Integers only.
                    {
                        v1->value.as_int = ~v1->value.as_int;
                    }
                    else
                    {
                        return m_preproc->error("operator \'" + lexer::get_punctuation_from_id(o->op) +
                                                "\' cannot be applied to floating-point value!");
                    }
                    break;
                }

            //
            // Ternary operator:
            //
            case lexer::punctuation_id::colon :
                {
                    if (!got_ternary_op_condition)
                    {
                        return m_preproc->error("\'" + lexer::get_punctuation_from_id(lexer::punctuation_id::colon) +
                                                "\' without \'" + lexer::get_punctuation_from_id(lexer::punctuation_id::question_mark) +
                                                "\' in preprocessor directive!");
                    }

                    if (ternary_op_condition.type == eval_type_double)
                    {
                        if (!ternary_op_condition.as_double)
                        {
                            v1->value = v2->value;
                        }
                    }
                    else
                    {
                        if (!ternary_op_condition.as_int)
                        {
                            v1->value = v2->value;
                        }
                    }

                    ternary_op_condition = eval_value{};
                    got_ternary_op_condition = false;
                    break;
                }
            case lexer::punctuation_id::question_mark :
                {
                    if (got_ternary_op_condition)
                    {
                        const std::string qm = lexer::get_punctuation_from_id(lexer::punctuation_id::question_mark);
                        return m_preproc->error("\'" + qm + "\' after \'" + qm + "\' in preprocessor directive!");
                    }

                    ternary_op_condition = v1->value;
                    got_ternary_op_condition = true;
                    break;
                }

            //
            // Other arithmetical and logical binary operators or built-in function call:
            //
            default :
                {
                    eval_value result;
                    if (o->mathfunc != nullptr)
                    {
                        if (v2->value.type == eval_type_double)
                        {
                            result.as_double = o->mathfunc->fptr(v2->value.as_double);
                        }
                        else
                        {
                            result.as_double = o->mathfunc->fptr(v2->value.as_int);
                        }
                        result.type = eval_type_double;
                    }
                    else
                    {
                        if (!resolve_subexpr(&result, v1->value, v2->value, o->op))
                        {
                            return false;
                        }
                    }
                    v1->value = result;
                    break;
                }
            } // switch (o->op)

            if (o->op != lexer::punctuation_id::logic_not &&
                o->op != lexer::punctuation_id::bitwise_not)
            {
                // Remove the second value if not the ternary operator.
                if (o->op != lexer::punctuation_id::question_mark)
                {
                    v = v->next;
                }
                list_remove_link(&first_value, &last_value, v);
            }

            // Remove the operator:
            list_remove_link(&first_operator, &last_operator, o);
        } // while more operators

        PREPROCESSOR_ASSERT(first_value != nullptr);
        *result_value = first_value->value;
        return true;
    }

    static int get_operator_precedence(const lexer::punctuation_id op) noexcept
    {
        switch (op)
        {
        case lexer::punctuation_id::colon            : return 5;
        case lexer::punctuation_id::question_mark    : return 5;
        case lexer::punctuation_id::logic_or         : return 6;
        case lexer::punctuation_id::logic_and        : return 7;
        case lexer::punctuation_id::bitwise_or       : return 8;
        case lexer::punctuation_id::bitwise_xor      : return 9;
        case lexer::punctuation_id::bitwise_and      : return 10;
        case lexer::punctuation_id::logic_eq         : return 11;
        case lexer::punctuation_id::logic_not_eq     : return 11;
        case lexer::punctuation_id::logic_greater_eq : return 12;
        case lexer::punctuation_id::logic_less_eq    : return 12;
        case lexer::punctuation_id::logic_greater    : return 12;
        case lexer::punctuation_id::logic_less       : return 12;
        case lexer::punctuation_id::rshift           : return 13;
        case lexer::punctuation_id::lshift           : return 13;
        case lexer::punctuation_id::add              : return 14;
        case lexer::punctuation_id::sub              : return 14;
        case lexer::punctuation_id::mul              : return 15;
        case lexer::punctuation_id::div              : return 15;
        case lexer::punctuation_id::mod              : return 15;
        case lexer::punctuation_id::bitwise_not      : return 16;
        case lexer::punctuation_id::logic_not        : return 17;
        default                                      : return 0;
        } // switch (op)
    }

    const lexer::token * next_token()
    {
        if (m_next_token_index < m_eval_tokens.size())
        {
            return &m_eval_tokens[m_next_token_index++];
        }
        return nullptr;
    }

    bool emit_mul_by_minus1(value_link ** first_value, value_link ** last_value,
                            operator_link ** first_operator, operator_link ** last_operator,
                            fixed_stack * stack, const int parentheses_count)
    {
        value_link    * new_val = stack->new_value();
        operator_link * new_op  = stack->new_operator();

        if (new_val == nullptr || new_op == nullptr)
        {
            return m_preproc->error("preprocessor expression is too long! No more stack space.");
        }

        new_val->value.as_int = -1; // Will promote to double if needed.
        new_val->value.type   = eval_type_int;
        new_val->parentheses  = parentheses_count;

        new_op->parentheses   = parentheses_count;
        new_op->mathfunc      = nullptr;
        new_op->op            = lexer::punctuation_id::mul;
        new_op->precedence    = get_operator_precedence(lexer::punctuation_id::mul);

        list_add_link(first_value,    last_value,    new_val);
        list_add_link(first_operator, last_operator, new_op);
        return true;
    }

    bool resolve_defined_subexpr(value_link ** first_value, value_link ** last_value, fixed_stack * stack,
                                 const int parentheses_count, const std::uint32_t flags)
    {
        const lexer::token * t = next_token();
        bool open_par;

        if (t != nullptr && lexer::is_punctuation_token(*t, lexer::punctuation_id::open_parentheses))
        {
            open_par = true;
            t = next_token(); // Skip it.
        }
        else
        {
            open_par = false;
        }

        // Parenthesis are optional in a 'defined' command.
        if (t == nullptr || !t->is_identifier())
        {
            return m_preproc->error("preprocessor \'defined\' directive without identifier!");
        }

        value_link * v = stack->new_value();
        if (v == nullptr)
        {
            return m_preproc->error("preprocessor expression is too long! No more stack space.");
        }

        // Try the math consts first, if they are enabled:
        int defined = 0;
        if (flags & eval_flags::allow_math_consts)
        {
            defined = (find_math_const(t->as_string()) != nullptr);
        }

        // Math consts disabled or not a math const, check the user #defines:
        if (!defined)
        {
            defined = m_preproc->is_defined(t->as_string());
        }

        v->value.as_int = defined;
        v->value.type   = eval_type_int;
        v->parentheses  = parentheses_count;
        list_add_link(first_value, last_value, v);

        if (open_par) // Expect the closing parentheses:
        {
            t = next_token();
            if (t == nullptr || !lexer::is_punctuation_token(*t, lexer::punctuation_id::close_parentheses))
            {
                return m_preproc->error("preprocessor \'defined\' directive missing closing parentheses.");
            }
        }

        return true;
    }

    template<typename T>
    static void set_value(eval_value * out_val, const T val, const eval_value_type type) noexcept
    {
        switch (type)
        {
        case eval_type_int :
            out_val->as_int = static_cast<std::int64_t>(val);
            break;

        case eval_type_double :
            out_val->as_double = static_cast<double>(val);
            break;
        } // switch (type)
        out_val->type = type;
    }

    bool apply_op_int(eval_value * out_result, const std::int64_t lhs, const std::int64_t rhs, const lexer::punctuation_id op)
    {
        PREPROCESSOR_ASSERT(out_result != nullptr);

        const bool is_division = (op == lexer::punctuation_id::div || op == lexer::punctuation_id::mod);
        if (is_division && rhs == 0)
        {
            return m_preproc->error("integer division by zero in preprocessor expression!");
        }

        switch (op)
        {
        case lexer::punctuation_id::add              : { set_value(out_result, lhs  + rhs, eval_type_int); break; }
        case lexer::punctuation_id::sub              : { set_value(out_result, lhs  - rhs, eval_type_int); break; }
        case lexer::punctuation_id::mul              : { set_value(out_result, lhs  * rhs, eval_type_int); break; }
        case lexer::punctuation_id::div              : { set_value(out_result, lhs  / rhs, eval_type_int); break; }
        case lexer::punctuation_id::mod              : { set_value(out_result, lhs  % rhs, eval_type_int); break; }
        case lexer::punctuation_id::rshift           : { set_value(out_result, lhs >> rhs, eval_type_int); break; }
        case lexer::punctuation_id::lshift           : { set_value(out_result, lhs << rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_and        : { set_value(out_result, lhs && rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_or         : { set_value(out_result, lhs || rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_eq         : { set_value(out_result, lhs == rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_not_eq     : { set_value(out_result, lhs != rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_greater    : { set_value(out_result, lhs  > rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_less       : { set_value(out_result, lhs  < rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_greater_eq : { set_value(out_result, lhs >= rhs, eval_type_int); break; }
        case lexer::punctuation_id::logic_less_eq    : { set_value(out_result, lhs <= rhs, eval_type_int); break; }
        case lexer::punctuation_id::bitwise_and      : { set_value(out_result, lhs  & rhs, eval_type_int); break; }
        case lexer::punctuation_id::bitwise_or       : { set_value(out_result, lhs  | rhs, eval_type_int); break; }
        case lexer::punctuation_id::bitwise_xor      : { set_value(out_result, lhs  ^ rhs, eval_type_int); break; }
        default :
            {
                return m_preproc->error("operator \'" + lexer::get_punctuation_from_id(op) +
                                        "\' is not legal in integer preprocessor expression!");
            }
        } // switch (op)
        return true;
    }

    bool apply_op_double(eval_value * out_result, const double lhs, const double rhs, const lexer::punctuation_id op)
    {
        PREPROCESSOR_ASSERT(out_result != nullptr);

        if (op == lexer::punctuation_id::div && rhs == 0.0)
        {
            return m_preproc->error("floating-point division by zero in preprocessor expression!");
        }

        // Relational and logical ops output an integer (boolean).
        // Arithmetical ops output double.
        switch (op)
        {
        case lexer::punctuation_id::add              : { set_value(out_result, lhs  + rhs, eval_type_double); break; }
        case lexer::punctuation_id::sub              : { set_value(out_result, lhs  - rhs, eval_type_double); break; }
        case lexer::punctuation_id::mul              : { set_value(out_result, lhs  * rhs, eval_type_double); break; }
        case lexer::punctuation_id::div              : { set_value(out_result, lhs  / rhs, eval_type_double); break; }
        case lexer::punctuation_id::logic_and        : { set_value(out_result, lhs && rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_or         : { set_value(out_result, lhs || rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_eq         : { set_value(out_result, lhs == rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_not_eq     : { set_value(out_result, lhs != rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_greater    : { set_value(out_result, lhs  > rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_less       : { set_value(out_result, lhs  < rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_greater_eq : { set_value(out_result, lhs >= rhs, eval_type_int);    break; }
        case lexer::punctuation_id::logic_less_eq    : { set_value(out_result, lhs <= rhs, eval_type_int);    break; }
        default :
            {
                return m_preproc->error("operator \'" + lexer::get_punctuation_from_id(op) +
                                        "\' is not legal in floating-point preprocessor expression!");
            }
        } // switch (op)
        return true;
    }

    bool resolve_subexpr(eval_value * out_result, const eval_value lhs, const eval_value rhs, const lexer::punctuation_id op)
    {
        //
        // Common case - same types left and right:
        //
        if (lhs.type == eval_type_int && rhs.type == eval_type_int)
        {
            return apply_op_int(out_result, lhs.as_int, rhs.as_int, op);
        }
        if (lhs.type == eval_type_double && rhs.type == eval_type_double)
        {
            return apply_op_double(out_result, lhs.as_double, rhs.as_double, op);
        }

        //
        // double <=> int combos:
        //
        if (lhs.type == eval_type_double && rhs.type == eval_type_int)
        {
            return apply_op_double(out_result, lhs.as_double, rhs.as_int, op);
        }
        if (lhs.type == eval_type_int && rhs.type == eval_type_double)
        {
            return apply_op_double(out_result, lhs.as_int, rhs.as_double, op);
        }

        // Not supposed to be reached.
        return m_preproc->error("unexpected types in preprocessor expression!");
    }

    bool token_to_value(eval_value * out_val, const lexer::token * tok, const bool negate_it)
    {
        if (tok->is_integer() || tok->is_boolean())
        {
            out_val->type = eval_type_int;
            out_val->as_int = tok->as_int64() * (negate_it ? -1 : 1);
            return true;
        }

        if (tok->is_float())
        {
            out_val->type = eval_type_double;
            out_val->as_double = tok->as_double() * (negate_it ? -1.0 : 1.0);
            return true;
        }

        return m_preproc->error("expected number or boolean value in preprocessor expression, "
                                "got \'" + tok->as_string() + "\'.");
    }

    static void value_to_token(lexer::token * out_token, const eval_value & val, const std::uint32_t flags)
    {
        std::uint32_t token_flags = 0;
        char num_str[PREPROC_NUMBUF_SIZE] = {'\0'};

        if (flags & eval_flags::force_int_type) // Force the result to integer?
        {
            token_flags |= lexer::token::flags::integer;
            token_flags |= lexer::token::flags::decimal;
            token_flags |= lexer::token::flags::signed_integer;
        }
        else if (flags & eval_flags::force_float_type) // Force the result to floating-point?
        {
            token_flags |= lexer::token::flags::floating_point;
            token_flags |= lexer::token::flags::double_precision;
        }
        else // Use the eval_value type:
        {
            switch (val.type)
            {
            case eval_type_int :
                token_flags |= lexer::token::flags::integer;
                token_flags |= lexer::token::flags::decimal;
                token_flags |= lexer::token::flags::signed_integer;
                break;

            case eval_type_double :
                token_flags |= lexer::token::flags::floating_point;
                token_flags |= lexer::token::flags::double_precision;
                break;
            } // switch (val.type)
        }

        switch (val.type)
        {
        case eval_type_int :
            if (flags & eval_flags::force_float_type) // Promote to double if necessary
            {
                std::snprintf(num_str, sizeof(num_str), " %f ",
                              static_cast<double>(val.as_int));
            }
            else
            {
                std::snprintf(num_str, sizeof(num_str), " " PREPROC_INT64_FMT " ",
                              val.as_int);
            }
            break;

        case eval_type_double :
            if (flags & eval_flags::force_int_type) // Truncate to integer if necessary
            {
                std::snprintf(num_str, sizeof(num_str), " " PREPROC_INT64_FMT " ",
                              static_cast<std::int64_t>(val.as_double));
            }
            else
            {
                std::snprintf(num_str, sizeof(num_str), " " PREPROC_FLOAT_FMT " ",
                              val.as_double);
            }
            break;
        } // switch (val.type)

        out_token->set_type(lexer::token::type::number);
        out_token->set_flags(token_flags);
        out_token->set_string(num_str);
    }
};

// ========================================================
// expr_evaluator built-in math functions and constants:
// ========================================================

static constexpr double preproc_pi  = 3.14159265358979323846;
static constexpr double preproc_e   = 2.71828182845904523536;
static constexpr double preproc_tau = 2.0 * preproc_pi;

const expr_evaluator::math_func expr_evaluator::builtin_math_funcs[]
{
    { "abs",   &std::fabs  },
    { "sqrt",  &std::sqrt  },
    { "sin",   &std::sin   },
    { "cos",   &std::cos   },
    { "tan",   &std::tan   },
    { "asin",  &std::asin  },
    { "acos",  &std::acos  },
    { "atan",  &std::atan  },
    { "ceil",  &std::ceil  },
    { "floor", &std::floor },
    { "round", &std::round },
    { "exp",   &std::exp   },
    { "exp2",  &std::exp2  },
    { "ln",    &std::log   },
    { "log2",  &std::log2  },
    { "log10", &std::log10 },

    // List terminator
    { nullptr, nullptr }
};

const expr_evaluator::math_const expr_evaluator::builtin_math_consts[]
{
    { "PI",      preproc_pi         },
    { "E",       preproc_e          },
    { "TAU",     preproc_tau        },
    { "INV_TAU", 1.0 / preproc_tau  },
    { "HALF_PI", preproc_pi / 2.0   },
    { "INV_PI",  1.0 / preproc_pi   },
    { "DEG2RAD", preproc_pi / 180.0 },
    { "RAD2DEG", 180.0 / preproc_pi },

    // List terminator
    { nullptr, 0.0 }
};

// ========================================================
// macro_parameter_pack helper:
// ========================================================

class preprocessor::macro_parameter_pack final
{
public:

    explicit macro_parameter_pack(lexer * script) noexcept
        : m_current_script{ script }
    { }

    macro_parameter_pack(const lexer::token * const tokens,
                         const std::uint32_t count) noexcept
        : m_macro_tokens{ tokens }
        , m_tokens_available{ count }
    { }

    macro_parameter_pack(const std::vector<lexer::token> * const params_provided,
                         const lexer::token * const params_names,
                         const std::uint32_t params_name_count,
                         const bool va_args_macro) noexcept
        : m_macro_tokens{ params_names }
    {
        if (params_provided != nullptr)
        {
            m_tokens_available    = params_provided->size();
            m_macro_token_expands = params_provided->data();

            if (va_args_macro)
            {
                m_macro_tokens = params_provided->data();

                // varargs start after the last named parameter.
                m_macro_tokens     += params_name_count;
                m_tokens_available -= params_name_count;
            }
        }
    }

    bool next_token(lexer::token * out_token)
    {
        bool success = false;

        if (m_current_script != nullptr)
        {
            success = m_current_script->next_token(out_token);
        }
        else if (m_macro_tokens != nullptr)
        {
            if (m_tokens_consumed < m_tokens_available)
            {
                *out_token = m_macro_tokens[m_tokens_consumed];
                success = true;
            }
        }

        if (success)
        {
            ++m_tokens_consumed;
        }
        return success;
    }

    const lexer::token * find_param(const lexer::token & wanted) const
    {
        if (m_macro_tokens == nullptr)
        {
            return nullptr;
        }

        for (std::uint32_t t = 0; t < m_tokens_available; ++t)
        {
            if (m_macro_tokens[t].get_type()  == wanted.get_type() &&
                m_macro_tokens[t].as_string() == wanted.as_string())
            {
                return &m_macro_token_expands[t];
            }
        }
        return nullptr;
    }

    std::uint32_t tokens_consumed() const noexcept
    {
        return m_tokens_consumed;
    }

    std::uint32_t tokens_left() const noexcept
    {
        return m_tokens_available - m_tokens_consumed;
    }

    void reset() noexcept
    {
        m_tokens_consumed = 0;
    }

private:

    lexer              * m_current_script      = nullptr;
    const lexer::token * m_macro_tokens        = nullptr;
    const lexer::token * m_macro_token_expands = nullptr;
    std::uint32_t        m_tokens_available    = 0;
    std::uint32_t        m_tokens_consumed     = 0;
};

// ========================================================
// preprocessor class:
// ========================================================

preprocessor::preprocessor()
{
    macro_define_builtins();
}

preprocessor::~preprocessor()
{
    for (lexer * script : m_dynamic_scripts)
    {
        delete script;
    }
}

preprocessor::preprocessor(preprocessor && other) noexcept
    : m_current_script       { other.m_current_script             }
    , m_flags                { other.m_flags                      }
    , m_skipping_conditional { other.m_skipping_conditional       }
    , m_output_line_len      { other.m_output_line_len            }
    , m_output_max_line_len  { other.m_output_max_line_len        }
    , m_prev_token_type      { other.m_prev_token_type            }
    , m_macros               { std::move(other.m_macros)          }
    , m_macro_tokens         { std::move(other.m_macro_tokens)    }
    , m_cond_stack           { std::move(other.m_cond_stack)      }
    , m_include_stack        { std::move(other.m_include_stack)   }
    , m_dynamic_scripts      { std::move(other.m_dynamic_scripts) }
    , m_search_paths         { std::move(other.m_search_paths)    }
{
    other.m_dynamic_scripts.clear(); // Clear here so they are not deleted next.
    other.clear();
}

preprocessor & preprocessor::operator = (preprocessor && other) noexcept
{
    m_current_script       = other.m_current_script;
    m_flags                = other.m_flags;
    m_skipping_conditional = other.m_skipping_conditional;
    m_output_line_len      = other.m_output_line_len;
    m_output_max_line_len  = other.m_output_max_line_len;
    m_prev_token_type      = other.m_prev_token_type;
    m_macros               = std::move(other.m_macros);
    m_macro_tokens         = std::move(other.m_macro_tokens);
    m_cond_stack           = std::move(other.m_cond_stack);
    m_include_stack        = std::move(other.m_include_stack);
    m_dynamic_scripts      = std::move(other.m_dynamic_scripts);
    m_search_paths         = std::move(other.m_search_paths);

    other.m_dynamic_scripts.clear(); // Clear here so they are not deleted next.
    other.clear();
    return *this;
}

void preprocessor::clear()
{
    for (lexer * script : m_dynamic_scripts)
    {
        delete script;
    }

    m_current_script       = nullptr;
    m_skipping_conditional = 0;
    m_output_line_len      = 0;
    m_prev_token_type      = lexer::token::type::none;

    m_macros.clear();
    m_macro_tokens.clear();
    m_cond_stack.clear();
    m_include_stack.clear();
    m_dynamic_scripts.clear();

    macro_define_builtins();
}

bool preprocessor::error(const std::string & message)
{
    if (m_current_script != nullptr)
    {
        return m_current_script->error(message);
    }
    return false;
}

void preprocessor::warning(const std::string & message)
{
    if (m_current_script != nullptr)
    {
        m_current_script->warning(message);
    }
}

bool preprocessor::is_initialized() const noexcept
{
    return m_current_script != nullptr;
}

bool preprocessor::allow_dollar_preproc() const noexcept
{
    return !(m_flags & flags::no_dollar_preproc);
}

bool preprocessor::allow_base_includes() const noexcept
{
    return !(m_flags & flags::no_base_includes);
}

bool preprocessor::allow_includes() const noexcept
{
    return !(m_flags & flags::no_includes);
}

void preprocessor::enable_warnings() noexcept
{
    if (m_current_script != nullptr)
    {
        m_flags &= ~flags::no_warnings;
        m_current_script->set_flags(m_current_script->get_flags() & (~lexer::flags::no_warnings));
    }
}

void preprocessor::disable_warnings() noexcept
{
    if (m_current_script != nullptr)
    {
        m_flags |= flags::no_warnings;
        m_current_script->set_flags(m_current_script->get_flags() | lexer::flags::no_warnings);
    }
}

std::uint32_t preprocessor::get_max_output_line_length() const noexcept
{
    return m_output_max_line_len;
}

void preprocessor::set_max_output_line_length(const std::uint32_t new_value) noexcept
{
    m_output_max_line_len = new_value;
}

lexer * preprocessor::get_current_script() noexcept
{
    return m_current_script;
}

const lexer * preprocessor::get_current_script() const noexcept
{
    return m_current_script;
}

bool preprocessor::add_default_search_path(std::string path, const char path_separator)
{
    if (path.empty())
    {
        return false;
    }

    // Should end in a path separator.
    if (path.back() != path_separator)
    {
        path.push_back(path_separator);
    }

    m_search_paths.emplace_back(std::move(path));
    return true;
}

void preprocessor::clear_default_search_paths()
{
    m_search_paths.clear();
}

bool preprocessor::init_from_file(std::string filename, const std::uint32_t flags, const bool silent)
{
    if (m_current_script != nullptr)
    {
        return (!silent ? error("preprocessor::init_from_file() -> another script is already loaded!") : false);
    }

    m_flags = flags;
    std::uint32_t lex_flags = lexer::flags::no_string_concat;

    if (m_flags != 0)
    {
        // Error/warning flags override the lexer settings.
        if (m_flags & flags::no_errors)
        {
            lex_flags |= lexer::flags::no_errors;
        }
        if (m_flags & flags::no_warnings)
        {
            lex_flags |= lexer::flags::no_warnings;
        }
        if (m_flags & flags::no_fatal_errors)
        {
            lex_flags |= lexer::flags::no_fatal_errors;
        }
    }

    lexer new_script;
    if (!new_script.init_from_file(std::move(filename), lex_flags, silent))
    {
        return false;
    }

    m_dynamic_scripts.push_back(new lexer{ std::move(new_script) });
    m_current_script = m_dynamic_scripts.back();
    return true;
}

bool preprocessor::init_from_memory(const char * const ptr, const std::uint32_t length, std::string filename,
                                    const std::uint32_t flags, const std::uint32_t starting_line)
{
    if (m_current_script != nullptr)
    {
        return error("preprocessor::init_from_memory() -> another script is already loaded!");
    }

    m_flags = flags;
    std::uint32_t lex_flags = lexer::flags::no_string_concat;

    if (m_flags != 0)
    {
        // Error/warning flags override the lexer settings.
        if (m_flags & flags::no_errors)
        {
            lex_flags |= lexer::flags::no_errors;
        }
        if (m_flags & flags::no_warnings)
        {
            lex_flags |= lexer::flags::no_warnings;
        }
        if (m_flags & flags::no_fatal_errors)
        {
            lex_flags |= lexer::flags::no_fatal_errors;
        }
    }

    lexer new_script;
    if (!new_script.init_from_memory(ptr, length, std::move(filename), lex_flags, starting_line))
    {
        return false;
    }

    m_dynamic_scripts.push_back(new lexer{ std::move(new_script) });
    m_current_script = m_dynamic_scripts.back();
    return true;
}

bool preprocessor::init_from_lexer(lexer * initial_script, const std::uint32_t flags)
{
    if (m_current_script != nullptr)
    {
        return error("preprocessor::init_from_lexer() -> another script is already loaded!");
    }

    if (initial_script == nullptr || !initial_script->is_initialized())
    {
        return false;
    }

    m_current_script = initial_script;
    m_flags = flags;

    if (m_flags != 0)
    {
        // Error/warning flags override the lexer settings.
        if (m_flags & flags::no_errors)
        {
            m_current_script->set_flags(m_current_script->get_flags() | lexer::flags::no_errors);
        }
        if (m_flags & flags::no_warnings)
        {
            m_current_script->set_flags(m_current_script->get_flags() | lexer::flags::no_warnings);
        }
        if (m_flags & flags::no_fatal_errors)
        {
            m_current_script->set_flags(m_current_script->get_flags() | lexer::flags::no_fatal_errors);
        }
    }

    return true;
}

bool preprocessor::preprocess(std::string * out_text_buffer)
{
    PREPROCESSOR_ASSERT(out_text_buffer != nullptr);
    if (m_current_script == nullptr)
    {
        return false;
    }

    // Likely for the result to be <= the size of the input,
    // unless there's a lot of conditional code removed or a bunch of #includes...
    out_text_buffer->reserve(m_current_script->get_script_length());

    for (lexer::token tok; ;)
    {
        if (!next_token(&tok))
        {
            if (!m_include_stack.empty())
            {
                // Restore the previous script:
                m_current_script = m_include_stack.back();
                m_include_stack.pop_back();

                // The top of the dynamic scripts stack is the lexer
                // instance for the include file we've just finished.
                PREPROCESSOR_ASSERT(!m_dynamic_scripts.empty());
                m_dynamic_scripts.back()->free_script_source();
                continue;
            }
            else
            {
                break;
            }
        }

        if (check_preproc(tok))
        {
            // Resolve the preprocessor directive and
            // append the result to the output buffer.
            if (!resolve_preproc_and_append(tok, out_text_buffer))
            {
                return false;
            }
            continue;
        }

        // Inside skipped #if/#ifdef/etc block.
        if (m_skipping_conditional > 0)
        {
            continue;
        }

        // Is the token a macro that needs to be expanded first?
        if (tok.is_identifier())
        {
            const auto hashed_name = hash_string(tok.as_string().c_str(), tok.get_length());
            const int  macro_index = macro_find_index(hashed_name);
            if (macro_index >= 0)
            {
                macro_parameter_pack param_pack{ m_current_script };
                if (!expand_macro_and_append(macro_index, out_text_buffer, &param_pack, nullptr))
                {
                    return false;
                }
                continue;
            }
        }

        // Append the raw token to the output buffer.
        output_append_token_text(tok, out_text_buffer);
    }

    PREPROCESSOR_ASSERT(m_include_stack.empty());
    return true;
}

bool preprocessor::next_token(lexer::token * out_token)
{
    if (m_current_script != nullptr)
    {
        return m_current_script->next_token(out_token);
    }
    return false;
}

bool preprocessor::next_token_on_line(lexer::token * out_token)
{
    if (m_current_script != nullptr)
    {
        return m_current_script->next_token_on_line(out_token);
    }
    return false;
}

void preprocessor::unget_token(const lexer::token & tok)
{
    if (m_current_script != nullptr)
    {
        m_current_script->unget_token(tok);
    }
}

bool preprocessor::skip_rest_of_line()
{
    if (m_current_script != nullptr)
    {
        return m_current_script->skip_rest_of_line();
    }
    return false;
}

bool preprocessor::read_line(std::string * out_line)
{
    PREPROCESSOR_ASSERT(out_line != nullptr);

    lexer::token tok;
    bool got_backlash = false;

    // Reads tokens until the end of a line is encountered.
    // If the last token was a '\', continue scanning the next line.
    while (next_token(&tok))
    {
        if (lexer::is_punctuation_token(tok, lexer::punctuation_id::backslash))
        {
            got_backlash = true;
            continue;
        }

        if (!got_backlash && tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
            break;
        }

        out_line->append(tok.as_string());
        got_backlash = false;
    }

    return true;
}

bool preprocessor::check_preproc(const lexer::token & tok) const noexcept
{
    if (lexer::is_punctuation_token(tok, lexer::punctuation_id::preprocessor)) // #
    {
        return true;
    }
    else if (lexer::is_punctuation_token(tok, lexer::punctuation_id::dollar_sign) && allow_dollar_preproc()) // $
    {
        return true;
    }
    else // Not a # preprocessor or $ preprocessor extensions are disabled.
    {
        return false;
    }
}

bool preprocessor::resolve_preproc_and_append(const lexer::token & tok, std::string * out_text_buffer)
{
    PREPROCESSOR_ASSERT(out_text_buffer != nullptr);
    PREPROCESSOR_ASSERT(tok.is_punctuation()); // # or $

    if (lexer::is_punctuation_token(tok, lexer::punctuation_id::preprocessor)) // #
    {
        return resolve_hash_directive();
    }
    else if (lexer::is_punctuation_token(tok, lexer::punctuation_id::dollar_sign) && allow_dollar_preproc()) // $
    {
        return resolve_dollar_directive(out_text_buffer);
    }
    else // Not normally reached if check_preproc() is working as it should!
    {
        return false;
    }
}

bool preprocessor::resolve_hash_directive()
{
    lexer::token tok;
    if (!next_token(&tok))
    {
        return error("found preprocessor directive without a following command!");
    }

    if (tok.get_lines_crossed() != 0) // Directive command name must be on the same line!
    {
        unget_token(tok);
        return error("preprocessor command found at end of line!");
    }

    if (!tok.is_identifier())
    {
        return error("invalid preprocessor directive \'" + tok.as_string() + "\'. " +
                     "Expected an identifier after the preprocessor symbol!");
    }

    //
    // Handle each of the supported # directives:
    //
    if (tok == "if")
    {
        return resolve_if_directive();
    }
    else if (tok == "ifdef")
    {
        return resolve_ifdef_directive();
    }
    else if (tok == "ifndef")
    {
        return resolve_ifndef_directive();
    }
    else if (tok == "elif")
    {
        return resolve_elif_directive();
    }
    else if (tok == "else")
    {
        return resolve_else_directive();
    }
    else if (tok == "endif")
    {
        return resolve_endif_directive();
    }
    else if (m_skipping_conditional > 0)
    {
        skip_rest_of_line();
        return true;
    }
    else
    {
        if (tok == "include")
        {
            return resolve_include_directive();
        }
        else if (tok == "define")
        {
            return resolve_define_directive();
        }
        else if (tok == "undef")
        {
            return resolve_undef_directive();
        }
        else if (tok == "line")
        {
            return resolve_line_directive();
        }
        else if (tok == "error")
        {
            return resolve_error_directive();
        }
        else if (tok == "warning" || tok == "warn") // Note: #warn is an extension to the C language.
        {
            return resolve_warning_directive();
        }
        else if (tok == "pragma")
        {
            return resolve_pragma_directive();
        }
    }

    //
    // Unknown directive:
    //

    // Custom message for the common error of mismatching
    // the # and $ in one of the extended directives:
    const char * dollar_directives[]{ "eval", "evalint", "evalfloat" };
    for (const char * d : dollar_directives)
    {
        if (tok.as_string() == d)
        {
            return error("\'" + tok.as_string() + "\' preprocessor directive must be preceded by \'" +
                         lexer::get_punctuation_from_id(lexer::punctuation_id::dollar_sign) + "\'.");
        }
    }

    // Generic error message:
    return error("unknown preprocessor directive \'" + tok.as_string() + "\'.");
}

bool preprocessor::resolve_dollar_directive(std::string * out_text_buffer)
{
    lexer::token tok;
    if (!next_token(&tok))
    {
        return error("found preprocessor directive without a following command!");
    }

    if (tok.get_lines_crossed() != 0) // Directive command name must be on the same line!
    {
        unget_token(tok);
        return error("preprocessor command found at end of line!");
    }

    // $eval() can contain the predefined math functions and constants.
    std::uint32_t flags = (expr_evaluator::eval_flags::allow_math_funcs |
                           expr_evaluator::eval_flags::allow_math_consts);

    if (tok == "eval")
    {
        // Type of result inferred from inputs.
        flags |= expr_evaluator::eval_flags::detect_type;
    }
    else if (tok == "evalint")
    {
        // Result is cast to integer.
        flags |= expr_evaluator::eval_flags::force_int_type;
    }
    else if (tok == "evalfloat")
    {
        // Result is cast to floating-point.
        flags |= expr_evaluator::eval_flags::force_float_type;
    }
    else
    {
        return error("expected \'eval\', \'evalint\' or \'evalfloat\' after \'" +
                     lexer::get_punctuation_from_id(lexer::punctuation_id::dollar_sign) +
                     "\' preprocessor directive!");
    }

    // Expect and opening parentheses:
    if (!next_token(&tok) ||
        !lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
    {
        return error("expected \'" + lexer::get_punctuation_from_id(lexer::punctuation_id::open_parentheses) +
                     "\' after \'eval\' directive!");
    }

    int parentheses_depth = 1;
    expr_evaluator evaluator{ this };

    // Collect the tokens:
    while (next_token(&tok))
    {
        if (lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
        {
            ++parentheses_depth;
        }
        else if (lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
        {
            if (--parentheses_depth == 0)
            {
                break;
            }
        }
        // Parentheses are also added.
        evaluator.push_token(std::move(tok));
    }

    // Expect the closing parentheses:
    if (!lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
    {
        return error("expected \'" + lexer::get_punctuation_from_id(lexer::punctuation_id::close_parentheses) +
                     "\' at the end of \'eval\' directive!");
    }

    tok.clear();
    if (!evaluator.evaluate(&tok, nullptr, flags))
    {
        return false;
    }

    output_append_token_text(tok, out_text_buffer);
    return true;
}

std::uint32_t preprocessor::hash_string(const char * const str, const std::size_t count)
{
    PREPROCESSOR_ASSERT(str != nullptr);

    //
    // Simple and fast One-at-a-Time (OAT) hash algorithm:
    //  http://en.wikipedia.org/wiki/Jenkins_hash_function
    //
    // String not required to be null-terminated.
    //
    std::uint32_t h = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        h += str[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
    return h;
}

bool preprocessor::define(const std::string & macro_name, lexer::token value, const bool allow_redefinition)
{
    const auto hashed_name = hash_string(macro_name.c_str(), macro_name.length());
    const int  macro_index = macro_find_index(hashed_name);

    macro_def new_macro;
    new_macro.hashed_name           = hashed_name;
    new_macro.first_param_token     = 0;
    new_macro.param_token_count     = 0;
    new_macro.first_body_token      = static_cast<std::uint32_t>(m_macro_tokens.size());
    new_macro.body_token_count      = 1;
    new_macro.empty_func_like_macro = false;
    new_macro.va_args_macro         = false;

    if (macro_index >= 0) // Redefined:
    {
        if (!allow_redefinition)
        {
            return false;
        }

        macro_clear_tokens(m_macros[macro_index]);
        m_macros[macro_index] = new_macro;
    }
    else // New definition:
    {
        m_macros.push_back(new_macro);
    }

    m_macro_tokens.emplace_back(std::move(value));
    return true;
}

bool preprocessor::define(const std::string & macro_name, std::string value, const bool allow_redefinition)
{
    lexer::token value_token;
    value_token.set_type(lexer::token::type::string);
    value_token.set_string(std::move(value));
    return define(macro_name, std::move(value_token), allow_redefinition);
}

bool preprocessor::define(const std::string & macro_name, const std::int64_t value, const bool allow_redefinition)
{
    lexer::token value_token;
    value_token.set_type(lexer::token::type::number);
    value_token.set_flags(lexer::token::flags::integer |
                          lexer::token::flags::decimal |
                          lexer::token::flags::signed_integer);

    char num_str[PREPROC_NUMBUF_SIZE] = {'\0'};
    std::snprintf(num_str, sizeof(num_str), PREPROC_INT64_FMT, value);
    value_token.set_string(num_str);

    return define(macro_name, std::move(value_token), allow_redefinition);
}

bool preprocessor::define(const std::string & macro_name, const double value, const bool allow_redefinition)
{
    lexer::token value_token;
    value_token.set_type(lexer::token::type::number);
    value_token.set_flags(lexer::token::flags::floating_point |
                          lexer::token::flags::double_precision);

    char num_str[PREPROC_NUMBUF_SIZE] = {'\0'};
    std::snprintf(num_str, sizeof(num_str), PREPROC_FLOAT_FMT, value);
    value_token.set_string(num_str);

    return define(macro_name, std::move(value_token), allow_redefinition);
}

bool preprocessor::define(const std::string & define_string, const bool allow_redefinition)
{
    if (define_string.empty())
    {
        return false;
    }

    lexer lex;
    lexer::token tok;
    const std::uint32_t lex_flags = (lexer::flags::no_fatal_errors |
                                     lexer::flags::no_errors       |
                                     lexer::flags::no_warnings     |
                                     lexer::flags::no_string_concat);

    // Tokenize the string:
    if (!lex.init_from_memory(define_string.c_str(), define_string.length(), "(define-string)", lex_flags))
    {
        return false;
    }

    // String should have started with "#define"
    if (!lex.next_token(&tok) || !lexer::is_punctuation_token(tok, lexer::punctuation_id::preprocessor))
    {
        return false;
    }
    if (!lex.next_token(&tok) || tok != "define")
    {
        return false;
    }

    if (!allow_redefinition)
    {
        if (!lex.next_token(&tok) || is_defined(tok.as_string()))
        {
            return false;
        }
        lex.unget_token(tok);
    }

    auto old_script = m_current_script;

    m_current_script = &lex;
    resolve_define_directive();

    m_current_script = old_script;
    return true;
}

bool preprocessor::is_defined(const std::string & macro_name) const
{
    const auto hashed_name = hash_string(macro_name.c_str(), macro_name.length());
    return macro_find_index(hashed_name) >= 0;
}

void preprocessor::undef(const std::string & macro_name)
{
    macro_undefine(macro_name);
}

void preprocessor::undef_all(const bool keep_built_ins)
{
    m_macros.clear();
    m_macro_tokens.clear();

    if (keep_built_ins) // Restore the built-in macros:
    {
        macro_define_builtins();
    }
}

bool preprocessor::find_macro_token(const std::string & macro_name, lexer::token * out_token) const
{
    const auto hashed_name = hash_string(macro_name.c_str(), macro_name.length());
    const int  macro_index = macro_find_index(hashed_name);

    if (macro_index < 0)
    {
        out_token->clear();
        return false;
    }

    const macro_def macro = m_macros[macro_index];
    if (macro.param_token_count != 0 || macro.body_token_count != 1)
    {
        out_token->clear();
        return false; // Only simple '#define FOO bar' macros are allowed.
    }

    *out_token = m_macro_tokens[macro.first_body_token];
    return true;
}

const lexer::token * preprocessor::find_macro_tokens(const std::string & macro_name, int * out_num_tokens) const
{
    const auto hashed_name = hash_string(macro_name.c_str(), macro_name.length());
    const int  macro_index = macro_find_index(hashed_name);

    if (macro_index < 0)
    {
        if (out_num_tokens != nullptr) { *out_num_tokens = 0; }
        return nullptr;
    }

    const macro_def macro = m_macros[macro_index];
    if (out_num_tokens != nullptr)
    {
        *out_num_tokens = static_cast<int>(macro.body_token_count);
    }

    if (macro.body_token_count != 0)
    {
        return &m_macro_tokens[macro.first_body_token];
    }
    else
    {
        return nullptr;
    }
}

bool preprocessor::find_macro_value(const std::string & macro_name, std::string * out_value) const
{
    PREPROCESSOR_ASSERT(out_value != nullptr);

    lexer::token tok;
    if (!find_macro_token(macro_name, &tok))
    {
        return false;
    }

    tok.move_to(out_value);
    return true;
}

bool preprocessor::find_macro_value(const std::string & macro_name, std::int64_t * out_value) const
{
    PREPROCESSOR_ASSERT(out_value != nullptr);

    lexer::token tok;
    if (!find_macro_token(macro_name, &tok) || !tok.is_number())
    {
        return false;
    }

    *out_value = tok.as_int64();
    return true;
}

bool preprocessor::find_macro_value(const std::string & macro_name, double * out_value) const
{
    PREPROCESSOR_ASSERT(out_value != nullptr);

    lexer::token tok;
    if (!find_macro_token(macro_name, &tok) || !tok.is_number())
    {
        return false;
    }

    *out_value = tok.as_double();
    return true;
}

bool preprocessor::macro_is_builtin(const macro_def & macro) const noexcept
{
    switch (macro.hashed_name)
    {
    case builtin_macro_file    : return true;
    case builtin_macro_line    : return true;
    case builtin_macro_date    : return true;
    case builtin_macro_time    : return true;
    case builtin_macro_va_args : return true;
    default                    : return false;
    } // switch (macro.hashed_name)
}

void preprocessor::macro_expand_builtin(const macro_def & macro, std::string * out_text_buffer, macro_parameter_pack * va_args)
{
    PREPROCESSOR_ASSERT(out_text_buffer != nullptr);

    if (m_current_script == nullptr)
    {
        error("no script loaded!");
        return;
    }

    // Output as quoted strings, except for the __LINE__ number and varargs.
    switch (macro.hashed_name)
    {
    case builtin_macro_file :
        {
            out_text_buffer->push_back('"');
            out_text_buffer->append(m_current_script->get_filename());
            out_text_buffer->push_back('"');
            break;
        }
    case builtin_macro_line :
        {
            const auto lineno_str = std::to_string(m_current_script->get_line_number());
            out_text_buffer->append(lineno_str);
            break;
        }
    case builtin_macro_date :
        {
            // Expected ctime output format:
            // Www Mmm dd hh:mm:ss yyyy
            const std::time_t tval = std::time(nullptr);
            const char * time_str  = std::ctime(&tval);

            if (time_str != nullptr && std::strlen(time_str) >= 24)
            {
                // We only want these three:
                const std::string month{ time_str + 4,  4 };
                const std::string day  { time_str + 8,  3 };
                const std::string year { time_str + 20, 4 };

                out_text_buffer->push_back('"');
                out_text_buffer->append(month + day + year);
                out_text_buffer->push_back('"');
            }
            break;
        }
    case builtin_macro_time :
        {
            // Expected ctime output format:
            // Www Mmm dd hh:mm:ss yyyy
            const std::time_t tval = std::time(nullptr);
            const char * time_str  = std::ctime(&tval);

            if (time_str != nullptr && std::strlen(time_str) >= 24)
            {
                const std::string daytime{ time_str + 11, 8 };

                out_text_buffer->push_back('"');
                out_text_buffer->append(daytime);
                out_text_buffer->push_back('"');
            }
            break;
        }
    case builtin_macro_va_args :
        {
            if (va_args == nullptr)
            {
                error("\'__VA_ARGS__\' macro expansion failed!");
                break;
            }

            lexer::token tok;
            while (va_args->next_token(&tok))
            {
                string_append_token(tok, out_text_buffer);
                if (va_args->tokens_left() != 0)
                {
                    out_text_buffer->push_back(',');
                    out_text_buffer->push_back(' ');
                }
            }
            va_args->reset();
            break;
        }
    default :
        {
            error("undefined built-in macro expansion!");
            break;
        }
    } // switch (macro.hashed_name)
}

void preprocessor::macro_define_builtins()
{
    macro_def builtin{};

    builtin.hashed_name = builtin_macro_file;
    m_macros.push_back(builtin);

    builtin.hashed_name = builtin_macro_line;
    m_macros.push_back(builtin);

    builtin.hashed_name = builtin_macro_date;
    m_macros.push_back(builtin);

    builtin.hashed_name = builtin_macro_time;
    m_macros.push_back(builtin);

    builtin.hashed_name = builtin_macro_va_args;
    m_macros.push_back(builtin);
}

int preprocessor::macro_find_index(const std::uint32_t hashed_macro_name) const noexcept
{
    const int macro_count = static_cast<int>(m_macros.size());

    for (int i = 0; i < macro_count; ++i)
    {
        if (m_macros[i].hashed_name == hashed_macro_name)
        {
            return i;
        }
    }

    return -1;
}

void preprocessor::macro_define(const std::string & macro_name, macro_def * new_macro)
{
    new_macro->hashed_name = hash_string(macro_name.c_str(), macro_name.length());
    const int  macro_index = macro_find_index(new_macro->hashed_name);

    if (macro_index >= 0) // Redefined:
    {
        if (m_flags & flags::warn_macro_redefinitions)
        {
            warning("macro \'" + macro_name + "\' is already defined and will be overwritten.");
        }

        macro_clear_tokens(m_macros[macro_index]);
        m_macros[macro_index] = *new_macro;
    }
    else // New definition:
    {
        m_macros.push_back(*new_macro);
    }
}

void preprocessor::macro_undefine(const std::string & macro_name)
{
    if (m_macros.empty())
    {
        return;
    }

    const auto hashed_name = hash_string(macro_name.c_str(), macro_name.length());
    const int  macro_index = macro_find_index(hashed_name);

    if (macro_index >= 0)
    {
        // This will ensure token strings release any allocated heap memory.
        macro_clear_tokens(m_macros[macro_index]);

        // Swap with the last item, so we don't have to shift the array.
        const int last_macro = static_cast<int>(m_macros.size() - 1);
        if (macro_index != last_macro)
        {
            m_macros[macro_index] = m_macros[last_macro];
        }
        m_macros.pop_back();
    }

    // Note that the macro parameters and body tokens, if it had any,
    // will remain allocated in m_macro_tokens. Since macro #undefing
    // is not a frequent thing, it is better to just leave that stale
    // memory in the array than shifting the vector to erase entries
    // taken by the tokens of the removed macro.
}

void preprocessor::macro_clear_tokens(const macro_def & macro)
{
    const lexer::token empty_token{};
    for (std::uint32_t i = 0; i < macro.param_token_count; ++i)
    {
        m_macro_tokens[i + macro.first_param_token] = empty_token;
    }
    for (std::uint32_t i = 0; i < macro.body_token_count; ++i)
    {
        m_macro_tokens[i + macro.first_body_token] = empty_token;
    }
}

bool preprocessor::expand_macro_and_append(const int macro_index, std::string * out_text_buffer,
                                           macro_parameter_pack * param_pack, macro_parameter_pack * parent_pack)
{
    PREPROCESSOR_ASSERT(out_text_buffer != nullptr);
    PREPROCESSOR_ASSERT(param_pack      != nullptr);

    lexer::token tok;
    const macro_def macro = m_macros[macro_index];

    // Function-like macros taking one or more parameters (including ... varargs):
    if (macro.va_args_macro || macro.param_token_count != 0)
    {
        bool expect_comma      = false;
        int  parentheses_count = 0;

        std::string current_param_text;
        std::vector<lexer::token> params_provided;

        if (!param_pack->next_token(&tok) ||
            !lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
        {
            return error("missing opening parenthesis is function-like macro instantiation.");
        }

        ++parentheses_count;
        params_provided.reserve(macro.param_token_count);

        //
        // Collect the given parameters:
        //
        while (param_pack->next_token(&tok))
        {
            if (lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
            {
                ++parentheses_count;
            }
            else if (lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
            {
                if (--parentheses_count <= 0)
                {
                    break;
                }
            }

            if (expect_comma && lexer::is_punctuation_token(tok, lexer::punctuation_id::comma))
            {
                tok.clear();
                tok.set_type(lexer::token::type::identifier);
                tok.set_string(std::move(current_param_text));

                params_provided.push_back(std::move(tok));
                current_param_text.clear();
                expect_comma = false;
                continue;
            }

            // Parameter itself can also be a previously
            // defined macro we might need to recursively expand.
            if (tok.is_identifier())
            {
                const auto hashed_name = hash_string(tok.as_string().c_str(), tok.get_length());
                const int other_index  = macro_find_index(hashed_name);
                if (other_index >= 0)
                {
                    if (macro_index == other_index)
                    {
                        return error("macro parameter references itself!");
                    }

                    if (m_macros[other_index].hashed_name == builtin_macro_va_args)
                    {
                        // We have to preserve the commas for a __VA_ARGS__ reference in
                        // the parameter list of another macro, so it gets expanded here
                        // instead of using the recursive expansion that writes into the string.
                        PREPROCESSOR_ASSERT(parent_pack != nullptr);
                        while (parent_pack->next_token(&tok))
                        {
                            params_provided.push_back(std::move(tok));
                        }
                        parent_pack->reset();
                        current_param_text.clear();
                    }
                    else
                    {
                        macro_parameter_pack other_param_pack{ m_current_script };
                        if (!expand_macro_and_append(other_index, &current_param_text, &other_param_pack, parent_pack))
                        {
                            return false;
                        }
                    }

                    // Need to get rig of some whitespace added for the other cases.
                    lexer::trim_string(&current_param_text);
                    expect_comma = true;
                    continue;
                }
                else if (parent_pack != nullptr)
                {
                    if (const auto p_tok = parent_pack->find_param(tok))
                    {
                        string_append_token(*p_tok, &current_param_text);
                        current_param_text.push_back(' ');
                        expect_comma = true;
                        continue;
                    }
                }
            }

            string_append_token(tok, &current_param_text);
            current_param_text.push_back(' ');
            expect_comma = true;
        }

        if (!lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
        {
            return error("missing closing parenthesis is function-like macro instantiation.");
        }

        // The last argument:
        if (!current_param_text.empty())
        {
            tok.clear();
            tok.set_type(lexer::token::type::identifier);
            tok.set_string(std::move(current_param_text));
            params_provided.push_back(std::move(tok));
        }

        if (!expect_comma)
        {
            return error("trailing comma in macro argument list!");
        }

        if (params_provided.size() != macro.param_token_count)
        {
            if (macro.va_args_macro && params_provided.size() > macro.param_token_count)
            {
                // OK to have more when a varargs macro.
            }
            else
            {
                return error("function-like macro expected " + std::to_string(macro.param_token_count) +
                             " parameters, but got " + std::to_string(params_provided.size()));
            }
        }

        // Check for invalid leading or trailing ## and trailing #
        if (macro.body_token_count != 0)
        {
            const auto & first_tok = m_macro_tokens[macro.first_body_token];
            const auto & last_tok  = m_macro_tokens[macro.first_body_token + macro.body_token_count - 1];

            if (lexer::is_punctuation_token(last_tok, lexer::punctuation_id::preprocessor))
            {
                return error("\'#\' cannot appear at end of macro expansion!");
            }
            if (lexer::is_punctuation_token(first_tok, lexer::punctuation_id::preprocessor_merge))
            {
                return error("\'##\' cannot appear at start of macro expansion!");
            }
            if (lexer::is_punctuation_token(last_tok, lexer::punctuation_id::preprocessor_merge))
            {
                return error("\'##\' cannot appear at end of macro expansion!");
            }
        }

        //
        // Expand it:
        //
        bool token_is_param           = false;
        bool next_is_merge            = false;
        bool prev_token_was_stringize = false;

        out_text_buffer->push_back(' ');
        for (std::uint32_t b = 0; b < macro.body_token_count; ++b)
        {
            const lexer::token & body_token = m_macro_tokens[b + macro.first_body_token];

            if (lexer::is_punctuation_token(body_token, lexer::punctuation_id::preprocessor)) // #
            {
                prev_token_was_stringize = true;
                continue;
            }
            if (lexer::is_punctuation_token(body_token, lexer::punctuation_id::preprocessor_merge)) // ##
            {
                continue;
            }

            // Check for references to other macros in the body of this macro.
            // If we refer an existing macro, re-enter the function to
            // recursively perform the expansions.
            if (body_token.is_identifier())
            {
                const auto hashed_name = hash_string(body_token.as_string().c_str(), body_token.get_length());
                const int other_index  = macro_find_index(hashed_name);
                if (other_index >= 0)
                {
                    const int tokens_consumed = expand_recursive_macro_and_append(
                            macro_index, other_index, b, &params_provided, out_text_buffer);
                    if (tokens_consumed < 0)
                    {
                        return false;
                    }
                    b += tokens_consumed;
                    continue;
                }
            }

            // Check for a merge op ahead to avoid emitting a whitespace between tokens.
            if (b != (macro.body_token_count - 1))
            {
                const lexer::token & next_body_token = m_macro_tokens[b + macro.first_body_token + 1];
                if (lexer::is_punctuation_token(next_body_token, lexer::punctuation_id::preprocessor_merge)) // ##
                {
                    next_is_merge = true;
                }
                else
                {
                    next_is_merge = false;
                }
            }

            // Check if the token is one of the parameters, replace it if so:
            token_is_param = false;
            for (std::uint32_t p = 0; p < macro.param_token_count; ++p)
            {
                const lexer::token & param_name = m_macro_tokens[p + macro.first_param_token];

                if (body_token.is_identifier() && (body_token.as_string() == param_name.as_string()))
                {
                    if (prev_token_was_stringize)
                    {
                        output_append_token_text(params_provided[p].stringize(), out_text_buffer, true, true);
                        out_text_buffer->push_back(' ');
                        prev_token_was_stringize = false;
                    }
                    else if (next_is_merge)
                    {
                        output_append_token_text(params_provided[p].trim(), out_text_buffer, false, true);
                    }
                    else
                    {
                        output_append_token_text(params_provided[p], out_text_buffer, false, true);
                    }

                    token_is_param = true;
                    break;
                }
            }

            if (!token_is_param)
            {
                output_append_token_text(body_token, out_text_buffer, false, true);
                if (!next_is_merge)
                {
                    out_text_buffer->push_back(' '); // Keep spaces between tokens.
                }
            }
        }
        out_text_buffer->push_back(' ');
    }
    else // Just values followed the macro name:
    {
        // Check for built-ins like __FILE__ and __LINE__:
        if (macro_is_builtin(macro))
        {
            macro_expand_builtin(macro, out_text_buffer, parent_pack);
        }
        else // User-defined macro:
        {
            // If it was declared as an empty function-like macro, it must have the empty '( )' pair.
            if (macro.empty_func_like_macro)
            {
                if (!param_pack->next_token(&tok) ||
                    !lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
                {
                    return error("missing opening parenthesis is function-like macro instantiation.");
                }

                if (!param_pack->next_token(&tok) ||
                    !lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
                {
                    if (tok.is_punctuation())
                    {
                        return error("missing closing parenthesis is function-like macro instantiation.");
                    }
                    else
                    {
                        return error("function-like macro takes no arguments!");
                    }
                }
            }

            // Check for invalid leading or trailing # and ##
            if (macro.body_token_count != 0)
            {
                const auto & first_tok = m_macro_tokens[macro.first_body_token];
                const auto & last_tok  = m_macro_tokens[macro.first_body_token + macro.body_token_count - 1];

                if (lexer::is_punctuation_token(first_tok, lexer::punctuation_id::preprocessor))
                {
                    return error("\'#\' cannot appear at start of macro expansion!");
                }
                if (lexer::is_punctuation_token(last_tok, lexer::punctuation_id::preprocessor))
                {
                    return error("\'#\' cannot appear at end of macro expansion!");
                }

                if (lexer::is_punctuation_token(first_tok, lexer::punctuation_id::preprocessor_merge))
                {
                    return error("\'##\' cannot appear at start of macro expansion!");
                }
                if (lexer::is_punctuation_token(last_tok, lexer::punctuation_id::preprocessor_merge))
                {
                    return error("\'##\' cannot appear at end of macro expansion!");
                }
            }

            out_text_buffer->push_back(' ');
            for (std::uint32_t i = 0; i < macro.body_token_count; ++i)
            {
                const lexer::token & body_token = m_macro_tokens[i + macro.first_body_token];

                // Recursive substitution of other macros referenced in the body.
                if (body_token.is_identifier())
                {
                    const auto hashed_name = hash_string(body_token.as_string().c_str(), body_token.get_length());
                    const int other_index  = macro_find_index(hashed_name);
                    if (other_index >= 0)
                    {
                        const int tokens_consumed = expand_recursive_macro_and_append(
                                macro_index, other_index, i, nullptr, out_text_buffer);
                        if (tokens_consumed < 0) { return false; }
                        i += tokens_consumed;
                        continue;
                    }
                }

                output_append_token_text(body_token, out_text_buffer);
                out_text_buffer->push_back(' '); // Keep spaces between tokens.
            }
        }
    }

    return true;
}

int preprocessor::expand_recursive_macro_and_append(const int macro_index, const int other_macro_index, const int token_index,
                                                    const std::vector<lexer::token> * const params_provided, std::string * out_text_buffer)
{
    PREPROCESSOR_ASSERT(out_text_buffer != nullptr);

    // Obviously referencing itself in the body would result in infinite recursion.
    if (macro_index == other_macro_index)
    {
        error("macro expansion references itself!");
        return -1;
    }

    const macro_def macro = m_macros[macro_index];
    const std::uint32_t next_token = token_index + 1;

    macro_parameter_pack param_pack{
        &m_macro_tokens[next_token + macro.first_body_token],
        ((next_token < macro.body_token_count) ? (macro.body_token_count - next_token) : 0)
    };

    macro_parameter_pack parent_pack{
        params_provided,
        ((params_provided != nullptr && macro.param_token_count != 0) ?
                  &m_macro_tokens[macro.first_param_token] : nullptr),
        macro.param_token_count,
        macro.va_args_macro
    };

    if (!expand_macro_and_append(other_macro_index, out_text_buffer, &param_pack, &parent_pack))
    {
        return -1;
    }

    return param_pack.tokens_consumed();
}

void preprocessor::output_append_token_text(const lexer::token & tok, std::string * out_text_buffer,
                                            const bool no_string_escape, const bool no_whitespace)
{
    PREPROCESSOR_ASSERT(out_text_buffer != nullptr);

    if (!no_whitespace && !tok.is_punctuation() && m_prev_token_type != lexer::token::type::punctuation)
    {
        out_text_buffer->push_back(' ');
    }

    if (!no_string_escape)
    {
        string_append_token(tok, out_text_buffer); // Re-escapes string tokens.
    }
    else
    {
        out_text_buffer->append(tok.as_string());
    }

    m_prev_token_type  = tok.get_type();
    m_output_line_len += tok.get_length();

    // We break lines once they get long enough. But only break if at a
    // semicolon, so m_output_max_line_len is not a hard constraint.
    if ((m_output_line_len > m_output_max_line_len) &&
        lexer::is_punctuation_token(tok, lexer::punctuation_id::semicolon))
    {
        out_text_buffer->push_back('\n');
        m_output_line_len = 0;
    }
}

void preprocessor::string_append_token(const lexer::token & tok, std::string * out_str)
{
    PREPROCESSOR_ASSERT(out_str != nullptr);

    if (tok.is_string())
    {
        out_str->push_back('"');

        const std::string & s = tok.as_string();
        const std::size_t len = s.length();

        // Might have to re-escape the string:
        for (std::size_t i = 0; i < len; ++i)
        {
            switch (s[i])
            {
            case '\n' : { out_str->append("\\n");   break; }
            case '\r' : { out_str->append("\\r");   break; }
            case '\t' : { out_str->append("\\t");   break; }
            case '\v' : { out_str->append("\\v");   break; }
            case '\b' : { out_str->append("\\b");   break; }
            case '\f' : { out_str->append("\\f");   break; }
            case '\a' : { out_str->append("\\a");   break; }
            case '\\' : { out_str->append("\\\\");  break; }
            case '\'' : { out_str->append("\\\'");  break; }
            case '\"' : { out_str->append("\\\"");  break; }
            case '\?' : { out_str->append("\\?");   break; }
            default   : { out_str->push_back(s[i]); break; }
            } // switch (s[i])
        }

        out_str->push_back('"');
    }
    else if (tok.is_literal()) // Character literal
    {
        out_str->push_back('\'');

        if (tok.get_length() == 0)
        {
            out_str->append("\\0");
        }
        else
        {
            // Might have to re-escape the character:
            switch (tok[0])
            {
            case '\n' : { out_str->append("\\n");     break; }
            case '\r' : { out_str->append("\\r");     break; }
            case '\t' : { out_str->append("\\t");     break; }
            case '\v' : { out_str->append("\\v");     break; }
            case '\b' : { out_str->append("\\b");     break; }
            case '\f' : { out_str->append("\\f");     break; }
            case '\a' : { out_str->append("\\a");     break; }
            case '\\' : { out_str->append("\\\\");    break; }
            case '\'' : { out_str->append("\\\'");    break; }
            case '\"' : { out_str->append("\\\"");    break; }
            case '\?' : { out_str->append("\\?");     break; }
            default   : { out_str->push_back(tok[0]); break; }
            } // switch (tok[0])
        }

        out_str->push_back('\'');
    }
    else
    {
        out_str->append(tok.as_string());
    }
}

void preprocessor::push_conditional(const conditional_type type, const bool skip_body,
                                    const bool * const opt_parent_state)
{
    m_cond_stack.push_back({ type, skip_body, ((opt_parent_state != nullptr) ? *opt_parent_state : skip_body) });
    m_skipping_conditional += skip_body;
}

bool preprocessor::pop_conditional(conditional_type * out_type, bool * out_skip, bool * out_parent_state)
{
    if (m_cond_stack.empty())
    {
        return false;
    }

    const auto & top = m_cond_stack.back();
    if (out_type != nullptr)
    {
        *out_type = top.type;
    }
    if (out_skip != nullptr)
    {
        *out_skip = top.skip_body;
    }
    if (out_parent_state != nullptr)
    {
        *out_parent_state = top.parent_state;
    }

    m_skipping_conditional -= top.skip_body;
    m_cond_stack.pop_back();
    return true;
}

bool preprocessor::evaluate_preproc_conditional(bool * out_result)
{
    PREPROCESSOR_ASSERT(out_result != nullptr);

    expr_evaluator evaluator{ this };
    expr_evaluator::eval_value expr_result;

    lexer::token tok;
    bool got_backlash = false;
    int parentheses_depth = 0;

    // Reads tokens until the end of a line is encountered.
    // If the last token was a '\', continue scanning the next line.
    while (next_token(&tok))
    {
        if (lexer::is_punctuation_token(tok, lexer::punctuation_id::backslash))
        {
            got_backlash = true;
            continue;
        }
        else if (!got_backlash && tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
            break;
        }
        else if (lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
        {
            ++parentheses_depth;
        }
        else if (lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
        {
            --parentheses_depth;
        }

        evaluator.push_token(std::move(tok));
        got_backlash = false;
    }

    // Error checking:
    if (parentheses_depth != 0)
    {
        return error("unbalanced opening/closing parentheses in #if/elif directive!");
    }
    if (evaluator.get_token_count() == 0)
    {
        return error("no expression after #if/#elif directive!");
    }

    // Evaluate the expression. Treat the result as a boolean.
    if (!evaluator.evaluate(nullptr, &expr_result,
                            expr_evaluator::eval_flags::detect_type |
                            expr_evaluator::eval_flags::undefined_consts_are_zero))
    {
        return false;
    }

    if (expr_result.type == expr_evaluator::eval_type_int)
    {
        *out_result = static_cast<bool>(expr_result.as_int);
    }
    else
    {
        *out_result = static_cast<bool>(expr_result.as_double);
    }
    return true;
}

bool preprocessor::resolve_if_directive()
{
    bool result;
    if (!evaluate_preproc_conditional(&result))
    {
        return false;
    }

    push_conditional(conditional_type::cond_if, result == false);
    return true;
}

bool preprocessor::resolve_ifdef_directive()
{
    lexer::token tok;
    if (!next_token_on_line(&tok) || !tok.is_identifier())
    {
        return error("expected name/identifier immediately after #ifdef directive!");
    }

    push_conditional(conditional_type::cond_ifdef, !is_defined(tok.as_string()));
    return true;
}

bool preprocessor::resolve_ifndef_directive()
{
    lexer::token tok;
    if (!next_token_on_line(&tok) || !tok.is_identifier())
    {
        return error("expected name/identifier immediately after #ifndef directive!");
    }

    push_conditional(conditional_type::cond_ifndef, is_defined(tok.as_string()));
    return true;
}

bool preprocessor::resolve_elif_directive()
{
    conditional_type prev_cond_type;
    bool skipped_prev;
    bool parent_state;
    bool result;

    if (!pop_conditional(&prev_cond_type, &skipped_prev, &parent_state) ||
        prev_cond_type == conditional_type::cond_else)
    {
        return error("misplaced #elif directive!");
    }

    if (!evaluate_preproc_conditional(&result))
    {
        return false;
    }

    // This is necessary for the optional final #else clause.
    const bool old_parent_state = parent_state;
    if (!(result == false || !skipped_prev))
    {
        parent_state = false;
    }

    const bool skip_elif = result == false || !skipped_prev || !old_parent_state;
    push_conditional(conditional_type::cond_elif, skip_elif, &parent_state);
    return true;
}

bool preprocessor::resolve_else_directive()
{
    conditional_type prev_cond_type;
    bool skipped_prev;
    bool parent_state;

    if (!pop_conditional(&prev_cond_type, &skipped_prev, &parent_state))
    {
        return error("misplaced #else directive!");
    }
    if (prev_cond_type == conditional_type::cond_else)
    {
        return error("#else directive followed by #else");
    }

    const bool skip_else = !skipped_prev || !parent_state; // Skip only if no previous conditional was entered.
    push_conditional(conditional_type::cond_else, skip_else);
    return true;
}

bool preprocessor::resolve_endif_directive()
{
    if (!pop_conditional())
    {
        return error("misplaced #endif directive!");
    }
    return true;
}

bool preprocessor::resolve_define_directive()
{
    if (m_current_script == nullptr)
    {
        return error("no script loaded!");
    }

    lexer::token tok;
    if (!next_token(&tok) || tok.get_lines_crossed() != 0)
    {
        if (tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
        }
        return error("empty #define directive!");
    }

    if (!tok.is_identifier())
    {
        return error("#define directive must be followed by a name/identifier!");
    }

    macro_def new_macro{};
    std::string macro_name;
    tok.move_to(&macro_name);

    //
    // Collect optional parameters on the same line:
    //
    // Note that the opening parenthesis must not be preceded
    // by any whitespace. If it is, we must assume it is part
    // of the macro body instead.
    //
    if (next_token(&tok) && m_current_script->get_last_whitespace_length() == 0 &&
        lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
    {
        bool expect_comma = false;
        bool has_va_args  = false;
        new_macro.first_param_token = static_cast<std::uint32_t>(m_macro_tokens.size());

        while (next_token(&tok) && tok.get_lines_crossed() == 0)
        {
            if (lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
            {
                break;
            }
            if (lexer::is_punctuation_token(tok, lexer::punctuation_id::ellipsis))
            {
                has_va_args  = true;
                expect_comma = true;
                new_macro.va_args_macro = true;
                continue;
            }
            if (expect_comma && lexer::is_punctuation_token(tok, lexer::punctuation_id::comma))
            {
                expect_comma = false;
                continue;
            }

            if (tok.is_identifier())
            {
                // A '...' must be at the end of the param list.
                if (has_va_args)
                {
                    return error("\'" + lexer::get_punctuation_from_id(lexer::punctuation_id::ellipsis) +
                                 "\' can only appear as the last parameter in a macro declaration!");
                }

                m_macro_tokens.emplace_back(std::move(tok));
                new_macro.param_token_count++;
                expect_comma = true;
            }
            else
            {
                return error("unexpected token \'" + tok.as_string() + "\' in macro argument list!");
            }
        }

        if (!lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
        {
            return error("missing closing \'" +
                         lexer::get_punctuation_from_id(lexer::punctuation_id::close_parentheses) +
                         "\' in function-like macro definition!");
        }

        if (new_macro.param_token_count == 0)
        {
            // Didn't have any tokens, flag the #define as an empty function-like macro:
            new_macro.first_param_token = 0;
            if (!new_macro.va_args_macro)
            {
                new_macro.empty_func_like_macro = true;
            }
        }
        else
        {
            if (!expect_comma)
            {
                return error("trailing comma in macro argument list!");
            }
            new_macro.empty_func_like_macro = false;
        }
    }
    else
    {
        // Had no parameter list.
        unget_token(tok);
    }

    //
    // Collect optional body tokens:
    //
    // Reads tokens until the end of a line is encountered.
    // If the last token was a '\', continue scanning the next line.
    //
    bool got_backlash = false;
    new_macro.first_body_token = static_cast<std::uint32_t>(m_macro_tokens.size());

    while (next_token(&tok))
    {
        if (lexer::is_punctuation_token(tok, lexer::punctuation_id::backslash))
        {
            got_backlash = true;
            continue;
        }

        if (!got_backlash && tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
            break;
        }

        m_macro_tokens.emplace_back(std::move(tok));
        new_macro.body_token_count++;
        got_backlash = false;
    }

    if (new_macro.body_token_count == 0)
    {
        // Didn't have any tokens. Reset:
        new_macro.first_body_token = 0;
    }

    macro_define(macro_name, &new_macro);
    return true;
}

bool preprocessor::resolve_undef_directive()
{
    lexer::token tok;
    if (!next_token(&tok) || tok.get_lines_crossed() != 0)
    {
        if (tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
        }
        return error("empty #undef directive!");
    }

    if (!tok.is_identifier())
    {
        return error("#undef directive must be followed by a name/identifier!");
    }

    macro_undefine(tok.as_string());
    return true;
}

bool preprocessor::resolve_line_directive()
{
    if (m_current_script == nullptr)
    {
        return error("no script loaded!");
    }

    lexer::token tok;
    if (!next_token(&tok) || tok.get_lines_crossed() != 0)
    {
        if (tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
        }
        return error("empty #line directive!");
    }

    if (!tok.is_number())
    {
        return error("#line directive must be followed by a non-negative line number!");
    }

    m_current_script->set_line_number(tok.as_uint64());
    return true;
}

bool preprocessor::resolve_error_directive()
{
    std::string error_arg;
    read_line(&error_arg);
    return error(error_arg); // Will return false.
}

bool preprocessor::resolve_warning_directive()
{
    std::string warn_arg;
    read_line(&warn_arg);
    warning(warn_arg);
    return true;
}

bool preprocessor::resolve_pragma_directive()
{
    if (m_current_script == nullptr)
    {
        return error("no script loaded!");
    }

    lexer::token tok;
    if (!next_token(&tok) || tok.get_lines_crossed() != 0)
    {
        if (tok.get_lines_crossed() != 0)
        {
            unget_token(tok);
        }
        warning("empty #pragma directive.");
        return true;
    }

    bool open_par;
    if (lexer::is_punctuation_token(tok, lexer::punctuation_id::open_parentheses))
    {
        if (!next_token(&tok)) // Skip the '('
        {
            return error("nothing after opening parentheses in #pragma directive!");
        }
        open_par = true;
    }
    else
    {
        open_par = false;
    }

    // Parenthesis are optional in a #pragma directive.
    if (!tok.is_identifier())
    {
        // Empty #pragma() with parenthesis? Not an error.
        if (lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
        {
            warning("empty #pragma directive.");
            return true;
        }
        return error("expected identifier/name after #pragma directive, got \'" + tok.as_string() + "\'.");
    }

    if (tok == "once") // #pragma once
    {
        // If the filename shows more than once in the dynamic scripts list, we don't need to include it again.
        int include_count = 0;
        for (const lexer * script : m_dynamic_scripts)
        {
            if (script->get_filename() == m_current_script->get_filename())
            {
                ++include_count;
            }
        }

        if (include_count > 1)
        {
            PREPROCESSOR_ASSERT(!m_include_stack.empty());
            m_current_script = m_include_stack.back();
            m_include_stack.pop_back();
            return true;
        }
    }
    else if (tok == "warning") // #pragma (warning: [enabled|disable])
    {
        if (!next_token(&tok) ||
            !lexer::is_punctuation_token(tok, lexer::punctuation_id::colon))
        {
            return error("\'#pragma warning\' must be followed by a colon!");
        }

        // Skip the ':'
        if (!next_token(&tok))
        {
           return error("incomplete #pragma warning command!");
        }

        if (tok == "enable")
        {
            enable_warnings();
        }
        else if (tok == "disable")
        {
            disable_warnings();
        }
        else
        {
            return error("unknown #pragma warning command: \'" + tok.as_string() + "\'.");
        }
    }
    else
    {
        warning("ignoring unknown #pragma directive: \'" + tok.as_string() + "\'.");
        skip_rest_of_line();
        return true; // Not an error.
    }

    if (open_par) // Expect the closing parentheses:
    {
        if (!next_token(&tok) ||
            !lexer::is_punctuation_token(tok, lexer::punctuation_id::close_parentheses))
        {
            return error("#pragma directive missing closing parentheses!");
        }
    }

    return true;
}

bool preprocessor::resolve_include_directive()
{
    lexer::token tok;
    std::string inc_filename;
    bool default_search_path;

    if (!allow_includes())
    {
        return error("file inclusion via the #include directive is disabled!");
    }

    //
    // Get the included filename:
    //
    if (!next_token_on_line(&tok))
    {
        return error("expected a filename after #include directive!");
    }

    // Not #include "file" nor #include <file>?
    if (!tok.is_string() && !lexer::is_punctuation_token(tok, lexer::punctuation_id::logic_less))
    {
        return error("expected string enclosed in double-quotes or \'< >\' after #include directive!");
    }

    if (tok.is_string()) // #include "file"
    {
        if (tok.as_string().empty())
        {
            return error("empty string after #include directive!");
        }

        tok.move_to(&inc_filename);
        default_search_path = false;
    }
    else // #include <file>
    {
        // Filename might contain path separators and dots, so it is simpler
        // to just read the rest of the line as a string and remove the closing '>'.
        read_line(&inc_filename);

        if (inc_filename.empty() || inc_filename.back() != '>')
        {
            return error("missing closing \'>\' in #include directive!");
        }

        inc_filename.pop_back();
        if (inc_filename.empty())
        {
            return error("empty string after #include directive!");
        }

        default_search_path = true;
    }

    //
    // Try to open the new script:
    //
    if (default_search_path && !m_search_paths.empty())
    {
        if (!allow_base_includes())
        {
            return error("base includes (#include <>) are not allowed!");
        }

        bool got_one = false;
        std::string full_script_path;

        for (const auto & path : m_search_paths)
        {
            full_script_path = path + inc_filename;
            if (try_open_include_file(full_script_path))
            {
                got_one = true;
                break;
            }
        }

        if (!got_one)
        {
            return error("unable to open included file \"" + inc_filename + "\" using default search paths.");
        }
    }
    else // Search locally:
    {
        if (!try_open_include_file(inc_filename))
        {
            return error("unable to open included file \"" + inc_filename + "\".");
        }
    }

    return true;
}

bool preprocessor::try_open_include_file(const std::string & filename)
{
    lexer included_script;
    std::uint32_t lex_flags = ((m_current_script != nullptr) ? m_current_script->get_flags() : 0);
    lex_flags |= lexer::flags::no_string_concat;

    if (m_flags != 0)
    {
        // Error/warning flags override the lexer settings.
        if (m_flags & flags::no_errors)
        {
            lex_flags |= lexer::flags::no_errors;
        }
        if (m_flags & flags::no_warnings)
        {
            lex_flags |= lexer::flags::no_warnings;
        }
        if (m_flags & flags::no_fatal_errors)
        {
            lex_flags |= lexer::flags::no_fatal_errors;
        }
    }

    if (!included_script.init_from_file(filename, lex_flags, /* silent = */ true))
    {
        return false;
    }

    m_include_stack.push_back(m_current_script);
    m_dynamic_scripts.push_back(new lexer{ std::move(included_script) });
    m_current_script = m_dynamic_scripts.back();
    return true;
}

bool preprocessor::eval(const std::string & expression, std::int64_t * out_i_result, double * out_f_result,
                        const bool math_consts, const bool math_funcs, const bool undefined_consts_are_zero)
{
    if (expression.empty())
    {
        if (out_i_result != nullptr) { *out_i_result = 0; }
        if (out_f_result != nullptr) { *out_f_result = 0; }
        return false;
    }

    std::uint32_t lex_flags = lexer::flags::no_string_concat;
    if (m_flags != 0)
    {
        if (m_flags & flags::no_errors)
        {
            lex_flags |= lexer::flags::no_errors;
        }
        if (m_flags & flags::no_warnings)
        {
            lex_flags |= lexer::flags::no_warnings;
        }
        if (m_flags & flags::no_fatal_errors)
        {
            lex_flags |= lexer::flags::no_fatal_errors;
        }
    }

    lexer lex;
    if (!lex.init_from_memory(expression.c_str(), expression.length(), "(eval-string)", lex_flags))
    {
        if (out_i_result != nullptr) { *out_i_result = 0; }
        if (out_f_result != nullptr) { *out_f_result = 0; }
        return false;
    }

    expr_evaluator evaluator{ this };
    expr_evaluator::eval_value expr_result;
    std::uint32_t eval_flags = expr_evaluator::eval_flags::detect_type;

    if (math_consts)
    {
        eval_flags |= expr_evaluator::eval_flags::allow_math_consts;
    }
    if (math_funcs)
    {
        eval_flags |= expr_evaluator::eval_flags::allow_math_funcs;
    }
    if (undefined_consts_are_zero)
    {
        eval_flags |= expr_evaluator::eval_flags::undefined_consts_are_zero;
    }

    lexer::token tok;
    while (lex.next_token(&tok))
    {
        evaluator.push_token(std::move(tok));
    }

    if (!evaluator.evaluate(nullptr, &expr_result, eval_flags))
    {
        if (out_i_result != nullptr) { *out_i_result = 0; }
        if (out_f_result != nullptr) { *out_f_result = 0; }
        return false;
    }

    if (expr_result.type == expr_evaluator::eval_type_int)
    {
        if (out_i_result != nullptr)
        {
            *out_i_result = expr_result.as_int;
        }
        if (out_f_result != nullptr)
        {
            *out_f_result = static_cast<double>(expr_result.as_int);
        }
    }
    else
    {
        PREPROCESSOR_ASSERT(expr_result.type == expr_evaluator::eval_type_double);

        if (out_i_result != nullptr)
        {
            *out_i_result = static_cast<std::int64_t>(expr_result.as_double);
        }
        if (out_f_result != nullptr)
        {
            *out_f_result = expr_result.as_double;
        }
    }

    return true;
}

#undef PREPROC_FLOAT_FMT
#undef PREPROC_INT64_FMT
#undef PREPROC_NUMBUF_SIZE

// ================ End of implementation =================
#endif // PREPROCESSOR_IMPLEMENTATION
// ================ End of implementation =================
