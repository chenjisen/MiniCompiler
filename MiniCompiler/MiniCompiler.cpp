// Copyright 2026 Chen Jisen. All rights reserved.
// MiniCompiler.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "lexer.h"
#include "parser.h"

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <stdexcept>
#include <string>

namespace {

using std::format;
using std::string;

string read_file(std::filesystem::path const& path) {
    // 1. 打开流（使用 binary 模式可以避免在 Windows
    // 上因换行符转换导致的偏移错误）
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return "";
    }

    // 2. 直接读取字节流
    auto size = std::filesystem::file_size(path);
    string content(size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(size));
    return content;
}

} // namespace

int main() {
    using namespace mini_compiler;
    string const source = R"(
    let x: int = { let a: int = 2; let b: int = 3; a * b };  // x = 6
    let y: int = 1 + 2 * 3 - 4 / 2;

    fn foo(a: int, b: float) -> bool {
        let s: string = "hi\n";
        x = { return 5; 6 };
        print(s);
        return !(a!=0) && (x > 5);
    }
    fn calc(a: int, b: int, c: int) -> int {
        -c + a * b + 10
    }

    let count: int = 0;
    fn increment(amount: int) -> int {
        count = count + amount;
        return count;  // simplified
    }

    fn test() {
        let a: int = -5;
        let b: int = - -10;
        let flag: bool = !true;
        let x: int = 1 + 2 * 3;
        x = x + 1;
        print(x);
        return;
    }

    fn main() {
        foo(2 - 3, 3.14 * 2);
        x = add(x, 20, 10);
        print("x: ", x);
        let result: int = increment(5);
        print("Result: ", result);
        test();
        let x1: int = if a > b { 5 } else { 10 };
        while i < 10 { i = i + 1; }
        print("Done");
    }
    )";

    try {
        std::ofstream out_lex_file(
            "out/lex.txt", std::ios::out | std::ios::binary);
        if (!out_lex_file) {
            throw std::runtime_error("Failed to open output lex file");
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        for (auto const& token : tokens) {
            string token_str(to_string(token.kind));
            if (!token.lexeme.empty()) {
                token_str = format("{:>10}    ({})", token.lexeme, token_str);
            }
            std::println(
                out_lex_file,
                "{:>2}:{:>2}    {}",
                token.pos.lineno,
                token.pos.colno,
                token_str);
        }
        Parser parser(tokens);
        Program const prog = parser.parse();
        std::cout << "Parsed OK. Statements=" << prog.statements.size() << "\n";

        std::ofstream out_parser_file(
            "out/parser.txt", std::ios::out | std::ios::binary);
        if (!out_parser_file) {
            throw std::runtime_error("Failed to open output file");
        }
        parser_debug_print(prog, out_parser_file);
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
