cmake_minimum_required(VERSION 3.5.0)
project(waydroid-files C CXX)

# Automatically create moc files
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")

find_package(Qt5Core REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickControls2 REQUIRED)
find_package(Qt5Widgets REQUIRED)

find_package(QCoro5 COMPONENTS Core Network DBus Qml QUIET)
if(NOT QCoro5_FOUND)
    message(STATUS "QCoro5 not found, building it as a submodule")
    set(QCORO_BUILD_EXAMPLES OFF)
    set(QCORO_BUILD_TESTING OFF)
    set(BUILD_SHARED_LIBS OFF)
    set(USE_QT_VERSION 5)
    set(QCORO_WITH_QTDBUS ON)
    set(QCORO_WITH_QTNETWORK ON)
    set(QCORO_WITH_QTWEBSOCKETS OFF)
    add_subdirectory(libs/qcoro)
    target_include_directories(QCoro5Core INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libs/qcoro/core/QCoro>)
endif()
qcoro_enable_coroutines()

execute_process(
    COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
    OUTPUT_VARIABLE ARCH_TRIPLET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(QT_IMPORTS_DIR "lib/${ARCH_TRIPLET}")

set(PROJECT_NAME "waydroid-files")
set(ALTERNATIVE_PROJECT_NAME "adb-file-explorer")
set(FULL_PROJECT_NAME "waydroid-files.jcm")
set(DATA_DIR /)
set(DESKTOP_FILE_NAME ${PROJECT_NAME}.desktop)
set(ALTERNATIVE_DESKTOP_FILE_NAME ${ALTERNATIVE_PROJECT_NAME}.desktop)
set(CONTENT_HUB_JSON ${PROJECT_NAME}-contenthub.json)

# This command figures out the minimum SDK framework for use in the manifest
# file via the environment variable provided by Clickable or sets a default value otherwise.
if(DEFINED ENV{SDK_FRAMEWORK})
    set(CLICK_FRAMEWORK "$ENV{SDK_FRAMEWORK}")
else()
    set(CLICK_FRAMEWORK "ubuntu-touch-24.04-1.x")
endif()

# This figures out the target architecture for use in the manifest file.
if(DEFINED ENV{ARCH})
    set(CLICK_ARCH "$ENV{ARCH}")
else()
    execute_process(
        COMMAND dpkg-architecture -qDEB_HOST_ARCH
        OUTPUT_VARIABLE CLICK_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

# Sets BUILD_VERSION: Either tag of the current git HEAD or devel build version with git hash
execute_process(
  COMMAND git describe --tags --abbrev=0 --exact-match
  OUTPUT_VARIABLE BUILD_VERSION_RAW
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  )
if(NOT BUILD_VERSION_RAW)
  execute_process(
    COMMAND git describe --tags --abbrev=0
    OUTPUT_VARIABLE LAST_VERSION_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    )
  string(TIMESTAMP BUILD_VERSION_RAW "${LAST_VERSION_RAW}.%y%m%d%H%M%S" UTC)
endif(NOT BUILD_VERSION_RAW)
string(SUBSTRING ${BUILD_VERSION_RAW} 1 -1 BUILD_VERSION)
message(STATUS "Build version is: ${BUILD_VERSION}")


configure_file(manifest.json.in ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manifest.json DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CONTENT_HUB_JSON} DESTINATION ${DATA_DIR})
install(FILES ${PROJECT_NAME}.apparmor DESTINATION ${DATA_DIR})
install(FILES assets/logo.svg DESTINATION assets)
install(FILES assets/logo-snap.svg DESTINATION assets)

qt5_add_resources(QT_RESOURCES qml/qml.qrc)
qt5_add_resources(QT_RESOURCES assets/assets.qrc)
set(APP_SOURCES
    src/main.cpp
    src/patharrowbackground.cpp
    src/snaphelper.cpp
)
add_executable(${PROJECT_NAME} ${APP_SOURCES} ${QT_RESOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
target_link_libraries(${PROJECT_NAME} Qt5::Gui Qt5::Qml Qt5::Quick Qt5::QuickControls2 Qt5::Widgets)
target_link_libraries(${PROJECT_NAME} QCoro5::Core QCoro5::DBus QCoro5::Qml)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})

# Translations
file(GLOB_RECURSE I18N_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/po qml/*.qml qml/*.js)
list(APPEND I18N_SRC_FILES ${DESKTOP_FILE_NAME}.in.h ${ALTERNATIVE_DESKTOP_FILE_NAME}.in.h)

find_program(INTLTOOL_MERGE intltool-merge)
if(NOT INTLTOOL_MERGE)
    message(FATAL_ERROR "Could not find intltool-merge, please install the intltool package")
endif()
find_program(INTLTOOL_EXTRACT intltool-extract)
if(NOT INTLTOOL_EXTRACT)
    message(FATAL_ERROR "Could not find intltool-extract, please install the intltool package")
endif()

add_custom_target(${DESKTOP_FILE_NAME} ALL
    COMMENT "Merging translations into ${DESKTOP_FILE_NAME}..."
    COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${CMAKE_SOURCE_DIR}/${DESKTOP_FILE_NAME}.in ${DESKTOP_FILE_NAME}
    COMMAND sed -i 's/${PROJECT_NAME}-//g' ${CMAKE_CURRENT_BINARY_DIR}/${DESKTOP_FILE_NAME}
)
add_custom_target(${ALTERNATIVE_DESKTOP_FILE_NAME} ALL
    COMMENT "Merging translations into ${DESKTOP_FILE_NAME}..."
    COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_SOURCE_DIR}/po ${CMAKE_SOURCE_DIR}/${ALTERNATIVE_DESKTOP_FILE_NAME}.in ${ALTERNATIVE_DESKTOP_FILE_NAME}
    COMMAND sed -i 's/${PROJECT_NAME}-//g' ${CMAKE_CURRENT_BINARY_DIR}/${ALTERNATIVE_DESKTOP_FILE_NAME}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DESKTOP_FILE_NAME} DESTINATION ${DATA_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${ALTERNATIVE_DESKTOP_FILE_NAME} DESTINATION ${DATA_DIR})

add_subdirectory(po)
add_subdirectory(plugins)

# Make source files visible in qtcreator
file(GLOB_RECURSE PROJECT_SRC_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    qml/*.qml
    qml/*.js
    src/*
    *.json
    *.json.in
    *.apparmor
    *.desktop.in
)

add_custom_target(${PROJECT_NAME}_FILES ALL SOURCES ${PROJECT_SRC_FILES})
