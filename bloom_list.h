#pragma once

#include <stdint.h>
#include <stddef.h>

#include "khash.h"

typedef struct {
    uint64_t size;       // Ко-во элементов для записи в фильтр
    float error;         // Ожидаемая ложно-позитивная ошибка

    uint64_t bit_size;   // Размер фильтра в битах
    uint64_t byte_size;  // Размер фильтра в байтах
    uint8_t  hash_num;   // Число хешей на запись
    uint64_t entries;    // Ко-цо записей
    uint8_t* data;       // Фильтр
} bloom_t;

KHASH_MAP_INIT_STR(key_bloom_hm, bloom_t*);   // char key -> filter
typedef khash_t(key_bloom_hm) bl_t; 

// Енкодинг данных в блум-список
bl_t* bl_encode(char* filename, float error);
// Восстанавление ключей
void bl_decode(bl_t* fl, char* key, char* results);
// Сохраняем фильтр   
void bl_save(bl_t* fl, char* output);
// Загрузка фильтра
bl_t* bl_load(char* filename);
// Очистка
void bl_destroy(bl_t* fl);
// вывод статистики
void bl_print(bl_t* fl);
