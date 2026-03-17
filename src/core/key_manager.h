#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <cstdint>
#include <string>
#include <vector>

namespace Crypto
{
class KeyManager
{
  std::vector<uint8_t> current_key;

public:
  // заглушка для derive_key
  std::vector<uint8_t> derive_key(const std::string &password,
                                  const std::vector<uint8_t> &salt)
  {

    // Просто копируем пароль в вектор байт
    std::vector<uint8_t> key(password.begin(), password.end());

    // Добиваем до 32 байт нулями (для имитации 256-битного ключа)
    key.resize(32, 0);

    return key;
  }

  // заглушка для store_key
  bool store_key(const std::vector<uint8_t> &key,
                 const std::string &identifier = "default")
  {

    // Просто сохраняем в памяти
    current_key = key;

    return true; // Всегда успешно
  }

  // Пзаглушка для load_key
  std::vector<uint8_t> load_key(const std::string &identifier = "default")
  {
    return current_key; // Возвращаем последний сохраненный ключ
  }
};
} // namespace Crypto

#endif