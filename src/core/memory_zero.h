#ifndef SECURE_MEMORY_H
#define SECURE_MEMORY_H

#include <cstddef>
#include <cstdint>

/*
    Функция для безопасной работы с памятью
    Предотвращает оптимизацию компилятора и утечки sensitive данных
*/

void memory_zero(void *ptr, size_t size)
{
  volatile uint8_t *p = reinterpret_cast<volatile uint8_t *>(ptr);
  for (size_t i = 0; i < size; ++i)
  {
    p[i] = 0;
  }
}

#endif