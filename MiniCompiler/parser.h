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

inline string_view to_string(BuiltInType const type) {
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

struct CallExpr {
    Identifier callee;
    vector<ExprPtr> args;
};

struct VarDecl {
    Identifier name;
    Type type;
    optional<ExprPtr> init;
};

struct ReturnStmt {
    optional<ExprPtr> value;
};

struct AssignStmt {
    ExprPtr left;
    ExprPtr right;
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

    template <typename T> static ExprPtr make(T&& value) {
        return std::make_unique<Expr>(std::forward<T>(value));
    }
};

struct Stmt {
    using Node =
        std::variant<ReturnStmt, AssignStmt, CallStmt, VarDecl, FunctionDecl>;
    Node node;

    template <typename T> static StmtPtr make(T&& value) {
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
    explicit Parser(vector<Token> tokens) : tokens(std::move(tokens)) {}

    Program parse() {
        Program program;
        while (!match(TokenKind::Eof)) {
            program.declarations.push_back(parse_declaration());
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
        if (pos + offset >= std::ssize(tokens)) {
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
        Identifier const name = parse_identifier();
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

    // var_declaration = "let" ident ":" type "=" expression ";"
    StmtPtr parse_var_decl() {
        expect(TokenKind::KwLet);
        Identifier const name = parse_identifier();
        expect(TokenKind::Colon, "after variable name");
        Type const type = parse_type();
        expect(TokenKind::Assignment, "in variable declaration");
        auto init = parse_expression();
        expect(TokenKind::Semicolon, "after variable declaration");
        return Stmt::make(VarDecl(name, type, std::move(init)));
    }

    // --- Statements ---

    // block = "{" { stmt } "}"
    Block parse_block() {
        expect(TokenKind::LeftBrace, "before function body");
        vector<StmtPtr> stmts;
        while (!match(TokenKind::RightBrace) && !match(TokenKind::Eof)) {
            stmts.push_back(parse_stmt());
        }
        expect(TokenKind::RightBrace, "after function body");
        return {std::move(stmts)};
    }

    // assignment_stmt = expression "=" expression ";"
    StmtPtr parse_assignment_stmt() {
        // Lookahead(1) is '=' -> Assignment Statement
        auto lhs = parse_expression();
        expect(TokenKind::Assignment, "in assignment");
        auto rhs = parse_expression();
        expect(TokenKind::Semicolon, "after assignment");
        return Stmt::make(AssignStmt(std::move(lhs), std::move(rhs)));
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
        optional<ExprPtr> value;
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
            if (match(TokenKind::LeftParen, 1)) {
                return parse_call_stmt();
            }
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

class ParseTreePrinter {
  public:
    explicit ParseTreePrinter(std::ostream& o) : out(o) {}

    void print(Program const& prog) {
        out << "Program\n";
        indent();
        for (auto const& stmt : prog.declarations) {
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

    // 语句节点
    void print(ReturnStmt const& node) {
        print_indent();
        out << "ReturnStmt";
        if (node.value.has_value()) {
            out << " ";
            print(**node.value);
        } else {
            out << " (void)";
        }
        out << "\n";
    }

    void print(AssignStmt const& node) {
        print_indent();
        out << "AssignStmt ";
        print(*node.left);
        out << " = ";
        print(*node.right);
        out << "\n";
    }

    void print(CallStmt const& node) {
        print_indent();
        out << "CallStmt ";
        print(node.call);
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
        out << call.callee.name << "(";
        for (size_t i = 0; i < call.args.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }
            print(*call.args[i]);
        }
        out << ")";
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
