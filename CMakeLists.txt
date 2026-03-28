cmake_minimum_required(VERSION 3.10)
project(CryptoSafe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # Автоматическая обработка Qt MOC
set(CMAKE_AUTOUIC ON)  # Автоматическая обработка Qt UI файлов
set(CMAKE_AUTORCC ON)  # Автоматическая обработка Qt ресурсов

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Название исполняемого файла
set(TARGET_NAME cryptosafe)

# ============================================================
# ПОИСК БИБЛИОТЕК
# ============================================================

# 1. Поиск Qt5
find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Core)

# 2. Поиск SQLite3
find_package(SQLite3 REQUIRED)

# 3. Поиск nlohmann_json
find_package(nlohmann_json REQUIRED)

# 4. Поиск argon2 — заменяем find_package на ручной поиск
find_library(ARGON2_LIB
    NAMES argon2
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
)

find_path(ARGON2_INCLUDE
    NAMES argon2.h
    PATHS /usr/include /usr/local/include
)

# 5. Поиск OpenSSL
find_package(OpenSSL REQUIRED)

# 6. Поиск zxcvbn
find_library(ZXCVB_LIB
    NAMES zxcvbn
    PATHS /usr/lib /usr/local/lib
)

find_path(ZXCVB_INCLUDE
    NAMES zxcvbn.h
    PATHS /usr/include /usr/local/include
)

# 7. Поиск libsodium
find_library(LIBSODIUM_LIB
    NAMES sodium
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

find_path(LIBSODIUM_INCLUDE
    NAMES sodium.h
    PATHS /usr/include /usr/local/include
)

# ============================================================
# ВСЕ ИСХОДНЫЕ ФАЙЛЫ
# ============================================================

set(SOURCES
    # Core
    src/core/audit_logger.h
    src/core/config_handler.h
    src/core/crypto/abstract.h
    src/core/crypto/AES256.h
    src/core/events.h
    src/core/key_manager.h
    src/core/crypto/authentication.h

    # Vault
    src/core/vault/VaultManager.cpp

    # Database
    src/database/DB_helper/db_helper.cpp

    # GUI Main
    src/gui/MainWindow.cpp

    # GUI Dialogs
    src/gui/dialogs/first_run_wizard/FirstRunWizard.cpp
    src/gui/dialogs/settings_dialog/SettingsDialog.cpp
    src/gui/dialogs/login_dialog/LoginDialog.cpp
    src/gui/dialogs/password_change/password_change.cpp
    src/gui/dialogs/entry_dialog/entry_dialog.cpp

    # GUI Widgets
    src/gui/widgets/audit_log_viewer/AuditLogViewer.cpp
    src/gui/widgets/password_entry/PasswordEntry.cpp
    src/gui/widgets/secure_table/SecureTable.cpp
    src/gui/widgets/secure_table/VaultTableModel.cpp
    src/gui/widgets/secure_table/SearchProxyModel.cpp

    # Main entry point
    main.cpp
)

# Qt заголовочные файлы для MOC
set(HEADERS
    src/gui/MainWindow.h
    src/gui/dialogs/first_run_wizard/FirstRunWizard.h
    src/gui/dialogs/settings_dialog/SettingsDialog.h
    src/gui/dialogs/login_dialog/LoginDialog.h
    src/gui/dialogs/entry_dialog/entry_dialog.h
    src/gui/dialogs/password_change/password_change.h
    src/gui/widgets/audit_log_viewer/AuditLogViewer.h
    src/gui/widgets/password_entry/PasswordEntry.h
    src/gui/widgets/secure_table/SecureTable.h
    src/gui/widgets/secure_table/VaultTableModel.h
    src/gui/widgets/secure_table/SearchProxyModel.h
)

# ============================================================
# СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА
# ============================================================

set(ALL_SOURCES ${SOURCES} ${HEADERS})

add_executable(${TARGET_NAME} ${ALL_SOURCES}
    src/gui/widgets/secure_table/PasswordDelegate.h
    src/gui/widgets/secure_table/PasswordDelegate.cpp)

# ============================================================
# ПОДКЛЮЧЕНИЕ ЗАГОЛОВОЧНЫХ ФАЙЛОВ
# ============================================================

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database/DB_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/first_run_wizard
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/settings_dialog
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/login_dialog
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/password_change
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/entry_dialog
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/audit_log_viewer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/password_entry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/secure_table
    ${SQLite3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    ${ARGON2_INCLUDE}
    ${ZXCVB_INCLUDE}
    ${LIBSODIUM_INCLUDE}
)

# Qt include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
)

# ============================================================
# ЛИНКОВКА БИБЛИОТЕК
# ============================================================

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Core
    ${SQLite3_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${ARGON2_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ZXCVB_LIB}
    ${LIBSODIUM_LIB}
)

# ============================================================
# КОМПИЛЯТОРНЫЕ ФЛАГИ
# ============================================================

target_compile_definitions(${TARGET_NAME} PRIVATE
    QT_CORE_LIB
    QT_WIDGETS_LIB
)

# ============================================================
# ВЫВОД ИНФОРМАЦИИ
# ============================================================

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Target: ${TARGET_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt5 found: ${Qt5Widgets_FOUND}")
message(STATUS "SQLite3 found: ${SQLite3_FOUND}")
message(STATUS "nlohmann_json found: ${nlohmann_json_FOUND}")
message(STATUS "OpenSSL found: ${OPENSSL_FOUND}")

# ============================================================
# ТЕСТЫ КРИПТОГРАФИИ
# ============================================================

# option(BUILD_TESTS "Build tests" ON)

# if(BUILD_TESTS)
#     enable_testing()

#     find_package(GTest REQUIRED)

#     add_executable(test_crypto
#         tests/test_crypto2.cpp
#     )

#     target_link_libraries(test_crypto PRIVATE
#         GTest::gtest
#         GTest::gtest_main
#         ${ARGON2_LIB}
#         ${LIBSODIUM_LIB}
#         OpenSSL::SSL
#         OpenSSL::Crypto
#         ${ZXCVB_LIB}
#     )

#     target_include_directories(test_crypto PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/src
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/core
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/core/crypto
#         ${ARGON2_INCLUDE}
#         ${LIBSODIUM_INCLUDE}
#     )

#     add_test(NAME CryptoTest COMMAND test_crypto2)
# endif()
