cmake_minimum_required(VERSION 3.21)

project(QuantumChaos VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ---------------------------------------------------------------- the backend (no Qt)
# Everything under backend/ is plain C++20. It is built as its own library and does not link to Qt,
# so a Qt include in there fails to compile.
file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/backend/*.cpp")

# Monocypher is compiled in here and only backend/common/crypto.cpp includes it.
add_library(notesengine STATIC ${BACKEND_SOURCES} dependencies/monocypher/monocypher.c)
target_include_directories(notesengine PUBLIC "${CMAKE_CURRENT_LIST_DIR}/backend")
target_include_directories(notesengine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/dependencies/monocypher")
target_compile_options(notesengine PRIVATE -Wall -Wextra)

# The key every note is encrypted with, given at configure time so it is never in the source:
#     cmake -DQUANTUMCHAOS_NOTE_KEY=$(head -c 32 /dev/urandom | xxd -p -c 64) ...
# Keep a copy of it: notes written with one key can't be read with another.
set(QUANTUMCHAOS_NOTE_KEY "" CACHE STRING "Note encryption key: 64 hex characters")
string(LENGTH "${QUANTUMCHAOS_NOTE_KEY}" NOTE_KEY_LENGTH)
if(NOT NOTE_KEY_LENGTH EQUAL 64 OR NOT QUANTUMCHAOS_NOTE_KEY MATCHES "^[0-9a-fA-F]+$")
    message(FATAL_ERROR "QUANTUMCHAOS_NOTE_KEY must be 64 hex characters. Make one with\n"
                        "    head -c 32 /dev/urandom | xxd -p -c 64\n"
                        "keep a copy, and pass it as -DQUANTUMCHAOS_NOTE_KEY=<key>.")
endif()

# The sync server's address ("https://host") and its token, also given at configure time
# (-DQUANTUMCHAOS_SYNC_SERVER=... -DQUANTUMCHAOS_SYNC_TOKEN=...). Without them the app runs with sync off.
set(QUANTUMCHAOS_SYNC_SERVER "" CACHE STRING "Sync server address")
set(QUANTUMCHAOS_SYNC_TOKEN "" CACHE STRING "Sync server token")

# ---------------------------------------------------------------- the app
find_package(Qt6 REQUIRED COMPONENTS Quick Network)
qt_standard_project_setup(REQUIRES 6.10)

# wrapper/  : QObject classes that connect the backend to QML (any subfolders)
# frontend/ : the QML files (any subfolders). File names must start with a capital letter and be
#             unique across all subfolders.
file(GLOB_RECURSE WRAPPER_SOURCES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/wrapper/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/wrapper/*.h")

file(GLOB_RECURSE FRONTEND_QML CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/frontend/*.qml")

set_source_files_properties(frontend/common/Theme.qml frontend/common/Icons.qml
                            PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(appQuantumChaos
    main.cpp
)

qt_add_qml_module(appQuantumChaos
    URI QuantumChaos
    QML_FILES
        Main.qml
        ${FRONTEND_QML}
    SOURCES
        ${WRAPPER_SOURCES}
)

# Qt's generated registration code includes the wrapper headers by bare file name,
# so every folder under wrapper/ has to be on the include path.
set(WRAPPER_DIRS "")
foreach(f ${WRAPPER_SOURCES})
    get_filename_component(d "${CMAKE_CURRENT_LIST_DIR}/${f}" DIRECTORY)
    list(APPEND WRAPPER_DIRS "${d}")
endforeach()
list(REMOVE_DUPLICATES WRAPPER_DIRS)

target_include_directories(appQuantumChaos PRIVATE "${CMAKE_CURRENT_LIST_DIR}" ${WRAPPER_DIRS})
target_compile_definitions(appQuantumChaos PRIVATE QUANTUMCHAOS_VERSION="${PROJECT_VERSION}"
                                                   QUANTUMCHAOS_NOTE_KEY="${QUANTUMCHAOS_NOTE_KEY}"
                                                   QUANTUMCHAOS_SYNC_SERVER="${QUANTUMCHAOS_SYNC_SERVER}"
                                                   QUANTUMCHAOS_SYNC_TOKEN="${QUANTUMCHAOS_SYNC_TOKEN}")
target_compile_options(appQuantumChaos PRIVATE -Wall -Wextra)
target_link_libraries(appQuantumChaos PRIVATE Qt6::Quick Qt6::Network notesengine)

# The window icon, from the app's resources (":/icons/icon.svg").
qt_add_resources(appQuantumChaos app_icon PREFIX "/icons" BASE "${CMAKE_CURRENT_LIST_DIR}/resources/icons"
                 FILES resources/icons/icon.svg)

# Qt's HTTPS on Android needs OpenSSL inside the APK (built for arm64 only).
if(ANDROID)
    set(OPENSSL_ANDROID_DIR "${CMAKE_CURRENT_LIST_DIR}/dependencies/openssl/${ANDROID_ABI}")
    set_property(TARGET appQuantumChaos APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
        "${OPENSSL_ANDROID_DIR}/libcrypto_3.so" "${OPENSSL_ANDROID_DIR}/libssl_3.so")
    # packaging/android/ holds the manifest (the app's permissions) and the launcher icon.
    set_target_properties(appQuantumChaos PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/packaging/android"
        QT_ANDROID_PACKAGE_NAME io.github.chandandasgit.quantumchaos
        QT_ANDROID_APP_NAME QuantumChaos
        QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE 1)
endif()

# ---------------------------------------------------------------- the tests
# tests/abc/abc_test.cpp is found by itself, and ctest gets one entry named abc. Desktop only.
if(NOT ANDROID)
enable_testing()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/tests/*_test.cpp")

add_executable(backendtests tests/backendtests.cpp ${TEST_SOURCES})
target_include_directories(backendtests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/tests")
target_compile_options(backendtests PRIVATE -Wall -Wextra)
target_link_libraries(backendtests PRIVATE notesengine)

foreach(file ${TEST_SOURCES})
    get_filename_component(stem "${file}" NAME_WE)
    string(REGEX REPLACE "_test$" "" suite "${stem}")
    add_test(NAME ${suite} COMMAND backendtests ${suite})
endforeach()
endif()
