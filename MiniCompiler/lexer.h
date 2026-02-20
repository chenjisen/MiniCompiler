#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mini_compiler {

using std::format;
using std::runtime_error;
using std::string;
using std::string_view;
using std::vector;

// ==========================================
// 1. Token Definitions
// ==========================================

#define TOKEN_LIST(X)                                                          \
    X(SlashEq, "/=", CompoundSymbol)                                           \
    X(Slash, "/", Symbol)                                                      \
    X(LessLess, "<<", CompoundSymbol)                                          \
    X(Spaceship, "<=>", CompoundSymbol)                                        \
    X(LessEq, "<=", CompoundSymbol)                                            \
    X(Less, "<", Symbol)                                                       \
    X(GreaterGreater, ">>", CompoundSymbol)                                    \
    X(GreaterEq, ">=", CompoundSymbol)                                         \
    X(Greater, ">", Symbol)                                                    \
    X(PlusPlus, "++", CompoundSymbol)                                          \
    X(PlusEq, "+=", CompoundSymbol)                                            \
    X(Plus, "+", Symbol)                                                       \
    X(MinusMinus, "--", CompoundSymbol)                                        \
    X(MinusEq, "-=", CompoundSymbol)                                           \
    X(Arrow, "->", CompoundSymbol)                                             \
    X(Minus, "-", Symbol)                                                      \
    X(LogicalOr, "||", CompoundSymbol)                                         \
    X(Pipe, "|", Symbol)                                                       \
    X(LogicalAnd, "&&", CompoundSymbol)                                        \
    X(MultiplyEq, "*=", CompoundSymbol)                                        \
    X(Multiply, "*", Symbol)                                                   \
    X(ModuloEq, "%=", CompoundSymbol)                                          \
    X(Modulo, "%", Symbol)                                                     \
    X(Ampersand, "&", Symbol)                                                  \
    X(Caret, "^", Symbol)                                                      \
    X(Tilde, "~", Symbol)                                                      \
    X(EqualComparison, "==", CompoundSymbol)                                   \
    X(Assignment, "=", Symbol)                                                 \
    X(NotEqualComparison, "!=", CompoundSymbol)                                \
    X(Not, "!", Symbol)                                                        \
    X(LeftBrace, "{", Symbol)                                                  \
    X(RightBrace, "}", Symbol)                                                 \
    X(LeftParen, "(", Symbol)                                                  \
    X(RightParen, ")", Symbol)                                                 \
    X(LeftBracket, "[", Symbol)                                                \
    X(RightBracket, "]", Symbol)                                               \
    X(Scope, "::", CompoundSymbol)                                             \
    X(Colon, ":", Symbol)                                                      \
    X(Semicolon, ";", Symbol)                                                  \
    X(Comma, ",", Symbol)                                                      \
    X(Dot, ".", Symbol)                                                        \
    X(DotDot, "..", CompoundSymbol)                                            \
    X(Ellipsis, "...", CompoundSymbol)                                         \
    X(EllipsisLess, "..<", CompoundSymbol)                                     \
    X(EllipsisEqual, "..=", CompoundSymbol)                                    \
    X(QuestionMark, "?", Symbol)                                               \
    X(At, "@", Symbol)                                                         \
    X(Dollar, "$", Symbol)                                                     \
    X(FloatLiteral, "Float Literal", Literal)                                  \
    X(IntLiteral, "Int Literal", Literal)                                      \
    X(StringLiteral, "String Literal", Literal)                                \
    X(CharLiteral, "Char Literal", Literal)                                    \
    X(BoolLiteral, "Bool Literal", Literal)                                    \
    X(KwLet, "let", Keyword)                                                   \
    X(KwFn, "fn", Keyword)                                                     \
    X(KwReturn, "return", Keyword)                                             \
    X(KwBitOr, "bitor", Keyword)                                               \
    X(KwBitAnd, "bitand", Keyword)                                             \
    X(KwXor, "xor", Keyword)                                                   \
    X(KwCompl, "compl", Keyword)                                               \
    X(KwLeftShift, "shl", Keyword)                                             \
    X(KwRightShift, "shr", Keyword)                                            \
    X(Identifier, "Identifier", Identifier)                                    \
    X(Error, "(ERROR)", Unknown)                                               \
    X(End, "(END)", Unknown)

enum class TokenKind : uint8_t {

#define AS_ENUM(name, str, kind_class) name,

    TOKEN_LIST(AS_ENUM)

#undef AS_ENUM

};

constexpr string_view to_string(TokenKind kind) {
    switch (kind) {

#define AS_CASE(name, str, kind_class)                                         \
    case TokenKind::name:                                                      \
        return str;

        TOKEN_LIST(AS_CASE)

#undef AS_CASE
    }
    return "(UNKNOWN)";
}

enum class TokenClass : uint8_t {
    Unknown,
    Symbol,
    CompoundSymbol,
    Literal,
    Keyword,
    Identifier
};

constexpr TokenClass get_token_class(TokenKind kind) {
    switch (kind) {

#define AS_CLASS(name, str, kind_class)                                        \
    case TokenKind::name:                                                      \
        return TokenClass::kind_class;

        TOKEN_LIST(AS_CLASS)

#undef AS_CLASS
    }
    return TokenClass::Unknown;
}

constexpr std::vector<TokenKind> get_keywords() {
    std::vector<TokenKind> keywords;

#define AS_KEYWORD(name, str, kind_class)                                      \
    if constexpr (TokenClass::kind_class == TokenClass::Keyword)               \
        keywords.push_back(TokenKind::name);

    TOKEN_LIST(AS_KEYWORD)

#undef AS_KEYWORD

    return keywords;
}

constexpr bool is_assign(TokenKind k) {
    switch (k) {
    case TokenKind::Assignment:
    case TokenKind::SlashEq:
    case TokenKind::PlusEq:
    case TokenKind::MinusEq:
    case TokenKind::ModuloEq:
    case TokenKind::MultiplyEq:
        return true;
    }
    return false;
}

constexpr bool is_prefix_unary(TokenKind k) {
    switch (k) {
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Not:
        return true;
    default:
        return false;
    }
}

using lineno_t = int32_t;
using colno_t = int32_t;
using index_t = int32_t;

struct SourcePosition {
    lineno_t lineno = 1; // one-based offset into program source
    colno_t colno = 1;   // one-based offset into line
    index_t index = 0;

    SourcePosition() = default;

    SourcePosition(lineno_t l, colno_t c, index_t i)
        : lineno{l}, colno{c}, index(i) {}

    auto operator<=>(SourcePosition const&) const = default;

    auto to_string() const -> string {
        return format("({}, {}) i={}", lineno, colno, index);
    }
};

struct Token {
    TokenKind kind = TokenKind::Error;
    string_view lexeme;
    SourcePosition pos;
};

// ==========================================
// 2. Lexer
// ==========================================
class Lexer {
  public:
    explicit Lexer(string_view src) : source(src) {}

    vector<Token> tokenize() {
        vector<Token> tokens;
        while (!is_at_end()) {
            Token const tok = next_token();
            if (tok.kind != TokenKind::Error) {
                // 忽略错误 token 或保留特殊 token
                tokens.emplace_back(tok);
            }
        }
        tokens.push_back({TokenKind::End, "", pos});
        if (!errors.empty()) {
            string msg = "Lex errors:\n";
            for (auto const& e : errors) {
                msg += e + "\n";
            }
            throw runtime_error(msg);
        }

        return tokens;
    }

  private:
    string_view source;
    SourcePosition pos;

    vector<string> errors;

    Token next_token() {
        skip_whitespace();
        if (is_at_end()) {
            return {.kind = TokenKind::End, .lexeme = "<eof>", .pos = pos};
        }

        char const c = peek();

        // number: digits ( '.' digits )?
        if (is_digit(c)) {
            return lex_number();
        }

        if (is_ident_start(c)) {
            return lex_identifier();
        }

        if (c == '"') {
            return lex_string_literal();
        }

        return lex_symbol();
    }

    bool is_at_end() const { return pos.index >= ssize(source); }

    static bool is_ident_start(char c) {
        return (std::isalpha(c) != 0) || c == '_';
    }

    static bool is_ident_part(char c) {
        return (std::isalnum(c) != 0) || c == '_';
    }

    char peek(int offset = 0) const {
        int const p = pos.index + offset;
        if (p >= ssize(source)) {
            return '\0';
        }
        return source[p];
    }

    char advance() {
        if (is_at_end()) {
            return '\0';
        }
        pos.colno++;
        return source[pos.index++];
    }

    string_view get_substr_from_start(SourcePosition start_pos) {
        if (start_pos.index >= ssize(source) || pos.index > ssize(source) ||
            start_pos.index > pos.index) {
            throw runtime_error(
                "Invalid source position for substring extraction");
        }
        return source.substr(
            start_pos.index, static_cast<size_t>(pos.index - start_pos.index));
    }

    void skip_whitespace() {
        while (!is_at_end()) {
            char const c = peek();
            if (std::isspace(c) != 0) {
                if (c == '\n') {
                    pos.lineno++;
                    pos.colno = 0;
                }
                advance();
            } else if (c == '/') {
                if (peek(1) == '/') {
                    // line comment //
                    advance();
                    advance(); // consume '//'
                    while (!is_at_end() && peek() != '\n') {
                        advance();
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }

    Token lex_identifier() {

        static std::unordered_map<string_view, TokenKind> keyword_map = []() {
            std::unordered_map<string_view, TokenKind> m;
            for (TokenKind const k : get_keywords()) {
                m[to_string(k)] = k;
            }
            return m;
        }();

        SourcePosition const start_pos = pos;
        while (!is_at_end() && is_ident_part(peek())) {
            advance();
        }
        string_view const text = get_substr_from_start(start_pos);
        if (auto it = keyword_map.find(text); it != keyword_map.end()) {
            return {.kind = it->second, .lexeme = "", .pos = start_pos};
        }

        return {
            .kind = TokenKind::Identifier, .lexeme = text, .pos = start_pos};
    }

    static bool is_digit(char c) { return std::isdigit(c) != 0; }

    Token lex_number() {
        SourcePosition const start_pos = pos;

        while (!is_at_end() && is_digit(peek())) {
            advance();
        }

        TokenKind kind = TokenKind::Error;

        // Check for float
        if (peek() == '.') {
            kind = TokenKind::FloatLiteral;
            advance(); // Consume .
            while (is_digit(peek())) {
                advance();
            }
        } else {
            kind = TokenKind::IntLiteral;
        }

        return {
            .kind = kind,
            .lexeme =
                source.substr(start_pos.index, pos.index - start_pos.index),
            .pos = start_pos};
    }

    Token lex_string_literal() {
        advance(); // consume "
        SourcePosition const start_pos = pos;

        while (!is_at_end()) {
            char const c = peek();
            if (c == '\\') {
                advance(); // skip escape
                if (!is_at_end()) {
                    advance(); // skip escaped char
                }
                // char next_c = peek();
                // TODO(cjs): escape
                continue;
            }
            if (c == '\n') {
                SourcePosition const err_pos = pos; // 记录错误位置
                advance();                          // 消费换行符
                errors.emplace_back("New line in string");
                return {.kind = TokenKind::Error, .lexeme = "", .pos = err_pos};
            }
            if (c == '"') {
                // 提取字符串内容，不包括开头和结尾的引号
                string_view const content = get_substr_from_start(start_pos);
                advance(); // 消费闭引号
                return {
                    .kind = TokenKind::StringLiteral,
                    .lexeme = content,
                    .pos = start_pos};
            }
            advance();
        }
        errors.emplace_back("Unterminated string");
        return {.kind = TokenKind::Error, .lexeme = "", .pos = start_pos};
    }

    Token lex_symbol() { // NOLINT(readability-function-cognitive-complexity)
        auto make_token = [&](TokenKind kind) {
            SourcePosition const start_pos = pos;
            for (int i = 0; i < ssize(to_string(kind)); ++i) {
                advance();
            }
            if (get_substr_from_start(start_pos) != to_string(kind)) {
                throw runtime_error(
                    "Internal error: lexed symbol does not match expected");
            }
            return Token(kind, "", start_pos);
        };

        char const c = peek();
        char const peek1 = peek(1);
        char const peek2 = peek(2);

        switch (c) {
            // G     '/=' '/'
        case '/':
            if (peek1 == '=') {
                return make_token(TokenKind::SlashEq);
            } else {
                return make_token(TokenKind::Slash);
            }

            // G     '<<' '<=>' '<=' '<'
        case '<':
            if (peek1 == '<') {
                return make_token(TokenKind::LessLess);
            } else if (peek1 == '=') {
                if (peek2 == '>') {
                    return make_token(TokenKind::Spaceship);
                }
                return make_token(TokenKind::LessEq);
            } else {
                return make_token(TokenKind::Less);
            }

            // G     '>>' '>=' '>'
        case '>':
            if (peek1 == '>') {
                return make_token(TokenKind::GreaterGreater);
            } else if (peek1 == '=') {
                return make_token(TokenKind::GreaterEq);
            } else {
                return make_token(TokenKind::Greater);
            }

            // G     '++' '+=' '+'
        case '+':
            if (peek1 == '+') {
                return make_token(TokenKind::PlusPlus);
            } else if (peek1 == '=') {
                return make_token(TokenKind::PlusEq);
            } else {
                return make_token(TokenKind::Plus);
            }

            // G     '--' '-=' '->' '-'
        case '-':
            if (peek1 == '-') {
                return make_token(TokenKind::MinusMinus);
            } else if (peek1 == '=') {
                return make_token(TokenKind::MinusEq);
            } else if (peek1 == '>') {
                return make_token(TokenKind::Arrow);
            } else {
                return make_token(TokenKind::Minus);
            }

            // G     '||' '|'
        case '|':
            if (peek1 == '|') {
                return make_token(TokenKind::LogicalOr);
            } else {
                return make_token(TokenKind::Pipe);
            }

            // G     '&&' '&'
        case '&':
            if (peek1 == '&') {
                return make_token(TokenKind::LogicalAnd);
            } else {
                return make_token(TokenKind::Ampersand);
            }

            //  Next,
            //  all the other operators
            //  that have a compound assignment form

            // G     '*=' '*'
        case '*':
            if (peek1 == '=') {
                return make_token(TokenKind::MultiplyEq);
            } else {
                return make_token(TokenKind::Multiply);
            }

            // G     '%=' '%'
        case '%':
            if (peek1 == '=') {
                return make_token(TokenKind::ModuloEq);
            } else {
                return make_token(TokenKind::Modulo);
            }

            // G     '==' '='
        case '=':
            if (peek1 == '=') {
                return make_token(TokenKind::EqualComparison);
            } else {
                return make_token(TokenKind::Assignment);
            }

            // G     '!=' '!'
        case '!':
            if (peek1 == '=') {
                return make_token(TokenKind::NotEqualComparison);
            } else {
                return make_token(TokenKind::Not);
            }

            // G
            // G punctuator: one of
            // G     '.' '..' '...' '..<' '..='
        case '.':
            if (peek1 == '.' && peek2 == '.') {
                return make_token(TokenKind::Ellipsis);
            } else if (peek1 == '.' && peek2 == '<') {
                return make_token(TokenKind::EllipsisLess);
            } else if (peek1 == '.' && peek2 == '=') {
                return make_token(TokenKind::EllipsisEqual);
            } else if (peek1 == '.') {
                return make_token(TokenKind::DotDot);
            } else {
                return make_token(TokenKind::Dot);
            }

            // G     '::' ':'
        case ':':
            if (peek1 == ':') {
                return make_token(TokenKind::Scope);
            } else {
                return make_token(TokenKind::Colon);
            }

            //  All the other single-character tokens

            // G     '^' '~'
        case '^':
            return make_token(TokenKind::Caret);
        case '~':
            return make_token(TokenKind::Tilde);

            // G     '{' '}' '(' ')' '[' ']' ';' ',' '?' '$'
            // G
        case '{':
            return make_token(TokenKind::LeftBrace);
        case '}':
            return make_token(TokenKind::RightBrace);
        case '(':
            return make_token(TokenKind::LeftParen);
        case ')':
            return make_token(TokenKind::RightParen);
        case '[':
            return make_token(TokenKind::LeftBracket);
        case ']':
            return make_token(TokenKind::RightBracket);
        case ';':
            return make_token(TokenKind::Semicolon);
        case ',':
            return make_token(TokenKind::Comma);
        case '?':
            return make_token(TokenKind::QuestionMark);
        case '@':
            return make_token(TokenKind::At);
        case '$':
            return make_token(TokenKind::Dollar);

        default:
            errors.push_back(
                "Unexpected character: " + string(source.substr(pos.index, 1)) +
                " at pos " + pos.to_string());
            return {
                .kind = TokenKind::Error,
                .lexeme = source.substr(pos.index, 1),
                .pos = pos};
        }
    }
};

} // namespace mini_compiler
