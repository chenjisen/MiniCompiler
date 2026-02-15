#pragma once

#include <cctype>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mini_compiler {

using std::string;
using std::string_view;
using std::vector;

// ==========================================
// 1. Token Definitions
// ==========================================

#define TOKEN_LIST(X)                                                          \
  X(SlashEq, "/=", SlashEq)                                                    \
  X(Slash, "/", Slash)                                                         \
  X(LeftShiftEq, "<<=", LeftShiftEq)                                           \
  X(LeftShift, "<<", LeftShift)                                                \
  X(Spaceship, "<=>", Spaceship)                                               \
  X(LessEq, "<=", LessEq)                                                      \
  X(Less, "<", Less)                                                           \
  X(RightShiftEq, ">>=", RightShiftEq)                                         \
  X(RightShift, ">>", RightShift)                                              \
  X(GreaterEq, ">=", GreaterEq)                                                \
  X(Greater, ">", Greater)                                                     \
  X(PlusPlus, "++", PlusPlus)                                                  \
  X(PlusEq, "+=", PlusEq)                                                      \
  X(Plus, "+", Plus)                                                           \
  X(MinusMinus, "--", MinusMinus)                                              \
  X(MinusEq, "-=", MinusEq)                                                    \
  X(Arrow, "->", Arrow)                                                        \
  X(Minus, "-", Minus)                                                         \
  X(LogicalOrEq, "||=", LogicalOrEq)                                           \
  X(LogicalOr, "||", LogicalOr)                                                \
  X(PipeEq, "|=", PipeEq)                                                      \
  X(Pipe, "|", Pipe)                                                           \
  X(LogicalAndEq, "&&=", LogicalAndEq)                                         \
  X(LogicalAnd, "&&", LogicalAnd)                                              \
  X(MultiplyEq, "*=", MultiplyEq)                                              \
  X(Multiply, "*", Multiply)                                                   \
  X(ModuloEq, "%=", ModuloEq)                                                  \
  X(Modulo, "%", Modulo)                                                       \
  X(AmpersandEq, "&=", AmpersandEq)                                            \
  X(Ampersand, "&", Ampersand)                                                 \
  X(CaretEq, "^=", CaretEq)                                                    \
  X(Caret, "^", Caret)                                                         \
  X(TildeEq, "~=", TildeEq)                                                    \
  X(Tilde, "~", Tilde)                                                         \
  X(EqualComparison, "==", EqualComparison)                                    \
  X(Assignment, "=", Assignment)                                               \
  X(NotEqualComparison, "!=", NotEqualComparison)                              \
  X(Not, "!", Not)                                                             \
  X(LeftBrace, "{", LeftBrace)                                                 \
  X(RightBrace, "}", RightBrace)                                               \
  X(LeftParen, "(", LeftParen)                                                 \
  X(RightParen, ")", RightParen)                                               \
  X(LeftBracket, "[", LeftBracket)                                             \
  X(RightBracket, "]", RightBracket)                                           \
  X(Scope, "::", Scope)                                                        \
  X(Colon, ":", Colon)                                                         \
  X(Semicolon, ";", Semicolon)                                                 \
  X(Comma, ",", Comma)                                                         \
  X(Dot, ".", Dot)                                                             \
  X(DotDot, "..", DotDot)                                                      \
  X(Ellipsis, "...", Ellipsis)                                                 \
  X(EllipsisLess, "..<", EllipsisLess)                                         \
  X(EllipsisEqual, "..=", EllipsisEqual)                                       \
  X(QuestionMark, "?", QuestionMark)                                           \
  X(At, "@", At)                                                               \
  X(Dollar, "$", Dollar)                                                       \
  X(FloatLiteral, "Float Literal", Literal)                                    \
  X(IntLiteral, "Int Literal", Literal)                                        \
  X(StringLiteral, "String Literal", Literal)                                  \
  X(CharLiteral, "Char Literal", Literal)                                      \
  X(BoolLiteral, "Bool Literal", Literal)                                      \
  X(KwLet, "Let", Keyword)                                                     \
  X(KwFn, "Fn", Keyword)                                                       \
  X(KwReturn, "Return", Keyword)                                               \
  X(Identifier, "Identifier", Identifier)

enum class TokenKind {
  Error,

#define AS_ENUM(name, str, kind_class) name,
  TOKEN_LIST(AS_ENUM)
#undef AS_ENUM

      Eof,
  None,
};

inline bool is_keyword(string_view sv) {
  return sv == "let" || sv == "fn" || sv == "return";
}

constexpr std::string_view to_string(TokenKind kind) {
  switch (kind) {
#define AS_CASE(name, str, kind_class)                                         \
  case TokenKind::name:                                                        \
    return str;
    TOKEN_LIST(AS_CASE)
#undef AS_CASE
  case TokenKind::Error:
    return "(ERROR)";
  case TokenKind::Eof:
    return "(EOF)";
  case TokenKind::None:
    return "(NONE)";
  default:
    return "(UNKNOWN)";
  }
}

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
  SourcePosition pos;
};

// ==========================================
// 2. Lexer
// ==========================================
class Lexer {
public:
  explicit Lexer(string_view src) : source(src) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (!is_at_end()) {
      Token const tok = next_token();
      if (tok.kind != TokenKind::Error) { // 忽略错误 token 或保留特殊 token
        tokens.emplace_back(tok);
      }
    }
    tokens.push_back({TokenKind::Eof, "", pos});
    if (!errors.empty()) {
      std::string msg = "Lex errors:\n";
      for (const auto &e : errors) {
        msg += e + "\n";
      }
      throw std::runtime_error(msg);
    }

    return tokens;
  }

private:
  string_view source;
  SourcePosition pos;

  std::vector<std::string> errors;

  Token next_token() {
    skip_whitespace();
    if (is_at_end()) {
      return {.kind = TokenKind::Eof, .lexeme = "", .pos = pos};
    }

    char const c = peek();

    // number: digits ( '.' digits )?
    if (std::isdigit(c) != 0) {
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

  static bool is_ident_start(char c) {
    return (std::isalpha(c) != 0) || c == '_';
  }
  static bool is_ident_part(char c) {
    return (std::isalnum(c) != 0) || c == '_';
  }

  char peek(int offset = 0) const {
    int const p = pos.index + offset;
    if (p >= source.size()) {
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
    SourcePosition const start_pos = pos;
    while (!is_at_end() && is_ident_part(peek())) {
      advance();
    }
    string_view const text =
        source.substr(start_pos.index, pos.index - start_pos.index);
    if (is_keyword(text)) {
      if (text == "let") {
        return {.kind = TokenKind::KwLet, .lexeme = text, .pos = start_pos};
      }
      if (text == "fn") {
        return {.kind = TokenKind::KwFn, .lexeme = text, .pos = start_pos};
      }
      if (text == "return") {
        return {.kind = TokenKind::KwReturn, .lexeme = text, .pos = start_pos};
      }
      throw std::runtime_error("Unknown keyword: " + string(text));
    }
    return {.kind = TokenKind::Identifier, .lexeme = text, .pos = start_pos};
  }

  Token lex_number() {
    SourcePosition const start_pos = pos;

    while (!is_at_end() && (std::isdigit(peek()) != 0)) {
      advance();
    }

    TokenKind kind = TokenKind::None;

    // Check for float
    if (peek() == '.') {
      kind = TokenKind::FloatLiteral;
      advance(); // Consume .
      if (std::isdigit(peek()) != 0) {
        while (std::isdigit(peek()) != 0) {
          advance();
        }
      }
    } else {
      kind = TokenKind::IntLiteral;
    }

    return {.kind = kind,
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
        advance(); // 消费闭引号
        // 提取字符串内容，不包括开头和结尾的引号
        string_view const content =
            source.substr(start_pos.index, pos.index - start_pos.index - 1);
        return {.kind = TokenKind::StringLiteral,
                .lexeme = content,
                .pos = start_pos};
      }
      advance();
    }
    errors.emplace_back("Unterminated string");
    return {.kind = TokenKind::Error, .lexeme = "", .pos = start_pos};
  }

  Token lex_symbol() {
    auto make_token = [&](TokenKind kind) {
      SourcePosition const start_pos = pos;
      for (int i = 0; i < to_string(kind).size(); ++i) {
        advance();
      }
      string_view const lexeme =
          source.substr(start_pos.index, pos.index - start_pos.index);
      return Token(kind, lexeme, start_pos);
    };

    char const c = peek();
    auto peek1 = peek(1);
    auto peek2 = peek(2);

    switch (c) {
      // G     '/=' '/'
    case '/':
      if (peek1 == '=') {
        return make_token(TokenKind::SlashEq);
      } else {
        return make_token(TokenKind::Slash);
      }

      // G     '<<=' '<<' '<=>' '<=' '<'
      break;
    case '<':
      if (peek1 == '<') {
        if (peek2 == '=') {
          return make_token(TokenKind::LeftShiftEq);
        }
        return make_token(TokenKind::LeftShift);
      } else if (peek1 == '=') {
        if (peek2 == '>') {
          return make_token(TokenKind::Spaceship);
        }
        return make_token(TokenKind::LessEq);
      } else {
        return make_token(TokenKind::Less);
      }

      // G     '>>=' '>>' '>=' '>'
      break;
    case '>':
      if (peek1 == '>') {
        if (peek2 == '=') {
          return make_token(TokenKind::RightShiftEq);
        }
        return make_token(TokenKind::RightShift);
      } else if (peek1 == '=') {
        return make_token(TokenKind::GreaterEq);
      } else {
        return make_token(TokenKind::Greater);
      }

      // G     '++' '+=' '+'
      break;
    case '+':
      if (peek1 == '+') {
        return make_token(TokenKind::PlusPlus);
      } else if (peek1 == '=') {
        return make_token(TokenKind::PlusEq);
      } else {
        return make_token(TokenKind::Plus);
      }

      // G     '--' '-=' '->' '-'
      break;
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

      // G     '||=' '||' '|=' '|'
      break;
    case '|':
      if (peek1 == '|') {
        if (peek2 == '=') {
          return make_token(TokenKind::LogicalOrEq);
        }
        return make_token(TokenKind::LogicalOr);
      } else if (peek1 == '=') {
        return make_token(TokenKind::PipeEq);
      } else {
        return make_token(TokenKind::Pipe);
      }

      // G     '&&=' '&&' '&=' '&'
      break;
    case '&':
      if (peek1 == '&') {
        if (peek2 == '=') {
          return make_token(TokenKind::LogicalAndEq);
        }
        return make_token(TokenKind::LogicalAnd);
      } else if (peek1 == '=') {
        return make_token(TokenKind::AmpersandEq);
      } else {
        return make_token(TokenKind::Ampersand);
      }

      //  Next, all the other operators that have a compound assignment form

      // G     '*=' '*'
      break;
    case '*':
      if (peek1 == '=') {
        return make_token(TokenKind::MultiplyEq);
      } else {
        return make_token(TokenKind::Multiply);
      }

      // G     '%=' '%'
      break;
    case '%':
      if (peek1 == '=') {
        return make_token(TokenKind::ModuloEq);
      } else {
        return make_token(TokenKind::Modulo);
      }

      // G     '^=' '^'
      break;
    case '^':
      if (peek1 == '=') {
        return make_token(TokenKind::CaretEq);
      } else {
        return make_token(TokenKind::Caret);
      }

      // G     '~=' '~'
      break;
    case '~':
      if (peek1 == '=') {
        return make_token(TokenKind::TildeEq);
      } else {
        return make_token(TokenKind::Tilde);
      }

      // G     '==' '='
      break;
    case '=':
      if (peek1 == '=') {
        return make_token(TokenKind::EqualComparison);
      } else {
        return make_token(TokenKind::Assignment);
      }

      // G     '!=' '!'
      break;
    case '!':
      if (peek1 == '=') {
        return make_token(TokenKind::NotEqualComparison);
      } else {
        return make_token(TokenKind::Not);
      }

      // G
      // G punctuator: one of
      // G     '.' '..' '...' '..<' '..='
      break;
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
      break;
    case ':':
      if (peek1 == ':') {
        return make_token(TokenKind::Scope);
      } else {
        return make_token(TokenKind::Colon);
      }

      //  All the other single-character tokens

      // G     '{' '}' '(' ')' '[' ']' ';' ',' '?' '$'
      // G

      break;
    case '{':
      return make_token(TokenKind::LeftBrace);

      break;
    case '}':
      return make_token(TokenKind::RightBrace);

      break;
    case '(':
      return make_token(TokenKind::LeftParen);

      break;
    case ')':
      return make_token(TokenKind::RightParen);

      break;
    case '[':
      return make_token(TokenKind::LeftBracket);

      break;
    case ']':
      return make_token(TokenKind::RightBracket);

      break;
    case ';':
      return make_token(TokenKind::Semicolon);

      break;
    case ',':
      return make_token(TokenKind::Comma);

      break;
    case '?':
      return make_token(TokenKind::QuestionMark);

      break;
    case '@':
      return make_token(TokenKind::At);

      break;
    case '$':
      return make_token(TokenKind::Dollar);
      break;
    default:
      string_view const sv(&c, 1);
      errors.push_back("Unexpected character: " + string(sv) + " at pos " +
                       pos.to_string());
      return {.kind = TokenKind::Error,
              .lexeme = std::string_view(&c, 1),
              .pos = pos};
    }
  }
};

} // namespace mini_compiler
