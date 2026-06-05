cmake_minimum_required(VERSION 3.20)
project(locadora)

set(CMAKE_CXX_STANDARD 17)

add_executable(locadora main.cpp)

# ─── Testes de consistência ───────────────────────────────────────────────────
add_executable(testes_locadora testes/testes.cpp)

enable_testing()
add_test(NAME consistencia COMMAND testes_locadora)
