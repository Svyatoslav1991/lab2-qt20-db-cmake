# Тесты пока отсутствуют, но подготовим зависимости
set(CMAKE_AUTOMOC ON)  # пригодится, когда появятся QObject/QTest ([cmake.org] https://cmake.org/cmake/help/latest/variable/CMAKE_AUTOMOC.html)

find_package(Qt5 REQUIRED COMPONENTS Core Sql Test)

# Пока нет тестовых исходников — создаём интерфейсный target "tests"
# (чтобы директория могла быть подключена без ошибок)
add_library(lab2_tests INTERFACE)
target_link_libraries(lab2_tests
  INTERFACE
    lab2_core
    Qt5::Core
    Qt5::Sql
    Qt5::Test
)
