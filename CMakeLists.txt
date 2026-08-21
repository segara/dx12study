cmake_minimum_required(VERSION 3.20)

# Visual Studio Solution 이름
project(Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine
add_library(Engine STATIC
    Engine/Utils/pch.h
    Engine/Utils/pch.cpp
    Engine/README.md
    Engine/Resource/README.md
    Engine/Utils/README.md
)

source_group("Engine" FILES
    Engine/README.md
)
source_group("Resource" FILES
    Engine/Resource/README.md
)
source_group("Utils" FILES
    Engine/Utils/pch.h
    Engine/Utils/pch.cpp
    Engine/Utils/README.md
)

target_precompile_headers(Engine PRIVATE
    Engine/Utils/pch.h
)

# Client
add_executable(Client WIN32
    Client/main.cpp
    Client/Game.h
    Client/Game.cpp
)

# Precompiled Header
target_precompile_headers(Client PRIVATE
    Client/pch.h
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
