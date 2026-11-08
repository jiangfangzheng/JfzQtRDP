cmake_minimum_required(VERSION 3.21)

project(JfzQtRDP VERSION 1.2.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets AxContainer)

# QWindowKit's installed static target references Qt's private modules. Qt does
# not create these targets as part of the normal Widgets package, so load them
# explicitly.
find_package(Qt6CorePrivate REQUIRED)
find_package(Qt6GuiPrivate REQUIRED)
find_package(Qt6WidgetsPrivate REQUIRED)

if(NOT DEFINED QWindowKit_DIR)
    set(_qwk_candidates
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/QWindowKit/lib/cmake/QWindowKit"
        "${CMAKE_CURRENT_SOURCE_DIR}/QWindowKit/lib/cmake/QWindowKit"
    )
    foreach(_candidate IN LISTS _qwk_candidates)
        if(EXISTS "${_candidate}/QWindowKitConfig.cmake")
            set(QWindowKit_DIR "${_candidate}" CACHE PATH "QWindowKit package directory" FORCE)
            break()
        endif()
    endforeach()
endif()

find_package(QWindowKit CONFIG REQUIRED COMPONENTS Widgets)

qt_add_executable(JfzQtRDP
    src/main.cpp
    src/appearance.cpp
    src/appearance.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/titlebar.cpp
    src/titlebar.h
    src/captionbutton.cpp
    src/captionbutton.h
    src/homepage.cpp
    src/homepage.h
    src/rdpsession.cpp
    src/rdpsession.h
    src/serverconfig.cpp
    src/serverconfig.h
)

set_target_properties(JfzQtRDP PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "JfzQtRDP"
)

target_compile_definitions(JfzQtRDP PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(JfzQtRDP PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::AxContainer
    QWindowKit::Widgets
)

if(MSVC)
    target_compile_options(JfzQtRDP PRIVATE /utf-8 /W3)
else()
    target_compile_options(JfzQtRDP PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
