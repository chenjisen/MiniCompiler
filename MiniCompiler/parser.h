#pragma once

// 下面给出一个**基于你最后那版“修正后的文法”**（仅：`let` 变量声明、`fn`
// 函数声明、block、return、赋值语句、函数调用语句；表达式只有
// primary：标识符/字面量/调用/括号）的**简洁 C++23 Lexer + Parser**。
// 实现方式：手写递归下降；AST 用 `std::unique_ptr`；token 用
// enum；错误用异常抛出。
//
//> 说明（重要且简短）
//> 1) 这是“语法层”解析：不做类型检查/作用域/返回类型推断。
//> 2) `string` 支持基本转义 `\" \\ \n \t \r`（可再扩展）。
//> 3) `number` 支持整数与小数（按你修正版的 `number`）。
//> 4) 解析中用 1-token lookahead，`assignment_stmt` 与 `call_stmt` 通过看
//`IDENT` 后面是否是 `=` 或 `(` 区分。 > 5) `expression` 目前只解析
// primary（无二元运算），与文法一致。

//---

// ## 单文件示例（lexer + parser + AST）

// ```---### 你当前文法仍需注意的点（与代码对应）
// - **语句层**你已经把 `assignment_stmt` 与 `function_call_stmt`
// 分开了，这很好：解析时只需看 `IDENT` 后续是 `=` 还是 `(`。
// - `expression` 只允许 primary，所以像 `a = f(1, g(2));` 仍然是允许的（因为
// `g(2)` 是 primary，作为参数 OK）。但 `a = (b = 1);`
// 不可能被解析（没有赋值表达式），符合“禁止 a=b=c /
// 禁止赋值作为表达式”的目标。
// 如果你希望我把 AST 再压缩（比如不用
// `variant`、只保留必要字段）、或加入一个
// pretty-printer/AST dump
// 方便调试，我也可以继续给一个更短版本。

// 这是一个基于
// C++23 标准编写的词法分析器（Lexer）和语法分析器（Parser）。
// 代码力求简洁，使用了 `string_view`、`std::unique_ptr` 和 `enum class`
// 等现代 C++ 特性。
// ### 设计说明1.  **Lexer**:
// 手写状态机，支持关键字、标识符、数字、字符串和符号（如 `->`）。
// 2.  **Parser**: 递归下降分析法（Recursive Descent）。
// 3.  **AST**: 使用简单的多态类结构表示语法树。
// 4.  **消歧义**: 在解析 `Statement` 时，如果遇到标识符，会预读下一个 Token
// 来区分是 **赋值语句** (`a = ...`) 还是 **表达式语句**
// (如函数调用 `foo();`)。
// ### C++23 代码实现
// ```###代码实现要点1.** 词法分析(Lexer) * * :
// *使用 `std::map` 将字符串快速映射到关键字。
// *   `lexSymbol` 中处理了双字符符号 `->`。
// * 支持字符串字面量转义跳过（虽然未实现完整的转义解析，但能正确识别字符串边界）。
// 2.** 语法分析(Parser) * * :
// * **语句消歧义** : 在 `parseStmt` 中，当遇到
// `TokenKind::Id` 时，我们无法立即判定是赋值还是函数调用。代码检查
// `peek(1)`（下一个 token）：
// * 如果是 `=`，则进入 `AssignStmt`
// 分支（禁止了连续赋值，因为 `AssignStmt` 不返回 Expr）。
// * 否则，当作 `ExprStmt` 处理（通常是函数调用）。
// * **表达式限制 * * :
// `parseExpr` 直接调用
// `parsePrimary`，严格限制了表达式不能包含二元运算符，符合您的要求。
// 3.* *AST *
// * : *使用了 `std::unique_ptr` 管理节点内存，无需手动 delete。 * 利用
// `dynamic_cast` 在 `printAST` 中进行简单的类型匹配（在生产环境中通常会使用
// Visitor 模式，但这更简洁）。 4.* *C++ 23 特性 * * : *使用了
// `string_view` 减少子串复制。 * 使用了 `std::map::contains` (C++ 20)。 *
// 使用了 `std::unique_ptr` 和 `std::move` 确保资源所有权清晰。
// 您可以直接复制这段代码，使用支持 C++ 20 / 23 的编译器（如
// `g++ - std = c++ 23 main.cpp`
// 或
// `clang++ - std = c++ 2b main.cpp`
// ）编译运行。*

// 下面是一个基于您最终文法的C++ 23
// Lexer和Parser实现。代码设计简洁，使用了现代C++
// 特性，并遵循了您定义的语法规则。

//    ###代码特点：
//
//    1. ** 现代C++ 23特性 **：
//    - `std::variant` 用于表示不同类型的AST节点
//    - `std::unique_ptr` 管理AST节点所有权
//    - `std::optional` 处理可选语法元素
//    - 结构化绑定和移动语义
//
//    2. ** 模块化设计**：
//    - Lexer：将源代码转换为Token流
//    - Parser：递归下降解析器构建AST
//    - AST：清晰表示语法结构
//
//    3. ** 错误处理**：
//    - 详细的错误消息包含行号
//    - 使用异常处理语法错误
//
//    4. ** 符合文法**：
//    - 支持变量声明：`let name : Type
//    = value;
//`
//    - 支持函数声明：`fn foo(params) -> Type { ... }
//`
//    - 支持赋值语句和函数调用语句
//    - 支持基本类型：int,
//    float, string_view, bool
//
//    5. ** 简洁性**：
//    - 单个文件实现 - 无外部依赖 - 约300行核心代码
//
//    ###使用说明：
//
//                                  1. 将代码保存为 `parser.cpp` 2. 使用支持C++
//                                  23的编译器编译：
//   ```bash g++
//    - std
//    = c++ 23 - o parser parser.cpp
//   ``` 3. 运行程序：
//   ```bash./ parser
//   ```
//
//        这个实现完全遵循您定义的文法，可以解析符合语法的代码并构建抽象语法树。如果需要扩展功能（如添加运算符、控制流等），可以在现有基础上轻松扩展表达式系统。

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "lexer.h"

using std::string;
using std::string_view;
using std::vector;

// ==========================================
// 3. AST Nodes
// ==========================================

struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Identifier {
  string_view name;
};

struct Type {
  enum class Kind { Never, Unit, Bool, Int, Float, String, Custom } kind;
  string_view name; // For custom types
};

struct LiteralExpr {
  enum class Kind { Int, Float, String, Bool } kind;
  string_view value;
};

struct CallExpr {
  Identifier callee;
  vector<ExprPtr> args;
};

struct VarDecl {
  Identifier name;
  Type type;
  std::optional<ExprPtr> init; // Optional
};

struct ReturnStmt {
  std::optional<ExprPtr> value; // Optional
};

struct AssignStmt {
  Identifier name;
  ExprPtr value;
};

struct Block {
  vector<StmtPtr> stmts;
};

struct CallStmt {
  CallExpr call;
};

struct Param {
  Identifier name;
  Type type;
};

struct FunctionDecl {
  Identifier name;
  vector<Param> params;
  Type return_type;
  Block body;
};

struct Expr {
  using Node = std::variant<Identifier, LiteralExpr, CallExpr>;
  Node node;

  template <typename T> static ExprPtr make(T &&value) {
    return std::make_unique<Expr>(std::forward<T>(value));
  }
};

struct Stmt {
  using Node =
      std::variant<ReturnStmt, AssignStmt, CallStmt, VarDecl, FunctionDecl>;
  Node node;

  template <typename T> static StmtPtr make(T &&value) {
    return std::make_unique<Stmt>(std::forward<T>(value));
  }
};

struct Program {
  vector<StmtPtr> declarations;
};

// ==========================================
// 4. Parser
// ==========================================

class Parser {
public:
  explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

  Program parse() {
    Program program;
    while (!match(TokenKind::Eof)) {
      program.declarations.push_back(parse_declaration());
    }
    return program;
  }

private:
  std::vector<Token> tokens;
  int pos = 0;

  const Token &peek(int offset = 0) const {
    if (pos + offset >= tokens.size())
      return tokens.back();
    return tokens[pos + offset];
  }

  bool match(TokenKind kind, int offset = 0) const {
    return peek(offset).kind == kind;
  }

  Token advance() {
    Token token = peek();
    pos++;
    return token;
  }

  bool accept(TokenKind kind) {
    if (match(kind)) {
      advance();
      return true;
    }
    return false;
  }

  Token expect(TokenKind kind, string_view msg = "") {
    if (!match(kind))
      throw error(std::format("expected {}, got {} {}", string(to_string(kind)),
                              string(to_string(peek().kind)), msg));
    return advance();
  }

  std::runtime_error error(string_view msg) const {
    return std::runtime_error(string(msg) + " at pos " +
                              peek().pos.to_string());
  }

  Identifier parse_identifier() {
    return {expect(TokenKind::Identifier).lexeme};
  }

  // type = "int" | "float" | "String" | "bool"
  Type parse_type() {
    auto token = expect(TokenKind::Identifier);
    std::optional<Type::Kind> kind;
    if (token.lexeme == "int") {
      kind = Type::Kind::Int;
    }
    if (token.lexeme == "float") {
      kind = Type::Kind::Float;
    }
    if (token.lexeme == "string") {
      kind = Type::Kind::String;
    }
    if (token.lexeme == "bool") {
      kind = Type::Kind::Bool;
    }
    if (kind)
      return {*kind, token.lexeme};
    return {Type::Kind::Custom, token.lexeme}; // For user-defined types
  }

  LiteralExpr parse_literal() {
    std::optional<LiteralExpr::Kind> kind;
    if (match(TokenKind::IntLiteral)) {
      kind = LiteralExpr::Kind::Int;
    }
    if (match(TokenKind::FloatLiteral)) {
      kind = LiteralExpr::Kind::Float;
    }
    if (match(TokenKind::StringLiteral)) {
      kind = LiteralExpr::Kind::String;
    }
    if (match(TokenKind::BoolLiteral)) {
      kind = LiteralExpr::Kind::Bool;
    }
    if (kind)
      return {*kind, advance().lexeme};
    throw error("Expected literal in expression: " + string(peek().lexeme));
  }

  // --- Expressions ---

  // expression = primary
  ExprPtr parse_expression() { return parse_primary_expression(); }

  // primary = identifier | literal | function_call | "(" expression ")"
  ExprPtr parse_primary_expression() {
    if (accept(TokenKind::LeftParen)) {
      auto expr = parse_expression();
      expect(TokenKind::RightParen, "after expression");
      return expr;
    }

    // Identifier or Function Call
    if (match(TokenKind::Identifier)) {
      // function_call starts with Ident "(" ... ")"
      if (match(TokenKind::LeftParen, 1)) {
        return parse_call_expression();
      }
      return Expr::make(parse_identifier());
    }

    LiteralExpr lit = parse_literal();
    return Expr::make(lit);
  }

  ExprPtr parse_call_expression() {
    Identifier name = parse_identifier();
    expect(TokenKind::LeftParen, "after function name");
    vector<ExprPtr> args;
    if (!match(TokenKind::RightParen)) {
      do {
        args.push_back(parse_expression());
      } while (accept(TokenKind::Comma));
    }
    expect(TokenKind::RightParen, "after arguments");
    return Expr::make(CallExpr(name, std::move(args)));
  }

  // --- Declarations ---

  // declaration = var_declaration | function_declaration
  StmtPtr parse_declaration() {
    if (match(TokenKind::KwLet)) {
      return parse_var_decl();
    }
    if (match(TokenKind::KwFn)) {
      return parse_function_declaration();
    }
    throw error("Expected declaration");
  }

  // function_declaration = "fn" ident "(" [param_list] ")" ["->" type] block
  StmtPtr parse_function_declaration() {
    expect(TokenKind::KwFn, "Expected 'fn'");
    Identifier name = parse_identifier();
    expect(TokenKind::LeftParen, "after function name");

    vector<Param> params;
    if (!match(TokenKind::RightParen)) {
      do {
        Identifier paramName = parse_identifier();
        expect(TokenKind::Colon, "after parameter name");
        Type paramType = parse_type();
        params.push_back({paramName, paramType});
      } while (accept(TokenKind::Comma));
    }
    expect(TokenKind::RightParen, "after parameters");

    // Default return type is unit
    Type return_type = {Type::Kind::Unit, "unit"};
    if (accept(TokenKind::Arrow))
      return_type = parse_type();

    auto body = parse_block();
    return Stmt::make(
        FunctionDecl(name, std::move(params), return_type, std::move(body)));
  }

  // var_declaration = "let" ident ":" type "=" expression ";"
  StmtPtr parse_var_decl() {
    expect(TokenKind::KwLet);
    Identifier name = parse_identifier();
    expect(TokenKind::Colon, "after variable name");
    Type type = parse_type();
    expect(TokenKind::Assignment, "in variable declaration");
    auto init = parse_expression();
    expect(TokenKind::Semicolon, "after variable declaration");
    return Stmt::make(VarDecl(name, type, std::move(init)));
  }

  // --- Statements ---

  // block = "{" { stmt } "}"
  Block parse_block() {
    expect(TokenKind::LeftBrace, "before function body");
    std::vector<StmtPtr> stmts;
    while (!match(TokenKind::RightBrace) && !match(TokenKind::Eof)) {
      stmts.push_back(parse_stmt());
    }
    expect(TokenKind::RightBrace, "after function body");
    return {std::move(stmts)};
  }

  // assignment_stmt = identifier "=" expression ";"
  StmtPtr parse_assignment_stmt() {
    // Lookahead(1) is '=' -> Assignment Statement
    Identifier name = parse_identifier();
    expect(TokenKind::Assignment, "in assignment");
    auto rhs = parse_expression();
    expect(TokenKind::Semicolon, "after assignment");
    return Stmt::make(AssignStmt(name, std::move(rhs)));
  }

  // function_call_stmt = function_call ";"
  // Here we already consumed callee ident and saw '('
  StmtPtr parse_call_stmt() {
    auto call = parse_call_expression();
    expect(TokenKind::Semicolon, "after call statement");
    CallExpr callee = std::move(std::get<CallExpr>(call->node));
    return Stmt::make(CallStmt(std::move(callee)));
  }

  // return_stmt = "return" [ expression ] ";"
  StmtPtr parse_return_stmt() {
    expect(TokenKind::KwReturn);
    std::optional<ExprPtr> value;
    if (!match(TokenKind::Semicolon)) {
      value = parse_expression();
    }
    expect(TokenKind::Semicolon, "after return");
    return Stmt::make(ReturnStmt(std::move(value)));
  }

  // stmt = var_declaration | return_stmt | assignment_stmt |
  // function_call_stmt
  StmtPtr parse_stmt() {
    if (match(TokenKind::KwLet)) {
      return parse_var_decl();
    }
    if (match(TokenKind::KwReturn)) {
      return parse_return_stmt();
    }

    // must start with identifier for assignment/call
    if (match(TokenKind::Identifier)) {
      // 关键消歧义逻辑： Identifier 可能是 Assignment 或者是
      // FunctionCall(ExprStmt)
      if (match(TokenKind::Assignment, 1)) {
        return parse_assignment_stmt();
      }
      // Expression Statement (mostly function calls)
      if (match(TokenKind::LeftParen, 1))
        return parse_call_stmt();
    }
    throw error("expected statement");
  }
};

// ==========================================
// 5. Demo / Verification//
// ==========================================
// void printAST(const Node *node, int depth = 0) {
//  string_view indent(depth * 2, ' ');
//  if (auto *v = dynamic_cast<const VarDecl *>(node)) {
//    std::cout << indent << "VarDecl: " << v->name << " (" << v->type <<
//    ")\n";
//    printAST(v->init.get(), depth + 1);
//  } else if (auto *f = dynamic_cast<const FunctionDecl *>(node)) {
//    std::cout << indent << "FnDecl: " << f->name << " -> " << f->return_type
//              << "\n";
//    printAST(f->body.get(), depth + 1);
//  } else if (auto *b = dynamic_cast<const Block *>(node)) {
//    std::cout << indent << "Block\n";
//    for (const auto &s : b->stmts)
//      printAST(s.get(), depth + 1);
//  } else if (auto *a = dynamic_cast<const AssignStmt *>(node)) {
//    std::cout << indent << "Assign: " << a->name << "\n";
//    printAST(a->value.get(), depth + 1);
//  } else if (auto *c = dynamic_cast<const CallExpr *>(node)) {
//    std::cout << indent << "Call: " << c->callee << "\n";
//    for (const auto &arg : c->args)
//      printAST(arg.get(), depth + 1);
//  } else if (auto *l = dynamic_cast<const LiteralExpr *>(node)) {
//    std::cout << indent << "Lit: " << l->value << "\n";
//  } else if (auto *es = dynamic_cast<const Expr *>(node)) {
//    std::cout << indent << "ExprStmt\n";
//    printAST(es->expr.get(), depth + 1);
//  }
//}
