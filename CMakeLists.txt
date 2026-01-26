cmake_minimum_required(VERSION 3.16)
project(udevme VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

set(CORE_SOURCES
    src/core/DeviceScanner.cpp
    src/core/DeviceScanner.h
    src/core/AppScanner.cpp
    src/core/AppScanner.h
    src/core/RuleModel.cpp
    src/core/RuleModel.h
    src/core/RuleGenerator.cpp
    src/core/RuleGenerator.h
    src/core/RuleParser.cpp
    src/core/RuleParser.h
    src/core/ConfigStore.cpp
    src/core/ConfigStore.h
    src/core/Types.h
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/AddRuleDialog.cpp
    src/ui/AddRuleDialog.h
    src/ui/LogWidget.cpp
    src/ui/LogWidget.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(udevme
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${RESOURCES}
)

target_include_directories(udevme PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(udevme PRIVATE Qt6::Widgets Qt6::Concurrent)

install(TARGETS udevme RUNTIME DESTINATION bin)
install(FILES resources/udevme_icon.png DESTINATION share/icons/hicolor/256x256/apps RENAME udevme.png)
install(FILES packaging/udevme.desktop DESTINATION share/applications)

# Tests
enable_testing()
add_subdirectory(tests)
