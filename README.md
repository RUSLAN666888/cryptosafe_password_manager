# CryptoSafe Manager

## О проекте

CryptoSafe Manager — это локальный менеджер паролей с открытым исходным кодом, разработанный на C++ с использованием Qt5. Основной упор сделан на криптографическую безопасность, защиту данных в памяти и совместимость с другими менеджерами паролей.

## Ключевые возможности

- Шифрование AES-256-GCM — каждая запись шифруется индивидуально с уникальным nonce
- Криптостойкое хеширование Argon2id — защита мастер-пароля от брутфорса
- PBKDF2-HMAC-SHA256 — вывод ключа шифрования (100 000 итераций)
- Ed25519 цифровые подписи — защита аудит-логов от подделки
- HKDF — разделение ключей для разных целей (шифрование / подпись)
- RSA-OAEP (2048 бит) — безопасный обмен ключами для шеринга записей
- Постоянное время сравнения (constant-time) — защита от timing-атак
- Безопасное зануление памяти (secure_zero) — очистка паролей и ключей после использования
- Безопасный буфер обмена — автоматическая очистка через заданный интервал
- Режим паники (Ctrl+Shift+Esc) — мгновенная блокировка хранилища, очистка буфера и зануление ключей
- Аудит-лог с хеш-цепочкой — криптографическая защита всех событий
- Импорт/экспорт — поддержка Bitwarden JSON, LastPass CSV, собственного зашифрованного формата

## Технологический стек

| Компонент | Технология |
|-----------|------------|
| Язык | C++20 |
| GUI | Qt5 |
| Криптография | OpenSSL 3.x / 1.1.1 |
| Хеширование пароля | libargon2 (Argon2id) |
| База данных | SQLite3 |
| Проверка сложности пароля | libzxcvbn |
| JSON | nlohmann/json |
| Тестирование | Google Test |
| Сборка | CMake |

## Структура проекта

src/core/crypto/                - AES-GCM, Argon2id, PBKDF2, Ed25519, RSA, HKDF
src/core/audit/                 - Логирование, подпись, верификация
src/core/vault/                 - VaultManager (CRUD операции)
src/core/clipboard_service/     - ClipboardService
src/core/import_export/         - Exporter / Importer
src/core/sharing/               - SharingService
src/gui/dialogs/                - Все диалоговые окна
src/gui/widgets/                - Переиспользуемые компоненты
src/database/                   - SQLite обёртка и схемы

## Установка и запуск (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt5-default libsqlite3-dev \
    libssl-dev libargon2-dev libzxcvbn-dev nlohmann-json3-dev

git clone https://github.com/RUSLAN666888/cryptosafe_password_manager.git
cd cryptosafe
mkdir build && cd build
cmake ..
make 
./cryptosafe
```
## Установка и запуск (Windows)

### 1. Установка MSYS2

Скачайте и установите MSYS2 с официального сайта: https://www.msys2.org/

После установки запустите **MSYS2 UCRT64** (из меню Пуск).

### 2. Установка компилятора и зависимостей

В терминале MSYS2 UCRT64 выполните команды:

```bash
pacman -Syu
pacman -Su
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-qt5 mingw-w64-ucrt-x86_64-sqlite3 \
    mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-argon2 \
    mingw-w64-ucrt-x86_64-zxcvbn-c mingw-w64-ucrt-x86_64-nlohmann-json
```

## Клонирование и сборка

git clone https://github.com/yourusername/cryptosafe.git
cd cryptosafe
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
