
// ------------------------------------
// $evalfloat / $evalint:
// ------------------------------------

// result = 11.0
$evalfloat((1 + 2) + 3 + 4 + 1);
$evalfloat(1 + 2 + 3 + 4 + 1);

// result = 10.0
$evalfloat(1 + 2 + (3 + 4));
$evalfloat(1 + 2 + 3 + 4);

// result = 10.0
$evalfloat(1 + (2 + 3) + 4);
$evalfloat(1 + 2 + 3 + 4);

// result = 10
$evalint((1 + 2) + (3 + 4));
$evalint(1 + 2 + 3 + 4);

// result = 15
$evalint(1 + (1 + 2) + (3 + 4) + 2 + (1 + 1));
$evalint(1 + 1 + 2 + 3 + 4 + 2 + 1 + 1);

// result = 14
$evalint(1 + (2 + (2 + 2) + 3) + 4);
$evalint(1 + 2 + 2 + 2 + 3 + 4);

// result = 18
$evalint(1 + (2 + (2 + 2) + (2 + 2) + 3) + 4);
$evalint(1 + 2 + 2 + 2 + 2 + 2 + 3 + 4);

// result = 16
$evalint(1 + (1 + (1 + (1 + 1))) + 1 + 2 + (4 + 4));

// result = 8
$evalint((1 + (1 + (1 + (1 + 1))) + 1 + 2 + (4 + 4)) * 0.5);

// ------------------------------------
// $eval:
// ------------------------------------

$eval(1 + 1)                    //  2
$eval(1.5 * 2)                  //  3.0
$eval(3.0 / 2.0)                //  1.5
$eval(-0);                      //  0
$eval(+0);                      //  0
$eval(-1);                      // -1
$eval(+1);                      // +1
$eval(-1 + (-2 + 3));           //  0
$eval(+666);                    //  666
$eval(0 + !1);                  //  0
$eval(0 + ~1);                  // -2
$eval(0 + !!1);                 //  1
$eval(0 + ~!1);                 // -1
$eval(0 + - -1);                //  1
$eval(0 + -(-1));               //  1
$eval(0 + - + 1);               // -1
$eval(~ -1);                    //  0
$eval(! -1);                    //  0
$eval(~(-1));                   //  0
$eval(!(-1));                   //  0
$eval(- ~1);                    //  2
$eval(- !1);                    //  0
$eval(-(~1));                   //  2
$eval(-(!1));                   //  0
$eval(2 + - ~1);                //  4
$eval(2 + - !1);                //  2
$eval(! -0);                    //  1
$eval(- !0);                    // -1
$eval(!(-0));                   //  1
$eval(-(!0));                   // -1
$eval(-42);                     // -42
$eval(-(2 + 1));                // -3
$eval(! -0.0);                  //  1.0
$eval(- !0.0);                  // -1.0
$eval(! -0.5);                  //  0.0
$eval(- !0.5);                  // -0.0
$eval(1 + 2 - 3 + 4);           //  4
$eval(1 + 2 * 3 + 4);           //  11
$eval((1 ? 42 : 666) + 1);      //  43
$eval((0 ? 42 : 666) + 1);      //  667
$eval(-1 - -2 + 4);             //  5
$eval(-1 - (-2 + 4));           // -3
$eval(((1 < 2) ? -1.0 : -2.0)); // -1.0
$eval(4 != 5);                  //  1
$eval(1 - 2 * 2);               // -3
$eval(1 + -(2 * 2));            // -3
$eval(-1 - -(-2 + 4));          //  1
$eval(1 + -(1 + 1));            // -1

// Predefined math constants visible inside $eval():
$eval(PI);
$eval(E);
$eval(TAU);
$eval(INV_TAU);
$eval(HALF_PI);
$eval(INV_PI);
$eval(DEG2RAD);
$eval(RAD2DEG);

// Some of the built-in math functions:
$eval(2 * cos(3 + 2));
$eval(round(-1.4));
$eval(floor(PI));
$eval(ceil(PI));
$eval(abs(+2));
$eval(abs(-2));
$eval(1 + acos(cos(1)) - 1);
$eval(log2(exp2(1.0)))

// ------------------------------------
// #warn/#warning command:
// ------------------------------------

#warn // no warning message

#warn "Single line warning message."

#warning "A long, " \
         "multi-line warning " \
         "message with several words."

"Some unrelated string";

#pragma(warning: disable)
#warning  "This warning will NOT print."

#pragma(warning: enable)
#warning "This warning will print."

// ------------------------------------
// #define macros:
// ------------------------------------

#define HELLO 111 222 333 "hello!" \
              555 "next up"        \
              "Howdy " 3.14

#define WORLD 222
#define WORLD 666 /* redefinition warning */

#define FANCY_MACRO                   \
    do                                \
    {                                 \
        for (int i = 0; i < 42; ++i)  \
        {                             \
            std::printf("i = %i", i); \
        }                             \
    } while (0)

#define EMPTY_MACRO
#define GET_DATE __DATE__
#define GET_LINE __LINE__

#define META_FUNC(x, y, z) { (z + y + x) - [x * x * x] + { y, y } / | z ^ z | }

// Expand them:
HELLO;
WORLD;
FANCY_MACRO;
EMPTY_MACRO;
GET_DATE;
GET_LINE;
META_FUNC(1, 2, 3);

#define MY_ASSERT(expr) if (!(expr)) { print_error(#expr); }

MY_ASSERT(1 < 2);
MY_ASSERT(true != false);

#define TEST_STRINGIZE(bar) "foo-" #bar "-baz"
#define TEST_CONCAT_1(bar) foo_ ## bar
#define TEST_CONCAT_2(bar) foo_ ## bar ## _baz

TEST_STRINGIZE(hello);
TEST_STRINGIZE(" hello \"john\" ");

TEST_CONCAT_1(world);
TEST_CONCAT_2(world);

#define FUNC_LIKE_MACRO(a, b, c)          \
    do                                    \
    {                                     \
        std::printf("%d\n", a);           \
        std::printf("%f\n", b##__LINE__); \
        std::printf("%s\n", #c);          \
        std::printf("%s\n", __FILE__);    \
        META_FUNC(1, 2, c);               \
        std::printf("FINISHED!\n");       \
    } while (0)

FUNC_LIKE_MACRO(1, A_, 33);

#define STRING_JOIN3_HELPER(x, y, z) x ## y ## z
#define STRING_JOIN3(x, y, z) STRING_JOIN3_HELPER(x, y, z)

STRING_JOIN3_HELPER(Foo_, Bar_, Baz1);
STRING_JOIN3(Foo_, Bar_, Baz2);

#define VARIADIC_MACRO_0(...)        \
    do                               \
    {                                \
        std::printf(__VA_ARGS__);    \
    } while (0)

#define VARIADIC_MACRO_1(x, ...)     \
    do                               \
    {                                \
        std::printf("x = %s\n", #x); \
        std::printf(__VA_ARGS__);    \
    } while (0)

#define VARIADIC_MACRO_2(x, y, ...)  \
    do                               \
    {                                \
        std::printf("x = %s\n", #x); \
        std::printf("y = %s\n", #y); \
        std::printf(__VA_ARGS__);    \
    } while (0)

VARIADIC_MACRO_0(1, 2, 3, "foo", "bar", some_var0);
VARIADIC_MACRO_1(AAA, 1, 2, 3, "foo", "bar", some_var1);
VARIADIC_MACRO_2(AAA, BBB, 1, 2, 3, "foo", "bar", some_var2);

#define PR3(a, b, c) std::printf("%s, %s, %s\n", a, b, c)
#define PRINT3(...) PR3(__VA_ARGS__)
PRINT3("one", "two", "three");

// ------------------------------------
// Preprocessor conditional directives:
// ------------------------------------

//#define FOO
//#define BAR

#ifdef FOO
    << FOO is defined >>
    #ifndef BAR
        << Top BAR is NOT defined >>
    #endif // BAR
#else // FOO
    << FOO not defined >>
    #ifndef BAR
        << Botton BAR is NOT defined >>
    #endif // BAR
#endif // FOO

#if ((((0 || 1) * 0) == 0) && 1 < 2)
    << 1 >> // This one should remain
#elif 1
    << 2 >> // This one is stripped
#endif

//#define UNDEFINED_M 1
//#define FOO 1
//#define BAR 1

#if defined(UNDEFINED_M) // same as 0 if not defined
    [[IF]]
#elif FOO
    [[ELIF_1]]
#elif BAR
    [[ELIF_2]]
#else
    [[ELSE]]
#endif

