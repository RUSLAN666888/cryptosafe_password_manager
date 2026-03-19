cmake_minimum_required(VERSION 3.10)
project(CryptoSafe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Название исполняемого файла
set(TARGET_NAME cryptosafe)

# ============================================================
# ПОИСК БИБЛИОТЕК
# ============================================================

# 1. Поиск wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# 2. Поиск SQLite3
find_package(SQLite3 REQUIRED)

# 3. Поиск nlohmann_json
find_package(nlohmann_json REQUIRED)

# 4. Поиск argon2 — заменяем find_package на ручной поиск
find_library(ARGON2_LIB
    NAMES argon2
    PATHS /usr/lib/x86_64-linux-gnu
    REQUIRED
)

find_path(ARGON2_INCLUDE
    NAMES argon2.h
    PATHS /usr/include
    REQUIRED
)

# 5. Поиск OpenSSL
find_package(OpenSSL REQUIRED)

# 6. Поиск zxcvbn
find_library(ZXCVB_LIB
    NAMES zxcvbn
    PATHS /usr/lib /usr/local/lib
    REQUIRED
)

find_path(ZXCVB_INCLUDE
    NAMES zxcvbn.h
    PATHS /usr/include /usr/local/include
    REQUIRED
)

find_library(LIBSODIUM_LIB
    NAMES sodium
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
    REQUIRED
)

find_path(LIBSODIUM_INCLUDE
    NAMES sodium.h
    PATHS /usr/include /usr/local/include
    REQUIRED
)

# ============================================================
# ВСЕ ИСХОДНЫЕ ФАЙЛЫ
# ============================================================

set(SOURCES
    # Core
    src/core/audit_logger.h
    src/core/config_handler.h
    src/core/crypto/abstract.h
    src/core/crypto/AES256Placeholder.h
    src/core/events.h
    src/core/key_manager.h
    src/core/crypto/authentication.h

    # Database
    src/database/DB_helper/db_helper.cpp

    # GUI Main
    src/gui/MainWindow.cpp

    # GUI Dialogs
    src/gui/dialogs/first_run_wizard/FirstRunWizard.cpp
    src/gui/dialogs/settings_dialog/SettingsDialog.cpp
    src/gui/dialogs/login_dialog/LoginDialog.cpp

    # GUI Widgets
    src/gui/widgets/audit_log_viewer/AuditLogViewer.cpp
    src/gui/widgets/password_entry/PasswordEntry.cpp
    src/gui/widgets/secure_table/SecureTable.cpp

    # Main entry point
    main.cpp
)

# ============================================================
# СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА
# ============================================================

add_executable(${TARGET_NAME} ${SOURCES})

# ============================================================
# ПОДКЛЮЧЕНИЕ ЗАГОЛОВОЧНЫХ ФАЙЛОВ
# ============================================================

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database/DB_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/first_run_wizard
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/settings_dialog
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/login_dialog
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/audit_log_viewer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/password_entry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/secure_table
    ${wxWidgets_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    ${ARGON2_INCLUDE}
    ${ZXCVB_INCLUDE}
    ${LIBSODIUM_INCLUDE}
)

# ============================================================
# ЛИНКОВКА БИБЛИОТЕК
# ============================================================

target_link_libraries(${TARGET_NAME} PRIVATE
    ${wxWidgets_LIBRARIES}
    ${SQLite3_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${ARGON2_LIB}  
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ZXCVB_LIB}
    ${LIBSODIUM_LIB}
)



message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Target: ${TARGET_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "wxWidgets found: ${wxWidgets_FOUND}")
message(STATUS "SQLite3 found: ${SQLite3_FOUND}")
message(STATUS "nlohmann_json found: ${nlohmann_json_FOUND}")