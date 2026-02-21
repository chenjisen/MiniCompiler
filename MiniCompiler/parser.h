#pragma once

#include "lexer.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace mini_compiler {

using std::format;
using std::optional;
using std::runtime_error;
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

enum class BuiltInType : uint8_t { Never, Unit, Bool, Int, Float, String };

constexpr string_view to_string(BuiltInType const type) {
    switch (type) {
    case BuiltInType::Never:
        return "never";
    case BuiltInType::Unit:
        return "unit";
    case BuiltInType::Bool:
        return "bool";
    case BuiltInType::Int:
        return "int";
    case BuiltInType::Float:
        return "float";
    case BuiltInType::String:
        return "string";
    }
    throw runtime_error("Unknown type");
}

struct Type {
    std::optional<BuiltInType> built_in_type; // For built-in types
    Identifier name;                          // For custom types
};

struct LiteralExpr {
    BuiltInType type;
    string_view value;
};

struct PrefixExpr {
    TokenKind op;
    ExprPtr operand;
};

struct PostfixExpr {
    TokenKind op;
    ExprPtr operand;
};

struct BinaryExpr {
    TokenKind op;
    ExprPtr lhs;
    ExprPtr rhs;
};

struct CallExpr {
    Identifier callee;
    vector<ExprPtr> args;
};

struct ReturnExpr {
    optional<ExprPtr> value;
};

struct AssignExpr {
    ExprPtr lhs;
    ExprPtr rhs;
};

struct Block {
    vector<StmtPtr> stmts;
};

struct VarDecl {
    Identifier name;
    Type type;
    optional<ExprPtr> init;
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
    using Node = std::variant<
        Identifier,
        LiteralExpr,
        CallExpr,
        BinaryExpr,
        PrefixExpr,
        PostfixExpr,
        ReturnExpr,
        AssignExpr>;
    Node node;

    template <typename T> static ExprPtr make(T&& value) {
        return std::make_unique<Expr>(std::forward<T>(value));
    }
};

struct ExprStmt {
    ExprPtr expr;
};

struct Stmt {
    using Node = std::variant<ExprStmt, VarDecl, FunctionDecl>;
    Node node;

    template <typename T> static StmtPtr make(T&& value) {
        return std::make_unique<Stmt>(std::forward<T>(value));
    }
};

struct Program {
    vector<StmtPtr> statements;
};

// ==========================================
// 4. Parser
// ==========================================

class Parser {
  public:
    explicit Parser(vector<Token> tokens) : tokens(std::move(tokens)) {}

    Program parse() {
        Program program;
        while (!match(TokenKind::End)) {
            program.statements.push_back(parse_statement());
        }
        return program;
    }

    auto debug_print(std::ostream& o) const -> void;

  private:
    vector<Token> tokens;
    index_t pos = 0;

    Token const& peek(index_t offset = 0) const {
        if (offset < 0) {
            throw error("Negative lookahead not supported");
        }
        if (pos < 0) {
            throw error("Negative position not supported");
        }
        if (pos + offset >= ssize(tokens)) {
            return tokens.back();
        }
        return tokens[static_cast<size_t>(pos) + offset];
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
        if (!match(kind)) {
            throw error(format(
                "expected {}, got {} {}",
                string(to_string(kind)),
                string(to_string(peek().kind)),
                msg));
        }
        return advance();
    }

    runtime_error error(string_view msg) const {
        return runtime_error(string(msg) + " at pos " + peek().pos.to_string());
    }

    Identifier parse_identifier() {
        return {expect(TokenKind::Identifier).lexeme};
    }

    // type = "int" | "float" | "String" | "bool"
    Type parse_type() {
        auto token = expect(TokenKind::Identifier);
        optional<BuiltInType> type;
        if (token.lexeme == "int") {
            type = BuiltInType::Int;
        }
        if (token.lexeme == "float") {
            type = BuiltInType::Float;
        }
        if (token.lexeme == "string") {
            type = BuiltInType::String;
        }
        if (token.lexeme == "bool") {
            type = BuiltInType::Bool;
        }
        return {.built_in_type = type, .name = {token.lexeme}};
    }

    LiteralExpr parse_literal() {
        optional<BuiltInType> type;
        if (match(TokenKind::IntLiteral)) {
            type = BuiltInType::Int;
        }
        if (match(TokenKind::FloatLiteral)) {
            type = BuiltInType::Float;
        }
        if (match(TokenKind::StringLiteral)) {
            type = BuiltInType::String;
        }
        if (match(TokenKind::BoolLiteral)) {
            type = BuiltInType::Bool;
        }
        if (type) {
            return {.type = *type, .value = advance().lexeme};
        }
        throw error("Expected literal in expression: " + string(peek().lexeme));
    }

    // --- Expressions ---

    // Identifier or Function Call
    ExprPtr parse_call_expression() {
        Identifier const name = parse_identifier();

        // function_call starts with Ident "(" ... ")"
        if (!accept(TokenKind::LeftParen)) {
            return Expr::make(name);
        }

        vector<ExprPtr> args;
        if (!match(TokenKind::RightParen)) {
            do {
                args.push_back(parse_expression());
            } while (accept(TokenKind::Comma));
        }
        expect(TokenKind::RightParen, "after arguments");
        return Expr::make(CallExpr(name, std::move(args)));
    }

    // primary = identifier | literal | function_call | "(" expression ")"
    ExprPtr parse_primary_expression() {
        if (accept(TokenKind::LeftParen)) {
            // parse any expression inside parentheses
            auto expr = parse_expression();
            expect(TokenKind::RightParen, "after expression");
            return expr;
        }

        if (match(TokenKind::Identifier)) {
            return parse_call_expression();
        }

        return Expr::make(parse_literal());
    }

    // parse unary operators
    ExprPtr parse_unary_expression() {

        // 前缀一元运算符（可连续）
        if (is_prefix_unary(peek().kind)) {
            TokenKind const op = advance().kind;
            auto operand = parse_unary_expression();
            return Expr::make(
                PrefixExpr{.op = op, .operand = std::move(operand)});
        }

        // 基础表达式（primary）
        auto expr = parse_primary_expression();

        // 后缀一元运算符（可连续）
        while (is_postfix_unary(peek().kind)) {
            TokenKind const op = advance().kind;
            expr =
                Expr::make(PostfixExpr{.op = op, .operand = std::move(expr)});
        }

        return expr;
    }

    // The core precedence-climbing routine:
    // parse expressions whose precedence is >= min_prec.
    ExprPtr parse_binary_expression(int min_precedence) {
        auto left = parse_unary_expression();

        while (true) {
            TokenKind const op = peek().kind;
            int const precedence = get_precedence(op);
            // 如果当前运算符优先级低于门槛，或者不是运算符，则停止
            if (precedence < min_precedence) {
                break;
            }
            advance(); // consume operator
            // 递归解析右操作数，使用当前优先级 + 1（左结合）
            auto right = parse_binary_expression(precedence + 1);
            left = Expr::make(
                BinaryExpr{
                    .op = op,
                    .lhs = std::move(left),
                    .rhs = std::move(right)});
        }
        return left;
    }

    // assignment_expr = expression "=" expression ";"
    ExprPtr parse_assignment_expression() {
        auto lhs = parse_binary_expression(0);
        if (accept(TokenKind::Assignment)) {
            auto rhs = parse_assignment_expression();
            return Expr::make(AssignExpr{std::move(lhs), std::move(rhs)});
        }
        return lhs;
    }

    // return_expr = "return" [ expression ] ";"
    ExprPtr parse_return_expression() {
        expect(TokenKind::KwReturn);
        optional<ExprPtr> value;
        if (!match(TokenKind::Semicolon)) {
            value = parse_expression();
        }
        return Expr::make(ReturnExpr{std::move(value)});
    }

ExprPtr parse_expression() {
        // 处理 return 表达式
        if (match(TokenKind::KwReturn)) {
            return parse_return_expression();
        }
        return parse_assignment_expression();
    }

    // block = "{" { stmt } "}"
    Block parse_block() {
        expect(TokenKind::LeftBrace, "before function body");
        vector<StmtPtr> stmts;
        while (!match(TokenKind::RightBrace) && !match(TokenKind::End)) {
            stmts.push_back(parse_statement());
        }
        expect(TokenKind::RightBrace, "after function body");
        return {std::move(stmts)};
    }

    // --- Declarations, Statements ---

    // var_declaration = "let" ident ":" type "=" expression ";"
    StmtPtr parse_var_declaration() {
        expect(TokenKind::KwLet);
        Identifier const name = parse_identifier();
        expect(TokenKind::Colon, "after variable name");
        Type const type = parse_type();
        expect(TokenKind::Assignment, "in variable declaration");
        auto init = parse_expression();
        expect(TokenKind::Semicolon, "after variable declaration");
        return Stmt::make(VarDecl(name, type, std::move(init)));
    }

    // function_declaration = "fn" ident "(" [param_list] ")" ["->" type] block
    StmtPtr parse_function_declaration() {
        expect(TokenKind::KwFn, "Expected 'fn'");
        Identifier const name = parse_identifier();
        expect(TokenKind::LeftParen, "after function name");

        vector<Param> params;
        if (!match(TokenKind::RightParen)) {
            do {
                Identifier const paramName = parse_identifier();
                expect(TokenKind::Colon, "after parameter name");
                Type const paramType = parse_type();
                params.push_back({paramName, paramType});
            } while (accept(TokenKind::Comma));
        }
        expect(TokenKind::RightParen, "after parameters");

        // Default return type is unit
        Type return_type = {
            .built_in_type = BuiltInType::Unit, .name = {"return"}};
        if (accept(TokenKind::Arrow)) {
            return_type = parse_type();
        }

        auto body = parse_block();
        return Stmt::make(FunctionDecl(
            name, std::move(params), return_type, std::move(body)));
    }

    // expression_statement = expression ";"
    StmtPtr parse_expression_statement() {
        auto expr = parse_expression();
        expect(TokenKind::Semicolon, "after expression statement");
        // 将表达式包装为语句，需要新增一个 ExprStmt 节点
        return Stmt::make(ExprStmt{std::move(expr)});
    }

    // stmt = var_declaration | function_declaration | expression_statement
    StmtPtr parse_statement() {
        if (match(TokenKind::KwLet)) {
            return parse_var_declaration();
        }
        if (match(TokenKind::KwFn)) {
            return parse_function_declaration();
        }
        return parse_expression_statement();
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

class ParseTreePrinter {
  public:
    explicit ParseTreePrinter(std::ostream& o) : out(o) {}

    void print(Program const& prog) {
        out << "Program\n";
        indent();
        for (auto const& stmt : prog.statements) {
            print(*stmt);
        }
        dedent();
    }

    void print(Stmt const& stmt) {
        std::visit([this](auto const& node) { print(node); }, stmt.node);
    }

    void print(Expr const& expr) {
        std::visit([this](auto const& node) { print(node); }, expr.node);
    }

  private:
    std::ostream& out;
    int level = 0;

    void indent() { level++; }

    void dedent() { level--; }

    void print_indent() {
        for (int i = 0; i < level; ++i) {
            out << "  ";
        }
    }

    void print(ReturnExpr const& node) {
        out << "return";
        if (node.value.has_value()) {
            out << " ";
            print(**node.value);
        } else {
            out << " (void)";
        }
    }

    void print(AssignExpr const& node) {
        print(*node.lhs);
        out << " = ";
        print(*node.rhs);
    }

    void print(ExprStmt const& node) {
        print_indent();
        out << "ExprStmt ";
        print(*node.expr);
        out << "\n";
    }

    void print(VarDecl const& node) {
        print_indent();
        out << "VarDecl " << node.name.name << ": "
            << type_to_string(node.type);
        if (node.init.has_value()) {
            out << " = ";
            print(**node.init);
        }
        out << "\n";
    }

    void print(FunctionDecl const& node) {
        print_indent();
        out << "FunctionDecl " << node.name.name << " -> "
            << type_to_string(node.return_type) << "\n";
        indent();
        print_indent();
        out << "Params:\n";
        indent();
        for (auto const& param : node.params) {
            print_indent();
            out << "Param " << param.name.name << ": "
                << type_to_string(param.type) << "\n";
        }
        dedent();
        print_indent();
        out << "Body:\n";
        print(node.body);
        dedent();
    }

    void print(Block const& block) {
        indent();
        for (auto const& stmt : block.stmts) {
            print(*stmt);
        }
        dedent();
    }

    // 表达式节点
    void print(Identifier const& id) { out << id.name; }

    void print(LiteralExpr const& lit) {
        switch (lit.type) {
        case BuiltInType::Int:
        case BuiltInType::Float:
        case BuiltInType::Bool:
            out << lit.value;
            break;
        case BuiltInType::String:
            out << '"' << lit.value << '"';
            break;
        }
    }

    void print(CallExpr const& call) {
        out << call.callee.name << " (";
        for (int i = 0; i < ssize(call.args); ++i) {
            if (i > 0) {
                out << ", ";
            }
            out << " ";
            print(*call.args[i]);
        }
        out << " )";
    }

    void print(PrefixExpr const& un) {
        out << "(" << to_string(un.op);
        print(*un.operand);
        out << ")";
    }

    void print(BinaryExpr const& bin) {
        out << "(";
        print(*bin.lhs);
        out << " " << to_string(bin.op) << " ";
        print(*bin.rhs);
        out << ")";
    }

    void print(PostfixExpr const& node) {
        out << "(";
        print(*node.operand);
        out << to_string(node.op) << ")";
    }

    static std::string_view type_to_string(Type const& type) {
        if (type.built_in_type.has_value()) {
            return to_string(*type.built_in_type);
        }
        return type.name.name;
    }
};

inline auto parser_debug_print(Program const& program, std::ostream& o)
    -> void {
    auto tree_printer = ParseTreePrinter{o};
    // TODO(cjs): visit(tree_printer);
    tree_printer.print(program);
}

} // namespace mini_compiler
