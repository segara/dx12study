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
    Engine/Engine/TableDescriptorHeap.h
    Engine/Engine/TableDescriptorHeap.cpp
    Engine/Engine/RootSignature.h
    Engine/Engine/RootSignature.cpp
    Engine/Engine/Device.h
    Engine/Engine/Device.cpp
    Engine/Engine/SwapChain.h
    Engine/Engine/SwapChain.cpp
    Engine/Engine/ConstantBuffer.h
    Engine/Engine/ConstantBuffer.cpp
    Engine/README.md
    Engine/Resource/Shader.h
    Engine/Resource/Shader.cpp
    Engine/Resource/Mesh.h
    Engine/Resource/Mesh.cpp
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
    Engine/Engine/TableDescriptorHeap.h
    Engine/Engine/TableDescriptorHeap.cpp
    Engine/Engine/RootSignature.h
    Engine/Engine/RootSignature.cpp
    Engine/Engine/ConstantBuffer.h
    Engine/Engine/ConstantBuffer.cpp
)
source_group("Resource" FILES
    Engine/Resource/Shader.h
    Engine/Resource/Shader.cpp
    Engine/Resource/Mesh.h
    Engine/Resource/Mesh.cpp
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

# Library (root/Library)
set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Library")
set(LIBRARY_INCLUDE_DIR "${LIBRARY_DIR}/include")
set(LIBRARY_LIB_DIR "${LIBRARY_DIR}/Lib")

# Lib 폴더의 모든 .lib 파일 (.pdb 등은 링크 대상이 아니므로 제외)
file(GLOB_RECURSE LIBRARY_LIB_FILES CONFIGURE_DEPENDS
    "${LIBRARY_LIB_DIR}/*.lib"
)

# include 폴더 및 하위 폴더의 모든 헤더를 인클루드 경로로 추가
file(GLOB_RECURSE LIBRARY_HEADER_FILES LIST_DIRECTORIES true CONFIGURE_DEPENDS
    "${LIBRARY_INCLUDE_DIR}/*"
)
set(LIBRARY_INCLUDE_DIRS "${LIBRARY_INCLUDE_DIR}")
foreach(path IN LISTS LIBRARY_HEADER_FILES)
    if(IS_DIRECTORY "${path}")
        list(APPEND LIBRARY_INCLUDE_DIRS "${path}")
    endif()
endforeach()

# PUBLIC: Engine에 링크하는 Client도 동일하게 적용됨
target_include_directories(Engine PUBLIC ${LIBRARY_INCLUDE_DIRS})
target_link_directories(Engine PUBLIC "${LIBRARY_LIB_DIR}")
target_link_libraries(Engine PUBLIC ${LIBRARY_LIB_FILES})

# Client
add_executable(Client WIN32
    Client/pch.h
    Client/main.cpp
    Client/Game.h
    Client/Game.cpp
)

target_include_directories(Client PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine"
    ${LIBRARY_INCLUDE_DIRS}
)

target_link_libraries(Client PRIVATE
    ${LIBRARY_LIB_FILES}
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
