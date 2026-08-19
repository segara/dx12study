cmake_minimum_required(VERSION 3.20)

# Visual Studio Solution 이름
project(Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Client
add_executable(Client WIN32
    Client/main.cpp
)

target_compile_definitions(Client PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# Visual Studio Startup Project
set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT Client
)