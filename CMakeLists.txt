cmake_minimum_required(VERSION 3.20)

# Visual Studio Solution 이름
project(Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Solution output directory.  The generator expression prevents multi-config
# generators such as Visual Studio from appending the configuration name.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/Output>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/Output>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/Output>")

# Engine
add_library(Engine STATIC
    Engine/Utils/pch.h
    Engine/Utils/pch.cpp
    Engine/Utils/EnginePch.h
    Engine/Utils/EnginePch.cpp
    Engine/Engine/Engine.h
    Engine/Engine/Engine.cpp
    Engine/Engine/CommandQueue.h
    Engine/Engine/CommandQueue.cpp
    Engine/Engine/DescriptorHeap.h
    Engine/Engine/DescriptorHeap.cpp
    Engine/Engine/Device.h
    Engine/Engine/Device.cpp
    Engine/Engine/SwapChain.h
    Engine/Engine/SwapChain.cpp
    Engine/README.md
    Engine/Resource/README.md
    Engine/Utils/README.md
)

source_group("Engine" FILES
    Engine/README.md
    Engine/Engine/Engine.h
    Engine/Engine/Engine.cpp
    Engine/Engine/CommandQueue.h
    Engine/Engine/CommandQueue.cpp
    Engine/Engine/Device.h
    Engine/Engine/Device.cpp
    Engine/Engine/SwapChain.h
    Engine/Engine/SwapChain.cpp
    Engine/Engine/DescriptorHeap.h
    Engine/Engine/DescriptorHeap.cpp
)
source_group("Resource" FILES
    Engine/Resource/README.md
)
source_group("Utils" FILES
    Engine/Utils/pch.h
    Engine/Utils/pch.cpp
    Engine/Utils/EnginePch.h
    Engine/Utils/EnginePch.cpp
    Engine/Utils/README.md
)

target_precompile_headers(Engine PRIVATE
    Engine/Utils/pch.h
)

# Client
add_executable(Client WIN32
    Client/pch.h
    Client/main.cpp
    Client/Game.h
    Client/Game.cpp
)

target_include_directories(Client PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine"
)

target_link_directories(Client PRIVATE
    "$<1:${CMAKE_BINARY_DIR}/Output>"
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
