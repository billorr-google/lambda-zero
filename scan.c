#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "scan.h"

bool isComment(const char* start) {
    return start[0] == '-' && start[1] == '-';
}

bool isEscapedNewline(const char* start) {
    return start[0] == '\\' && start[1] == '\n';
}

static const char* skipTo(const char* str, const char* characters) {
    return &(str[strcspn(str, characters)]);
}

static const char* skipPast(const char* str, const char* characters) {
    return &(str[strspn(str, characters)]);
}

static const char* skipPastSameCharacter(const char* str) {
    return skipPast(str, (char[2]){str[0], '\0'});
}

static const char* skipToDelimiter(const char* str) {
    return skipTo(str, " \n();,.");
}

static const char* skipToNewline(const char* str) {
    return skipTo(str, "\n");
}

static const char* skipOneCharacter(const char* str) {
    return str[0] == '\0' ? str : str + 1;
}

static const char* skipElided(const char* str) {
    while (isComment(str) || isEscapedNewline(str))
        str = isComment(str) ? skipToNewline(str) :
            skipOneCharacter(skipToNewline(str));
    return str;
}

static const char* skipQuote(const char* start) {
    char quote = start[0];
    // assumption is that start points to the opening quotation mark
    for (start += 1; start[0] != '\0' && start[0] != '\n'; start += 1) {
        if (start[0] == '\\' && start[1] != '\0')
            start += 1;     // skip character following slash
        else if (start[0] == quote)
            return start + 1;
    }
    return start;
}

const char* skipLexeme(const char* lexeme) {
    assert(lexeme[0] != '\0');
    if (lexeme[0] == '"' || lexeme[0] == '\'')
        return skipToDelimiter(skipQuote(lexeme));
    if (lexeme[0] == ' ' || lexeme[0] == '.')
        return skipPastSameCharacter(lexeme);
    const char* delimiter = skipToDelimiter(lexeme);
    return delimiter == lexeme ? delimiter + 1 : delimiter;
}

const char* getFirstLexeme(const char* input) {
    return skipElided(input);
}

const char* getNextLexeme(const char* lastLexeme) {
    return skipElided(skipLexeme(lastLexeme));
}

size_t getLexemeLength(const char* lexeme) {
    return (size_t)(lexeme[0] == '\0' ? 1 : skipLexeme(lexeme) - lexeme);
}

bool isSameLexeme(const char* a, const char* b) {
    size_t lengthA = getLexemeLength(a);
    size_t lengthB = getLexemeLength(b);
    return lengthA == lengthB && strncmp(a, b, lengthA) == 0;
}
