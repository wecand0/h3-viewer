cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

file(READ VERSION VERSION LIMIT_COUNT 1)
string(REPLACE "\n" "" VERSION "${VERSION}")
string(REGEX REPLACE "-.*$" "" VERSION "${VERSION}")

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME} VERSION ${VERSION} LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#add_definitions(-DQT_NO_DEBUG_OUTPUT -DNDEBUG)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-Wl,--strip-all")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
#        -O2 \
#        -s \
#        -Wall \
#        -Wextra \
#        -pedantic \
#        -fdiagnostics-color=always \
#        -Wunreachable-code \
#        -Wno-attributes")


####################

# Найти Qt компоненты
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 QuickWidgets Concurrent Network REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_resources(RESOURCES resources.qrc)

find_package(QMapLibre COMPONENTS Location REQUIRED)

set(THIRDPARTY_DIR thirdparty/) # Более описательное имя
add_subdirectory(${THIRDPARTY_DIR})

include_directories(src/models)

# Исходные файлы (.cpp)
set(SRC
        src/main.cpp
        src/mainwindow.cpp

        src/application.cpp
        src/application.h
        src/mainwindow.h

        src/mapProvider.h
        src/mapProvider.cpp
        src/h3model.cpp
        src/h3model.h
        src/h3datamanager.cpp
        src/h3datamanager.h
)

qt_add_executable(${PROJECT_NAME} ${SRC})

qt_policy(SET QTP0004 NEW)

# Настройка QML ресурсов
qt_add_qml_module(
        ${PROJECT_NAME}
        URI H3VIEWER
        VERSION 1.0
        RESOURCE_PREFIX "/"
        QML_FILES
        ui/main.qml
        ui/H3map.qml

        RESOURCES
        ${PROJECT_RESOURCES}
        resources.qrc

        data/style.json
)

set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/H3VIEWER" ${QML_IMPORT_PATH})

# Настройка свойств целевого объекта
set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

# Библиотеки для линковки
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::QuickControls2
        Qt6::Quick
        Qt6::QuickWidgets
        Qt6::Concurrent
        Qt6::Network
        Qt6::Positioning
        QMapLibre::Location
)


# Общие библиотеки для всех платформ
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        spdlog
        h3
        pthread
)

qmaplibre_location_setup_plugins(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
        ${THIRDPARTY_DIR}h3/src/h3lib/include
        ${THIRDPARTY_DIR}spdlog
)

if (${DEBUG_MODE})
    target_compile_definitions(${PROJECT_NAME} PUBLIC QT_DEBUG_NO_OUTPUT)
endif ()

target_compile_definitions(${PROJECT_NAME} PUBLIC LINUX_PLATFORM_DEFINE)

# Функция копирования ресурсов рекурсивно
function(copy_recursive SOURCE_PATH DESTINATION_PATH REGEX)
    file(GLOB_RECURSE
            FILES
            ${SOURCE_PATH}
            "${SOURCE_PATH}/${REGEX}")
    foreach (file ${FILES})
        # To recreate the same folder structure we first need to read the base folder
        file(RELATIVE_PATH RELATIVE_FILE_PATH ${SOURCE_PATH} ${file})
        get_filename_component(FOLDER ${RELATIVE_FILE_PATH} DIRECTORY ${SOURCE_PATH})
        file(MAKE_DIRECTORY ${DESTINATION_PATH}/${FOLDER})
        configure_file(${file} "${DESTINATION_PATH}/${RELATIVE_FILE_PATH}" COPYONLY)
    endforeach ()
endfunction()

# Копируем глифы только для Linux, т.к. для Android они будут включены в APK
copy_recursive(${CMAKE_SOURCE_DIR}/glyphs/ ${CMAKE_CURRENT_BINARY_DIR}/glyphs *)