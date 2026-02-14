#pragma once

#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

using std::string;
using std::string_view;
using std::vector;

// ==========================================
// 1. Token Definitions
// ==========================================

enum class TokenKind {
  // Keywords
  KwLet,
  KwFn,
  KwReturn,
  KwInt,
  KwFloat,
  KwString,
  KwBool,
  KwVoid,

  // Identifiers
  Identifier,

  // Literals
  Number,
  String,
  Bool,

  Type,

  // Delimiters
  LParen,
  RParen,
  LBrace,
  RBrace,

  // Operators
  Comma,
  Colon,
  Semicolon,
  Assign,
  Arrow,

  // Special
  Eof
};

struct Token {
  TokenKind kind{};
  string_view lexeme;
  int line{};
  int pos{};
};

static const std::unordered_map<string_view, TokenKind> keywords = {
    {"let", TokenKind::KwLet},       {"fn", TokenKind::KwFn},
    {"return", TokenKind::KwReturn}, {"int", TokenKind::KwInt},
    {"float", TokenKind::KwFloat},   {"string", TokenKind::KwString},
    {"bool", TokenKind::KwBool},     {"void", TokenKind::KwVoid},
    {"true", TokenKind::Bool},       {"false", TokenKind::Bool}};

// ==========================================
// 2. Lexer
// ==========================================
class Lexer {
public:
  explicit Lexer(string src) : source(std::move(src)), pos(0), line(1) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (!is_at_end()) {
      tokens.push_back(next_token());
    }
    return tokens;
  }

private:
  string source;
  int start = 0;
  int pos = 0;
  int line = 1;

  Token next_token() {
    skip_whitespace();
    start = pos;

    if (is_at_end())
      return {TokenKind::Eof, "", line};

    char c = peek();

    // number: digits ( '.' digits )?
    if (std::isdigit(c)) {
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

  bool is_at_end() const { return pos >= source.size(); }

  static bool is_ident_start(char c) { return std::isalpha(c) || c == '_'; }
  static bool is_ident_part(char c) { return std::isalnum(c) || c == '_'; }

  char peek(int offset = 0) const {
    int p = pos + offset;
    if (p >= source.size())
      return '\0';
    return source[p];
  }

  char advance() {
    if (is_at_end())
      return '\0';
    return source[pos++];
  }

  void skip_whitespace() {
    while (!is_at_end()) {
      char c = peek();
      if (std::isspace(c)) {
        if (c == '\n')
          line++;
        advance();
      } else if (c == '/') {
        if (advance() == '/') {
          // line comment //
          while (peek() != '\n' && !is_at_end())
            advance();
          line++;
        } else {
          return;
        }
      } else {
        return;
      }
    }
  }

  Token lex_identifier() {
    int start = pos;
    while (!is_at_end() && (is_ident_part(peek())))
      advance();
    string_view text = source.substr(start, pos - start);
    if (keywords.contains(text)) {
      return {keywords.at(text), text, line};
    }
    return {TokenKind::Identifier, text, line};
  }

  Token lex_number() {
    int start = pos;
    while (!is_at_end() && std::isdigit(peek()))
      advance();

    // Check for float
    if (peek() == '.' && std::isdigit(peek(1))) {
      advance(); // Consume .
      while (std::isdigit(peek()))
        advance();
    }

    return {TokenKind::Number, source.substr(start, pos - start), line};
  }

  Token lex_string_literal() {
    advance(); // "
    int start = pos;
    while (!is_at_end() && peek() != '"') {
      if (peek() == '\n') {
        throw std::runtime_error("newline in string literal");
      }
      if (peek() == '\\')
        advance(); // skip escape
      advance();
    }

    if (is_at_end()) {
      throw std::runtime_error("Unterminated string at line " +
                               std::to_string(line));
    }
    advance(); // Closing "
    return {TokenKind::String, source.substr(start + 1, pos - start - 2), line};
  }

  Token lex_symbol() {
    char c = advance();
    switch (c) {
    case '(':
      return {TokenKind::LParen, "(", line};
    case ')':
      return {TokenKind::RParen, ")", line};
    case '{':
      return {TokenKind::LBrace, "{", line};
    case '}':
      return {TokenKind::RBrace, "}", line};
    case ':':
      return {TokenKind::Colon, ":", line};
    case ';':
      return {TokenKind::Semicolon, ";", line};
    case ',':
      return {TokenKind::Comma, ",", line};
    case '=':
      return {TokenKind::Assign, "=", line};
    case '-':
      if (advance() == '>') {
        return {TokenKind::Arrow, "->", line};
      }
    }
    throw std::runtime_error("Unexpected character: " + string(1, c) +
                             " at pos " + std::to_string(pos));
  }
};
