find_package(Qt5 REQUIRED COMPONENTS Test Core Widgets Sql)

function(add_qt_test target_name)
    add_executable(${target_name} ${ARGN})

    set_target_properties(${target_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
        WIN32_EXECUTABLE FALSE
    )

    target_link_libraries(${target_name}
        PRIVATE
            lab2_core
            lab2_ui
            Qt5::Test
            Qt5::Core
            Qt5::Widgets
            Qt5::Sql
    )

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/app/include
            ${CMAKE_SOURCE_DIR}/app/src
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

add_qt_test(test_smoke
    test_smoke.cpp
)

add_qt_test(test_mydelegate
    test_mydelegate.cpp
)

add_qt_test(test_mainwindow
    test_mainwindow.cpp
)


