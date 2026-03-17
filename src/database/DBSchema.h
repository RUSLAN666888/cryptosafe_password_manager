#ifndef DATABASE_SCHEMA_H
#define DATABASE_SCHEMA_H

#include <string>

const int CURRENT_VERSION = 1;

const std::string CREATE_TABLES = R"(
            -- Версия базы данных
            PRAGMA user_version = 1;

            -- Включение поддержки внешних ключей
            PRAGMA foreign_keys = ON;

            -- =====================================================
            -- Таблица: vault_entries (Основное хранилище паролей)
            -- =====================================================

            CREATE TABLE IF NOT EXISTS vault_entries (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                title TEXT NOT NULL,
                username TEXT NOT NULL,
                encrypted_password BLOB NOT NULL,  -- Данные шифруются до вставки
                url TEXT,
                notes TEXT,
                tags TEXT,  -- JSON массив тегов для будущей фильтрации
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );

            -- Индексы для быстрого поиска
            CREATE INDEX IF NOT EXISTS idx_vault_title ON vault_entries(title);
            CREATE INDEX IF NOT EXISTS idx_vault_username ON vault_entries(username);
            CREATE INDEX IF NOT EXISTS idx_vault_created ON vault_entries(created_at);
            CREATE INDEX IF NOT EXISTS idx_vault_tags ON vault_entries(tags);

            -- Триггер для автоматического обновления updated_at
            CREATE TRIGGER IF NOT EXISTS update_vault_entries_timestamp 
            AFTER UPDATE ON vault_entries
            BEGIN
                UPDATE vault_entries SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id;
            END;

            -- =====================================================
            -- Таблица: audit_log (Журнал действий)
            -- =====================================================
            CREATE TABLE IF NOT EXISTS audit_log (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                action TEXT NOT NULL,  -- EntryAdded, EntryUpdated, EntryDeleted, UserLoggedIn, UserLoggedOut, ClipboardCopied, ClipboardCleared
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                entry_id INTEGER,      -- Ссылка на запись (может быть NULL для действий без записи)
                details TEXT,          -- Дополнительная информация в JSON формате
                signature BLOB,        -- Место для цифровой подписи (Sprint 5)
                FOREIGN KEY (entry_id) REFERENCES vault_entries(id) ON DELETE SET NULL
            );

            -- Индексы для журнала
            CREATE INDEX IF NOT EXISTS idx_audit_timestamp ON audit_log(timestamp);
            CREATE INDEX IF NOT EXISTS idx_audit_action ON audit_log(action);
            CREATE INDEX IF NOT EXISTS idx_audit_entry_id ON audit_log(entry_id);

            -- =====================================================
            -- Таблица: settings (Настройки приложения)
            -- =====================================================
            CREATE TABLE IF NOT EXISTS settings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                setting_key TEXT UNIQUE NOT NULL,
                setting_value TEXT,
                encrypted BOOLEAN DEFAULT 0,  -- Флаг, что значение зашифровано
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );


            -- Триггер для обновления timestamp в settings
            CREATE TRIGGER IF NOT EXISTS update_settings_timestamp 
            AFTER UPDATE ON settings
            BEGIN
                UPDATE settings SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id;
            END;

            -- =====================================================
            -- Таблица: key_store (Хранение ключей и параметров)
            -- =====================================================
            CREATE TABLE IF NOT EXISTS key_store (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key_type TEXT NOT NULL,  -- master_key, derivation_salt, etc.
                salt BLOB NOT NULL,      -- Соль для выведения ключей
                hash BLOB,               -- Хеш для проверки
                params TEXT,             -- JSON параметры (итерации, алгоритм)
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );

            -- Индекс для поиска по типу ключа
            CREATE INDEX IF NOT EXISTS idx_key_type ON key_store(key_type);

        )";

#endif