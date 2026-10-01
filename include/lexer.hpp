#pragma once

#include <string>

// 1.2
// for lexer, we need to allow our language to have:
// eof (edge case if we are at end of file)
// def, extern, identifier(variable assigned by the programmer),
// the lexer treats 35.1, the same as 47.5, and 31 so we need a number too
// all the '+' '-', etc is taken over by the cpp language itself
// we use enum because we want to use
// IdentifierStr: there can be two identifier's but we need to know which one
// NumVal: same as above
// we don't need to use for def because def can only have one definition (extra
// information)

enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
    tok_if = -6,
    tok_then = -7,
    tok_else = -8,
    tok_for = -9,
    tok_in = -10,
    tok_error = -11
};

int gettok();

extern std::string IdentifierStr;
extern double NumVal;
extern int LineNum;
extern int ColNum;
