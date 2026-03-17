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

# 4. Поиск argon2
find_package(Argon2 REQUIRED)

# 5. Поиск OpenSSL
find_package(OpenSSL REQUIRED)

# 6. Поиск zxcvbn
find_package(zxcvbn REQUIRED)

# ============================================================
# ВСЕ ИСХОДНЫЕ ФАЙЛЫ
# ============================================================

set(SOURCES
    # Core
    src/core/audit_logger.h          # Заголовочные файлы тоже можно добавлять
    src/core/config_handler.h
    src/core/crypto/abstract.h
    src/core/crypto/AES256Placeholder.h
    src/core/events.h
    src/core/key_manager.h
    src/core/memory_zero.h
    
    # Database
    src/database/DB_helper/db_helper.cpp
    
    # GUI Main
    src/gui/MainWindow.cpp
    
    # GUI Dialogs
    src/gui/dialogs/first_run_wizard/FirstRunWizard.cpp
    src/gui/dialogs/settings_dialog/SettingsDialog.cpp
    
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/audit_log_viewer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/password_entry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/widgets/secure_table
    ${wxWidgets_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
)

# ============================================================
# ЛИНКОВКА БИБЛИОТЕК
# ============================================================

target_link_libraries(${TARGET_NAME} PRIVATE
    ${wxWidgets_LIBRARIES}
    ${SQLite3_LIBRARIES}
    nlohmann_json::nlohmann_json
    Argon2::Argon2
    OpenSSL::SSL
    OpenSSL::Crypto
    zxcvbn::zxcvbn
)



message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Target: ${TARGET_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "wxWidgets found: ${wxWidgets_FOUND}")
message(STATUS "SQLite3 found: ${SQLite3_FOUND}")
message(STATUS "nlohmann_json found: ${nlohmann_json_FOUND}")