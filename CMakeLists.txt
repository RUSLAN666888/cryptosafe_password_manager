cmake_minimum_required(VERSION 3.10)
project(CryptoSafe)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # Автоматическая обработка Qt MOC
set(CMAKE_AUTOUIC ON)  # Автоматическая обработка Qt UI файлов
set(CMAKE_AUTORCC ON)  # Автоматическая обработка Qt ресурсов

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Название исполняемого файла
set(TARGET_NAME cryptosafe)


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

find_package(GTest REQUIRED)


set(SOURCES
    src/core/audit/log_signer/log_signer.cpp
    src/core/audit/audit_logger/audit_logger.cpp
    src/core/audit/log_verifier/log_verifier.cpp
    src/core/vault/VaultManager.cpp
    src/database/DB_helper/db_helper.cpp
    src/gui/MainWindow.cpp
    src/gui/dialogs/first_run_wizard/FirstRunWizard.cpp
    src/gui/dialogs/settings_dialog/SettingsDialog.cpp
    src/gui/dialogs/login_dialog/LoginDialog.cpp
    src/gui/dialogs/password_change/password_change.cpp
    src/gui/dialogs/entry_dialog/entry_dialog.cpp
    src/gui/widgets/password_entry/PasswordEntry.cpp
    src/gui/widgets/secure_table/SecureTable.cpp
    src/gui/widgets/secure_table/VaultTableModel.cpp
    src/gui/widgets/secure_table/SearchProxyModel.cpp
    src/gui/widgets/secure_table/PasswordDelegate.cpp
    src/core/clipboard_service/clipboard_service.cpp
    src/gui/dialogs/audit_dialog/audit_log_model.cpp
    src/gui/dialogs/audit_dialog/audit_log_dialog.cpp
)

set(MAIN_SOURCES
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
    src/gui/widgets/password_entry/PasswordEntry.h
    src/gui/widgets/secure_table/SecureTable.h
    src/gui/widgets/secure_table/VaultTableModel.h
    src/gui/widgets/secure_table/SearchProxyModel.h
    src/gui/widgets/secure_table/PasswordDelegate.h
    src/core/clipboard_service/clipboard_service.h
    src/gui/dialogs/audit_dialog/audit_log_model.h
    src/gui/dialogs/audit_dialog/audit_log_dialog.h
)


# Создаем список файлов для библиотеки
set(LIBRARY_SOURCES ${SOURCES} ${HEADERS})

add_library(cryptosafe_lib STATIC ${LIBRARY_SOURCES})

target_include_directories(cryptosafe_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit/log_verifier
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit/audit_logger
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit/log_signer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database/DB_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/secure_table
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/audit_dialog
    ${SQLite3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    ${ARGON2_INCLUDE}
    ${ZXCVB_INCLUDE}
    ${LIBSODIUM_INCLUDE}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
)

add_executable(${TARGET_NAME} ${MAIN_SOURCES}
    src/core/audit/log_formatter/log_formatter.h
    src/core/audit/log_formatter/log_formatter.cpp)



target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Qt include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
)


target_link_libraries(${TARGET_NAME} PRIVATE
    cryptosafe_lib
    Qt5::Widgets
    Qt5::Core
    ${SQLite3_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${ARGON2_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ZXCVB_LIB}
    ${LIBSODIUM_LIB}
    X11
)



target_compile_definitions(${TARGET_NAME} PRIVATE
    QT_CORE_LIB
    QT_WIDGETS_LIB
)


add_executable(run_tests
    tests/test_log_verification.cpp
    # tests/test_key_rotation.cpp
    # tests/test_encryption.cpp
    # tests/test_crud.cpp
    # tests/test_database.cpp
    # tests/test_database_auth_data.cpp
    # tests/test_config.cpp
    # tests/test_events.cpp
)

target_link_libraries(run_tests PRIVATE
    cryptosafe_lib
    GTest::gtest
    GTest::gtest_main
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
target_include_directories(run_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database/DB_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/secure_table
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit/log_verifier
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit/audit_logger
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/audit/log_signer
    ${SQLite3_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)




