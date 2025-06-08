cmake_minimum_required(VERSION 3.10)
project(MyMuduoProject CXX)

# 设置 C++ 标准，例如 C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 通常建议关闭编译器特定扩展

# 添加包含目录
# 因为源文件中的 #include 指令是类似 "muduo/base/Xxx.h" 的形式，
# 所以需要将项目源码的根目录添加到包含路径中。
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/muduo)
# 查找 muduo 目录及其子目录下的所有 .cpp 源文件
# CONFIGURE_DEPENDS 确保当源文件列表变化时，CMake 会重新生成构建系统
file(GLOB_RECURSE MUDUO_SOURCES CONFIGURE_DEPENDS "muduo/**/*.cpp")

# 检查是否找到了源文件
if(NOT MUDUO_SOURCES)
    message(WARNING "No source files found in the 'muduo' directory. Please check the path and file patterns.")
else()
    message(STATUS "Found Muduo sources: ${MUDUO_SOURCES}")
endif()

# 添加一个库目标（例如，静态库 myMuduoLib）
# 如果你想创建共享库，请使用 SHARED 而不是 STATIC
add_library(myMuduo STATIC ${MUDUO_SOURCES})

# (可选) 设置库的输出目录
# 对于静态库，使用 ARCHIVE_OUTPUT_DIRECTORY
# 对于共享库，使用 LIBRARY_OUTPUT_DIRECTORY
set_target_properties(myMuduo PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")  # 输出到构建目录下的 lib 文件夹
#     # 或者输出到源码目录下的 lib 文件夹 (通常不推荐直接输出到源码树)
#     # ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
# )

#如果你有一个测试用的可执行文件 main.cpp
add_executable(test "./test/test.cpp")
target_link_libraries(test PRIVATE myMuduo)
# MyApp 可能也需要 ${PROJECT_SOURCE_DIR} 作为包含目录
target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR})
set_target_properties(test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/test"
)
# (可选) 添加编译选项
# target_compile_options(myMuduoLib PRIVATE -Wall -Wextra -g) # 示例编译选项

# (可选) 如果你的库需要链接其他库，例如 pthread
# target_link_libraries(myMuduoLib PUBLIC pthread) # 或者 PRIVATE/INTERFACE