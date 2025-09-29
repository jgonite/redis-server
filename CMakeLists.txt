cmake_minimum_required(VERSION 3.10)
project(redis_server LANGUAGES CXX)

# Definir padrão C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags adicionais (sem otimizações redundantes, o CMake já sabe lidar com isso por build type)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread")

# Diretório de fontes
file(GLOB SOURCES src/*.cpp)

# Criar executável
add_executable(redis_server ${SOURCES})
