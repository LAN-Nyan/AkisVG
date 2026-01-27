cmake_minimum_required(VERSION 3.16)
project(AkisVG VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages - Added Multimedia here
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Svg
    Multimedia
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -O3)
endif()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
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
    src/panels/layerpanel.cpp
    src/panels/colorpicker.cpp
    src/panels/assetlibrary.cpp
    src/tools/erasertool.cpp
    src/tools/shapetool.cpp
    src/tools/texttool.cpp
    src/tools/brushtool.cpp
    src/tools/linetool.cpp
    src/canvas/objects/shapeobject.cpp
    src/canvas/objects/textobject.cpp
    src/core/commands.cpp
    src/canvas/objects/imageobject.cpp
    src/tools/filltool.cpp
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
    src/canvas/objects/imageobject.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS}
    resources.qrc
    src/panels/settingspanel.h src/panels/settingspanel.cpp
    src/tools/filltool.h src/tools/filltool.cpp
    src/panels/toolsettingspanel.h src/panels/toolsettingspanel.cpp
    src/panels/toolbutton.h src/panels/toolbutton.cpp
    src/config.h
    src/tools/blendtool.h src/tools/blendtool.cpp)

# Link Qt libraries - Added Qt6::Multimedia here
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
    Qt6::Multimedia
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
