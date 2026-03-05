cmake_minimum_required(VERSION 3.16...4.0)
project(fortuna-leaderboard-server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prepend our cmake/ directory so custom find-modules override the system ones.
# The system-bundled FindJsoncpp.cmake uses exec_program which was removed in
# CMake 4.0 (CMP0153); our replacement avoids that command.
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Drogon REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/ScoresController.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
