cmake_minimum_required(VERSION 3.9)
set(CXX_STANDARD 11)

project("DB")
include_directories("includes")
add_executable("db.out" "src/db.cpp")