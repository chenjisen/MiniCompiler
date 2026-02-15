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
  Error,

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
  Minus,
  Arrow,

  // Special
  Eof
};

//// 1. 在一个头文件或宏定义中列出所有 Token
// #d efine TOKEN_LIST(X) \
//  X(Plus, "+") \
//  X(Minus, "-") \
//  X(Integer, "INT") \ X(Identifier, "ID")
//
//// 2. 自动生成枚举
// enum class TokenKind {
// #define AS_ENUM(kind, str) kind,
//   TOKEN_LIST(AS_ENUM)
// #undef AS_ENUM
// };
//
//// 3. 自动生成映射函数
// constexpr std::string_view to_string(TokenKind kind) {
//   switch (kind) {
// #d ef ine AS_CASE(name, str) \
//  case TokenKind::name: \
//    return str;
//     TOKEN_LIST(AS_CASE)
// #undef AS_CASE
//   default:
//     return "UNKNOWN";
//   }
// }
//
//
// struct TokenInfo {
//   std::string_view name;
//   int precedence;
//   bool right_associative;
// };
//
// class TokenUtils {
// public:
//   // 静态映射表
//   static constexpr TokenInfo get_info(TokenKind kind) {
//     switch (kind) {
// #d ef ine DEFINE_CASE(kind, str, prec, assoc) \
//  case TokenKind::kind: \
//    return {str, prec, assoc == 1};
//       TOKEN_LIST(DEFINE_CASE)
// #undef DEFINE_CASE
//     default:
//       return {"UNKNOWN", 0, false};
//     }
//   }
//
//   // 获取优先级（用于运算符爬行法/普拉特解析法）
//   static constexpr int precedence(TokenKind kind) {
//     return get_info(kind).precedence;
//   }
// };

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
  auto operator<=>(SourcePosition const &) const = default;
  auto to_string() const -> std::string {
    return std::format("({}, {}) i={}", lineno, colno, index);
  }
};

struct Token {
  TokenKind kind{};
  string_view lexeme;
  SourcePosition pos{};
};

static const std::unordered_map<string_view, TokenKind> keywords = {
    {"let", TokenKind::KwLet},       {"fn", TokenKind::KwFn},
    {"return", TokenKind::KwReturn}, {"int", TokenKind::KwInt},
    {"float", TokenKind::KwFloat},   {"string", TokenKind::KwString},
    {"bool", TokenKind::KwBool},     {"void", TokenKind::KwVoid},
    {"true", TokenKind::Bool},       {"false", TokenKind::Bool}};

std::optional<TokenKind> to_lexeme_type(string_view text) {
  if (auto it = keywords.find(text); it != keywords.end())
    return it->second;
  return {};
}

string_view to_string(TokenKind type) {
  for (const auto &[key, value] : keywords) {
    if (value == type) {
      return key;
    }
  }
  return "";
}

// ==========================================
// 2. Lexer
// ==========================================
class Lexer {
public:
  explicit Lexer(string_view src) : source(src) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (!is_at_end()) {
      Token tok = next_token();
      if (tok.kind != TokenKind::Error) // 忽略错误 token 或保留特殊 token
        tokens.emplace_back(std::move(tok));
    }
    tokens.push_back({TokenKind::Eof, "", pos});
    if (!errors.empty()) {
      throw std::runtime_error("Lex error：\n" + errors.front());
    }

    return tokens;
  }

private:
  string_view source;
  SourcePosition pos{};

  std::vector<std::string> errors;

  Token next_token() {
    skip_whitespace();
    if (is_at_end()) {
      return {TokenKind::Eof, "", pos};
    }

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

  bool is_at_end() const { return pos.index >= source.size(); }

  static bool is_ident_start(char c) { return std::isalpha(c) || c == '_'; }
  static bool is_ident_part(char c) { return std::isalnum(c) || c == '_'; }

  char peek(int offset = 0) const {
    int p = pos.index + offset;
    if (p >= source.size())
      return '\0';
    return source[p];
  }

  char advance() {
    if (is_at_end())
      return '\0';
    pos.colno++;
    return source[pos.index++];
  }

  void skip_whitespace() {
    while (!is_at_end()) {
      char c = peek();
      if (std::isspace(c)) {
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
          while (!is_at_end() && peek() != '\n')
            advance();
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }

  Token lex_identifier() {
    SourcePosition start_pos = pos;
    while (!is_at_end() && is_ident_part(peek())) {
      advance();
    }
    string_view text =
        source.substr(start_pos.index, pos.index - start_pos.index);

    TokenKind kind = to_lexeme_type(text).value_or(TokenKind::Identifier);
    return {kind, text, start_pos};
  }

  Token lex_number() {
    SourcePosition start_pos = pos;

    while (!is_at_end() && std::isdigit(peek()))
      advance();

    // Check for float
    if (peek() == '.') {
      advance(); // Consume .
      if (std::isdigit(peek())) {
        while (std::isdigit(peek()))
          advance();
      }
    }

    return {TokenKind::Number,
            source.substr(start_pos.index, pos.index - start_pos.index),
            start_pos};
  }

  Token lex_string_literal() {

    advance(); // consume "
    SourcePosition start_pos = pos;

    while (!is_at_end()) {
      char c = peek();
      if (c == '\\') {
        advance(); // skip escape
        if (!is_at_end()) {
          advance(); // skip escaped char
        }
        char next_c = peek();
        // TODO: escape
        continue;
      } else if (c == '\n') {
        SourcePosition err_pos = pos; // 记录错误位置
        advance();                    // 消费换行符
        errors.push_back("New line in string");
        return {TokenKind::Error, "", err_pos};
      } else if (c == '"') {
        advance(); // 消费闭引号
        // 提取字符串内容，不包括开头和结尾的引号
        string_view content =
            source.substr(start_pos.index, pos.index - start_pos.index - 1);
        return {TokenKind::String, content, start_pos};
      } else {
        advance();
      }
    }
    errors.push_back("Unterminated string");
    return {TokenKind::String, "", start_pos};
  }

  Token lex_symbol() {
    char c = advance();
    switch (c) {
    case '(':
      return {TokenKind::LParen, "(", pos};
    case ')':
      return {TokenKind::RParen, ")", pos};
    case '{':
      return {TokenKind::LBrace, "{", pos};
    case '}':
      return {TokenKind::RBrace, "}", pos};
    case ':':
      return {TokenKind::Colon, ":", pos};
    case ';':
      return {TokenKind::Semicolon, ";", pos};
    case ',':
      return {TokenKind::Comma, ",", pos};
    case '=':
      return {TokenKind::Assign, "=", pos};
    case '-':
      if (peek() == '>') {
        advance();
        return {TokenKind::Arrow, "->", pos};
      }
      return {TokenKind::Minus, "-", pos};
    default:
      string_view sv(&c, 1);
      errors.push_back("Unexpected character: " + string(sv) + " at pos " +
                       pos.to_string());
      return {TokenKind::Error, sv, pos};
    }
  }
};
