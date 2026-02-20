# app/CMakeLists.txt

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Sql)

add_library(lab2_core INTERFACE)
target_include_directories(lab2_core
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(lab2_core
  INTERFACE
    Qt5::Core
    Qt5::Sql
    Qt5::Widgets
)

add_library(lab2_ui STATIC
  src/mainwindow.h
  src/mainwindow.cpp
  src/mainwindow.ui
  src/mydelegate.h
  src/mydelegate.cpp
)

target_link_libraries(lab2_ui
  PUBLIC
    lab2_core
    Qt5::Widgets
    Qt5::Sql
    Qt5::Core
)

target_include_directories(lab2_ui
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(lab2_app
  src/main.cpp
)

target_link_libraries(lab2_app
  PRIVATE
    lab2_ui
)
