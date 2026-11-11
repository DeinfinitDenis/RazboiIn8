cmake_minimum_required(VERSION 3.20)
project(RazboiIn8 LANGUAGES CXX)

add_executable(RazboiIn8 main.cpp text_renderer.cpp text_renderer.h)
set_target_properties(RazboiIn8 PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

# Keep the original TrueType file and its license next to the executable.
add_custom_target(game_assets ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:RazboiIn8>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSans-Regular.ttf"
    "$<TARGET_FILE_DIR:RazboiIn8>/assets/NotoSans-Regular.ttf"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/FONT_LICENSE.txt"
    "$<TARGET_FILE_DIR:RazboiIn8>/assets/FONT_LICENSE.txt"
  VERBATIM)
add_dependencies(RazboiIn8 game_assets)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3>=3.3)
target_link_libraries(RazboiIn8 PRIVATE PkgConfig::GLFW OpenGL::GL)

if(MSVC)
  target_compile_options(RazboiIn8 PRIVATE /W4 /utf-8)
else()
  target_compile_options(RazboiIn8 PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(NAME rules COMMAND RazboiIn8 --self-test)
endif()
