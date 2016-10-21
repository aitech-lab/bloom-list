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
} filter_t;

KHASH_MAP_INIT_STR(key_filter_hm, filter_t*);   // char key -> filter
typedef khash_t(key_filter_hm) fl_t; 

// Енкодинг данных в блум-список
fl_t* fl_encode(char* filename, float error);
// Создаёт пустые фильтры без алокации данных, заполняет счетчики
fl_t* fl_stat(char* filename);
// Собирает статистику по сегментам 
void fl_stat_line(fl_t* fl, char* line);
// Заполняет фильтры
void fl_parse_line(fl_t* fl, char* line);
// Выделаем реальную память под блум фильтры
void fl_allocate(fl_t* fl, float error);
// Заполянем филтер данными
void fl_fill(fl_t* fl, char* filename);
// Сохраняем фильтр   
void fl_save(fl_t* fl, char* output);
// Загрузка фильтра
fl_t* fl_load(char* filename);
// Очистка
void fl_destroy(fl_t* fl);
// вывод статистики
void fl_print(fl_t* fl);
