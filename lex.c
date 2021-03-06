#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "lib/tree.h"
#include "scan.h"
#include "ast.h"
#include "errors.h"
#include "lex.h"

bool isNameLexeme(const char* lexeme) {
    // note: the quote case is only for internal code
    return islower(lexeme[0]) || lexeme[0] == '_' ||  lexeme[0] == '\'';
}

bool isOperatorLexeme(const char* lexeme) {
    return isDelimiterCharacter(lexeme[0]) || isOperatorCharacter(lexeme[0]) ||
        isSpaceCharacter(lexeme[0]);
}

bool isIntegerLexeme(const char* lexeme) {
    for (unsigned int i = 0; i < getLexemeLength(lexeme); i++)
        if (!isdigit(lexeme[i]))
            return false;
    return true;
}

const char* skipQuoteCharacter(const char* start) {
    return start[0] == '\\' ? start + 2 : start + 1;
}

char decodeCharacter(const char* start, const char* lexeme) {
    lexerErrorIf(start[0] <= 0, lexeme, "illegal character in");
    if (start[0] != '\\')
        return start[0];
    switch (start[1]) {
        case '0': return '\0';
        case 't': return '\t';
        case 'r': return '\r';
        case 'n': return '\n';
        case '\n': return '\n';
        case '\\': return '\\';
        case '\"': return '\"';
        case '\'': return '\'';
        default: lexerErrorIf(true, lexeme, "invalid escape sequence in");
    }
    return 0;
}

Node* newCharacterLiteral(const char* lexeme) {
    char quote = lexeme[0];
    const char* end = lexeme + getLexemeLength(lexeme) - 1;
    lexerErrorIf(end[0] != quote, lexeme, "missing end quote for");
    const char* skip = skipQuoteCharacter(lexeme + 1);
    lexerErrorIf(skip != end, lexeme, "invalid character literal");
    unsigned char code = (unsigned char)decodeCharacter(lexeme + 1, lexeme);
    return newInteger(getLexemeLocation(lexeme), code);
}

Node* buildStringLiteral(const char* lexeme, const char* start) {
    char c = start[0];
    int location = getLexemeLocation(lexeme);
    lexerErrorIf(c == '\n' || c == 0, lexeme, "missing end quote for");
    if (c == lexeme[0])
        return newNil(location);
    return prepend(location, newInteger(location, decodeCharacter(start,
        lexeme)), buildStringLiteral(lexeme, skipQuoteCharacter(start)));
}

Node* newStringLiteral(const char* lexeme) {
    return buildStringLiteral(lexeme, lexeme + 1);
}

long long parseInteger(const char* lexeme) {
    errno = 0;
    long long result = strtoll(lexeme, NULL, 10);
    lexerErrorIf((result == LLONG_MIN || result == LLONG_MAX) &&
        errno == ERANGE, lexeme, "magnitude of integer is too large");
    return result;
}

Node* createToken(const char* lexeme) {
    lexerErrorIf(isupper(lexeme[0]), lexeme, "names can't start with uppercase");
    if (lexeme[0] == '"')
        return newStringLiteral(lexeme);
    // single quoted operands are internal names while parsing internal code
    if (lexeme[0] == '\'')
        return newCharacterLiteral(lexeme);

    int location = getLexemeLocation(lexeme);
    if (isIntegerLexeme(lexeme))
        return newInteger(location, parseInteger(lexeme));
    if (isNameLexeme(lexeme))
        return newName(location);
    if (isOperatorLexeme(lexeme))
        return newOperator(location);

    lexerErrorIf(true, lexeme, "invalid token");
    return NULL;
}

Hold* getFirstToken(const char* sourceCode) {
    return hold(createToken(getFirstLexeme(sourceCode)));
}

Hold* getNextToken(Hold* token) {
    return hold(createToken(getNextLexeme(getLexeme(getNode(token)))));
}
