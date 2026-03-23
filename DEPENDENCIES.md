# CryptoSafe Manager - Dependencies

## Required Libraries

| Library | Minimum Version | Purpose | Platforms |
|---------|----------------|---------|-----------|
| **Qt5** | 5.15.0 | GUI Framework | Windows, Linux, macOS |
| **SQLite3** | 3.35.0 | Embedded Database | Windows, Linux, macOS |
| **Argon2** | 20190702 | Password Hashing | Windows, Linux, macOS |
| **OpenSSL** | 1.1.1 | Cryptography (PBKDF2) | Windows, Linux, macOS |
| **libsodium** | 1.0.18 | Random Generation | Windows, Linux, macOS |
| **zxcvbn-c** | 2.5 | Password Strength | Windows, Linux, macOS |
| **nlohmann-json** | 3.11.0 | JSON Parsing | Windows, Linux, macOS |

## Development Tools

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| **CMake** | 3.16 | Build System |
| **C++ Compiler** | C++17 | Language Standard |
| **Git** | 2.25 | Version Control |

## Optional (for Testing)

| Library | Minimum Version | Purpose |
|---------|----------------|---------|
| **Google Test** | 1.10.0 | Unit Testing Framework |

## Package Manager References

### vcpkg
```bash
vcpkg install qt5 sqlite3 argon2 openssl libsodium zxcvbn-c nlohmann-json gtest
