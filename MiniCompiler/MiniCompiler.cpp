// MiniCompiler.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "lexer.h"
#include "parser.h"

#include <filesystem>
#include <fstream>
#include <print>
#include <sstream>
#include <string>

std::string read_file(const std::filesystem::path &path) {
  // 1. 打开流（使用 binary 模式可以避免在 Windows
  // 上因换行符转换导致的偏移错误）
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file)
    return "";

  // 2. 直接读取字节流
  auto size = std::filesystem::file_size(path);
  std::string content(size, '\0');
  file.read(content.data(), size);

  return content;
}

int main() {

  string source = R"(
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
    std::ofstream out_file("out\\lex.txt", std::ios::out | std::ios::binary);
    if (!out_file) {
      throw std::runtime_error("Failed to open output file");
    }

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    for (const auto &token : tokens) {
      std::string token_str = std::format("{:>10}", token.lexeme);
      if (auto token_kind_str = to_string(token.kind);
          token_kind_str != token.lexeme) {
        token_str += std::format(" | ({})", token_kind_str);
      }
      std::println(out_file, "{:>2}:{:>2} | {}", token.pos.lineno,
                   token.pos.colno, token_str);
    }
    Parser parser(tokens);
    Program prog = parser.parse();
    std::cout << "Parsed OK. decls=" << prog.declarations.size() << "\n";

    // std::cout << "AST Structure:\n";
    // for (const auto &decl : prog.declarations) {
    //   printAST(decl.get());
    // }
  } catch (const std::exception &e) {
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
