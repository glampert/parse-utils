
## Parse Utils

C++11 utilities for lexing and parsing of script languages, configuration files and command-lines.

`lexer.hpp` is a lightweight lexicographical scanner capable of scanning C-like languages.
Code was adapted from the lexer used by the id Tech 4 engine and the DOOM 3 game [(available here)](https://github.com/id-Software/DOOM-3-BFG/blob/master/neo/idlib/Lexer.h).

The C++ classes are header only and self contained. You have to include the `.hpp` in one source file
and define `XYZ_IMPLEMENTATION` to generate the implementation code in that source file. After that,
the header can be used as a normal C++ header. This is in the same spirit of the [stb](https://github.com/nothings/stb) libraries.

Check `tests/` directory for usage examples and other tests.

### License

Code in this repository is released under the terms of the GNU GPL version 3,
to comply with the license used by code this work was based on.

