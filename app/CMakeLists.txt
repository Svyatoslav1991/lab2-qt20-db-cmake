# app/CMakeLists.txt

set(CMAKE_AUTOMOC ON)  # moc для Q_OBJECT
set(CMAKE_AUTOUIC ON)  # uic для .ui
set(CMAKE_AUTORCC ON)  # rcc для .qrc (если появится)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Sql)

# На будущее оставим "ядро" как interface (пока без cpp)
add_library(lab2_core INTERFACE)
target_include_directories(lab2_core
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lab2_core
  INTERFACE
    Qt5::Core
    Qt5::Sql
)

# Уже реальное приложение
add_executable(lab2_app
  src/main.cpp
  src/mainwindow.h
  src/mainwindow.cpp
  src/mainwindow.ui
)

target_link_libraries(lab2_app
  PRIVATE
    lab2_core
    Qt5::Widgets
    Qt5::Sql
)
