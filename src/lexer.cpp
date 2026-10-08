#include "lexer.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>

std::string IdentifierStr; // static because we want to keep it here
double NumVal;
int LineNum{1};
int ColNum{1};

// gettok()
//  skips any whitespace
//  if not whitespace, add to IdentifierStr, and check if we have def or
//  extern since those are the only two 'fixed' keywords for numbers we
//  check if it has decimal or if the lastchar is a digit, we add to numstr
//  until we cannot anymore, convert and return for comments: we keep moving
//  until we hit eof or new line or \r we check for eof and return or we
//  have an operator

int gettok() {
    static int LastChar = ' ';

    while (std::isspace(LastChar)) {
        if (LastChar == '\n') {
            LineNum++;
            ColNum = 1;
        } else {
            ColNum++;
        }
        LastChar = std::getchar();
    }

    if (LastChar != EOF &&
        (std::isalpha(static_cast<unsigned char>(LastChar)) ||
         LastChar == '_')) {

        IdentifierStr = static_cast<char>(LastChar);
        ColNum++;

        LastChar = std::getchar();

        while (LastChar != EOF &&
               (std::isalnum(static_cast<unsigned char>(LastChar)) ||
                LastChar == '_')) {
            IdentifierStr += static_cast<char>(LastChar);
            ColNum++;
            LastChar = std::getchar();
        }

        if (IdentifierStr == "def")
            return tok_def;

        if (IdentifierStr == "extern")
            return tok_extern;

        if (IdentifierStr == "_") {
            std::cout << "an identifier cannot just be _, terminating...\n";
            return tok_error;
        }

        if (IdentifierStr == "if")
            return tok_if;

        if (IdentifierStr == "then")
            return tok_then;

        if (IdentifierStr == "else")
            return tok_else;

        if (IdentifierStr == "for")
            return tok_for;

        if (IdentifierStr == "in")
            return tok_in;

        return tok_identifier;
    }

    if (std::isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;

        bool SeenDecimal = false;

        while (std::isdigit(LastChar) || LastChar == '.') {
            if (LastChar == '.') {
                if (SeenDecimal) {
                    std::cout << "There are more than one decimal points in "
                                 "the number, terminating...\n";
                    return tok_error;
                }
                SeenDecimal = true;
            }

            NumStr += LastChar;
            ColNum++;
            LastChar = std::getchar();

            std::cout << "ColNum is " << ColNum << "\n";
            std::cout << "LineNum is " << LineNum << "\n";
        }
        NumVal = std::strtod(NumStr.c_str(), nullptr);
        // std::cout << "NumVal is " << NumVal << "\n";
        return tok_number;
    }

    if (LastChar == '#') {
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r') {
            ColNum++;
            LastChar = std::getchar();
        }

        // if not end of file then we restart from the beginning
        if (LastChar != EOF) {
            return gettok();
        }
    }

    if (LastChar == EOF) {
        return tok_eof;
    }

    int ThisChar = LastChar;
    ColNum++;
    LastChar = std::getchar();
    return ThisChar;
}
