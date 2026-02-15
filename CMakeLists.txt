cmake_minimum_required(VERSION 3.16)

project(lab2_qt20_db_cmake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включает поддержку тестов через CTest и опцию BUILD_TESTING
include(CTest) # BUILD_TESTING + enable_testing() по умолчанию :contentReference[oaicite:2]{index=2}

add_subdirectory(app)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
