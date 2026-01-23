cmake_minimum_required(VERSION 3.16)
project(LuminaStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Svg
)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -O3)
endif()

# Source files (we'll add more as we build)
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/canvas/objects/shapeobject.cpp
    src/canvas/objects/textobject.cpp
    src/panels/layerpanel.cpp
    src/panels/colorpicker.cpp
    src/panels/assetlibrary.cpp
    src/tools/brushtool.cpp
    src/tools/erasertool.cpp
    src/tools/shapetool.cpp
    src/tools/texttool.cpp
    src/core/project.cpp
    src/core/layer.cpp
    src/core/frame.cpp
    src/canvas/vectorcanvas.cpp
    src/canvas/canvasview.cpp
    src/canvas/objects/vectorobject.cpp
    src/canvas/objects/pathobject.cpp
    src/tools/tool.cpp
    src/tools/penciltool.cpp
    src/tools/selecttool.cpp
    src/panels/toolbox.cpp
    src/timeline/timelinewidget.cpp
)

set(HEADERS
    src/mainwindow.h
    src/core/project.h
    src/core/layer.h
    src/core/frame.h
    src/canvas/vectorcanvas.h
    src/canvas/canvasview.h
    src/canvas/objects/vectorobject.h
    src/canvas/objects/pathobject.h
    src/tools/tool.h
    src/tools/penciltool.h
    src/tools/selecttool.h
    src/panels/toolbox.h
    src/timeline/timelinewidget.h
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
