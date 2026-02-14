// MiniCompiler.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "lexer.h"
#include "parser.h"



int main() {

	std::cout << "Hello World!\n";

	string source1 = R"(
      let x: int = 123;
      fn foo(a: int, b: float) -> bool {
          let y: string_view = "hi\n";
          x = 1;
          print(y);
          return true;
      }
    )";

	string source2 =
		R"(
            let x: int = 10;
            fn add(a: int, b: int) -> int {
                return a; // simplified
            }
            fn main() {
                x = add(x, 20);
                print("Done");
            }
)";
	string source3 = R"(
        let count: int = 0;

        fn increment(amount: int) -> int {
            count = count + amount;
            return count;
        }

        fn main() {
            let result: int = increment(5);
            print("Result: ", result);
        }
    )";

	try {
		for (auto source : { source1, source2, source3 }) {
			Lexer lexer(std::move(source));
			auto tokens = lexer.tokenize();
			Parser parser(tokens);
			Program prog = parser.parse();
			std::cout << "Parsed OK. decls=" << prog.declarations.size() << "\n";
		}

		// std::cout << "AST Structure:\n";
		// for (const auto &decl : prog.declarations) {
		//   printAST(decl.get());
		// }
	}
	catch (const std::exception& e) {
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
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
