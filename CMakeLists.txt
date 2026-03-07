# CMakeList.txt: MiniCompiler 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.12)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MiniCompiler")

# 将源代码添加到此项目的可执行文件。
add_executable (MiniCompiler "MiniCompiler/MiniCompiler.cpp")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
    set_property(TARGET MiniCompiler PROPERTY CXX_STANDARD 26)
    target_compile_options(MiniCompiler PRIVATE /utf-8)
endif()

target_compile_definitions(MiniCompiler PRIVATE 
    PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

# TODO: 如有需要，请添加测试并安装目标。
