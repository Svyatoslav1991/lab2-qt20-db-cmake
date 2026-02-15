# Автогенерация для Qt (понадобится, когда появятся файлы с Q_OBJECT / .ui / .qrc)
set(CMAKE_AUTOMOC ON)  # ([cmake.org] https://cmake.org/cmake/help/latest/variable/CMAKE_AUTOMOC.html)
set(CMAKE_AUTOUIC ON)  # ([cmake.org] https://cmake.org/cmake/help/latest/variable/CMAKE_AUTOUIC.html)
set(CMAKE_AUTORCC ON)  # ([cmake.org] https://cmake.org/cmake/help/latest/variable/CMAKE_AUTORCC.html)

# Qt5 компоненты: сейчас просто "готовим площадку"
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Sql)

# Публичные include-деревья подключим заранее (пусть пока пустые)
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

# Пока без исходников: делаем "пустой" интерфейсный target приложения,
# чтобы верхний CMake мог add_subdirectory(app) без ошибок.
add_library(lab2_app INTERFACE)
target_link_libraries(lab2_app
  INTERFACE
    lab2_core
    Qt5::Widgets
)
