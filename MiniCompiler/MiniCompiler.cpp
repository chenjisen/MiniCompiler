// MiniCompiler.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

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
    let x: int = 123;
    fn foo(a: int, b: float) -> bool {
        let y: string = "hi\n";
        x = 1;
        print(y);
        return true;
    }
    fn add(a: int, b: int) -> int {
         return a; // simplified
    }

    let count: int = 0;
    fn increment(amount: int) -> int {
        // count = count + amount;
        return count;  // simplified
    }

    fn main() {
        foo(10, 3.14);
        x = add(x, 20);
        print("x: ", x);
        let result: int = increment(5);
        print("Result: ", result);
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
            string token_str = format("{}", token.lexeme);
            if (auto token_kind_str = to_string(token.kind);
                token_kind_str != token.lexeme) {
                token_str = format("{:<10} ({})", token.lexeme, token_kind_str);
            }
            std::println(
                out_lex_file,
                "{:>2}:{:>2}    {:>5}",
                token.pos.lineno,
                token.pos.colno,
                token_str);
        }
        Parser parser(tokens);
        Program const prog = parser.parse();
        std::cout << "Parsed OK. decls=" << prog.declarations.size() << "\n";

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

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧:
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5.
//   转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
