#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bloom_list.h"
#include "murmur3.h"

  ///////////
 // BLOOM //
///////////

// TODO
// Huge filters over 4e9 bits needs uint64_t hashes!
#define bit_set(v,n) ((v)[(n) >> 3] |= (0x1 << (0x7 - ((n) & 0x7))))
#define bit_get(v,n) ((v)[(n) >> 3] &  (0x1 << (0x7 - ((n) & 0x7))))
#define bit_clr(v,n) ((v)[(n) >> 3] &=~(0x1 << (0x7 - ((n) & 0x7))))

#define LN2 0.6931471805599453
#define LN4 0.4804530139182014

#define MAX_HASHES 32

// установить биты фильтра
static void 
bloom_set(bloom_t* f, 
           char* key, 
           uint32_t* hashes) {

    if(f == NULL) return;

    for (uint32_t i = 0; i < f->hash_num; i++) {
        uint32_t h = hashes[i];
        h %= f->bit_size;
        bit_set(f->data, h);
    }
    f->entries++;

}

// проверка битов фильтра
static uint8_t 
bloom_get(bloom_t* f, 
           const char* key, 
           uint32_t* hashes) {

    for (uint32_t i = 0; i < f->hash_num; i++) {
        uint32_t h = hashes[i];
        h %= f->bit_size;
        if (!bit_get(f->data, h))
            return 0;
    }
    return 1;
}


// запись блум-фильтра
static void 
bloom_save(bloom_t* f, FILE* fd) {
    // FILE * fd = fopen(filename, "wb");
    if(f == NULL || fd == NULL) return;
    // save struct
    fwrite(f, sizeof(bloom_t), 1, fd);                  // write struct
    fwrite(f->data, sizeof(char), f->byte_size, fd); // write filter
    // fclose(fd);
}

// чтение блум-фильтра
static bloom_t* 
bloom_load(FILE* fd) {
    if(fd == NULL) return NULL;
    // load structure
    size_t ln = sizeof(bloom_t);
    bloom_t* f = calloc(1, ln);
    fread(f, ln, 1, fd);
    // load data
    ln = f->byte_size;
    f->data = (uint8_t *) calloc(ln, sizeof(uint8_t));
    fread(f->data, sizeof(uint8_t), ln, fd);
    return f;
}

  //////////
 // LIST //
//////////

// Создаёт пустые фильтры без алокации данных, заполняет счетчики
static bl_t* bl_stat(char* filename);
// Собирает статистику по сегментам 
static void bl_stat_line(bl_t* fl, char* line);
// Заполняет фильтры
static void bl_parse_line(bl_t* fl, char* line);
// Выделаем реальную память под блум фильтры
static void bl_allocate(bl_t* fl, float error);
// Заполянем филтер данными
static void bl_fill(bl_t* fl, char* filename);

// Возращает hash-map фильтров
bl_t*
bl_encode(char* filename, float error) {
    bl_t* fl = bl_stat(filename);
    if(fl == NULL) return NULL;
    bl_allocate(fl, error);
    bl_fill(fl, filename);
    return fl;    
}

// Восстанавление ключей
void
bl_decode(bl_t* fl, char* key, char* results) {
    
    uint32_t hashes[MAX_HASHES];
    for(uint32_t i=0; i<MAX_HASHES; ++i) murmur3_hash32(key, strlen(key), i, &hashes[i]);

    for (khiter_t ki=kh_begin(fl); ki!=kh_end(fl); ++ki) {
        if (kh_exist(fl, ki)) {
            char *key = (char*) kh_key(fl, ki);
            bloom_t* f = kh_value(fl, ki);
            // put filter to results
            if(bloom_get(f, key, hashes)) printf("%s\n", key);
        }
    }
}

// Print statistic
void 
bl_print(bl_t* fl) {
    // segment counters
    for (khiter_t ki=kh_begin(fl); ki!=kh_end(fl); ++ki) {
        if (kh_exist(fl, ki)) {
            char *key = (char*) kh_key(fl, ki);
            bloom_t* f = kh_value(fl, ki);
            printf("%s %lu\n", key, f->size);
        }
    }
}

// Сохраняем фильтр   
void
bl_save(bl_t* fl, 
        char* output) {
    
    FILE* fd = fopen(output, "wb");
    
    // prefix
    char* prefix  = "BL";
    fwrite(prefix, 2, 1, fd);
    
    // list size
    size_t ls = kh_size(fl);
    fwrite(&ls, sizeof(size_t), 1, fd);

    // segment counters
    for (khiter_t ki=kh_begin(fl); ki!=kh_end(fl); ++ki) {
        if (kh_exist(fl, ki)) {
            char *key = (char*) kh_key(fl, ki);
            bloom_t* f = kh_value(fl, ki);
            // length of key
            size_t ks = strlen(key);
            fwrite(&ks, sizeof(size_t), 1, fd);
            // key itself
            fwrite(key, ks, 1, fd);
            // filter
            bloom_save(f, fd);
        }
    }
    fclose(fd);
}


// Загрузка фильтра
bl_t* 
bl_load(char* filename) {
    
    bl_t* fl = kh_init(key_bloom_hm);
    FILE* fd = fopen(filename, "rb");
    
    char prefix[2];
    fread(prefix, 2, 1, fd);

    // read list size
    size_t ls;
    fread(&ls, sizeof(size_t), 1, fd);
    
    char key[128]; // key buffer
    size_t ks;     // key size
    for(uint32_t i = 0; i<ls; i++) {
        fread(&ks, sizeof(size_t), 1, fd); // read size of key
        fread(key, ks, 1, fd);             // read key itself
        key[ks] = 0;                       // set zero terminator
        
        int ret;
        // индекс сегмента
        khiter_t ki = kh_put(key_bloom_hm, fl, strdup(key), &ret);
        //создаем новый фильтр
        kh_value(fl, ki) = bloom_load(fd);
    }

    fclose(fd);
    return fl;
}

// Очистка
void
bl_destroy(bl_t* fl) {
    // segment counters
    for (khiter_t ki=kh_begin(fl); ki!=kh_end(fl); ++ki) {
        if (kh_exist(fl, ki)) {
            bloom_t* f  = kh_value(fl, ki);
            // Очищаем фильтры
            free(f->data);
            // Очищаем структуры фильтров
            free(f);
            // Очищаем ключи
            free((char*) kh_key(fl, ki));
        }
    }
    // Удаляем список
    kh_destroy(key_bloom_hm, fl);
}


/// STATIC /// 


// Создаёт пустые фильтры без алокации данных, заполняет счетчики
static bl_t*
bl_stat(char* filename) {

    bl_t* fl = kh_init(key_bloom_hm);

    FILE*  fp;
    char*  line = NULL;
    size_t len = 0;
    size_t read;

    fp = fopen(filename, "r");
    if (fp == NULL) return NULL;
    while ((read = getline(&line, &len, fp)) != -1) {
        bl_stat_line(fl, line);
    }

    // cleanup
     fclose(fp);
    if (line) free(line);

    return fl;
}


// Выделаем реальную память под блум фильтры
static void
bl_allocate(bl_t* fl, float error) {
     // segment counters
     for (khiter_t ki=kh_begin(fl); ki!=kh_end(fl); ++ki) {
         if (kh_exist(fl, ki)) {
            bloom_t* f  = kh_value(fl, ki);
            f->error     = error;
            f->bit_size  = (uint64_t) ceil(-(float)f->size * log(error) / LN4);
            f->byte_size = (uint64_t) ceil((float)f->bit_size / 8.0);
            f->hash_num  = (uint8_t ) ((float)f->bit_size/(float)f->size * LN2);
            f->entries   = 0;
            f->data      = calloc(f->byte_size, sizeof(uint8_t));
         }
     }
}

// Заполянем филтер данными
static void 
bl_fill(bl_t* fl, 
        char* filename) {

    FILE*  fp;
    char*  line = NULL;
    size_t len = 0;
    size_t read;

    fp = fopen(filename, "r");
    while ((read = getline(&line, &len, fp)) != -1) {
        bl_parse_line(fl, line);
    }

    // cleanup
    fclose(fp);
    if (line) free(line);

}

// размечаем строку нулями
// base64 key [\t] spaced flags [\x20] slashed segments [\n\0]
//              0                   0                       0
// возращаем начало списка сегментов
static char*
bl_tokenize_line(char* line) {

    char *seg, *tab, *end;
    
    end = strchr(line, '\n');     // find end / new line
    if(end == NULL) return NULL;       
    *end = 0;                     // replace it fith terminator

    tab = strchr(line, '\t');     // find tab
    if(tab == NULL) return NULL;
    *tab = 0;                     // split key/data
    
    // get last space
    seg = strrchr(tab+1, ' ');   // find segments
    if(seg == NULL) return NULL;
    *seg = 0;                     // data to flags/segments
    seg++;

    return seg; 
}

// Сбор статистики по сегментам
static void 
bl_stat_line(bl_t* fl, 
             char* line) {

    char* seg =bl_tokenize_line(line);
    
    if(seg == NULL) return;

    // iterate through segments
    char *s, *sv;
    s = strtok_r(seg, "/", &sv);
    
    while (s!=NULL) {
        int ret;
        // индекс сегмента
        khiter_t ki = kh_get(key_bloom_hm, fl, s);
        if(ki == kh_end(fl)) { 
            // новый key id для ключа
            ki = kh_put(key_bloom_hm, fl, strdup(s), &ret);
            //создаем новый фильтр
            kh_value(fl, ki) = calloc(1, sizeof(bloom_t));
            kh_value(fl, ki)->size = 1;
        } else {
            kh_value(fl, ki)->size += 1;
        }

        // next token
        s = strtok_r(NULL, "/", &sv);
    }
}


// Индексация сегментов
static void 
bl_parse_line(bl_t* fl, 
              char* line) {

    char* seg =bl_tokenize_line(line);
    
    if(seg == NULL) return;

    // iterate through segments
    char *s, *sv;
    s = strtok_r(seg, "/", &sv);
    
    // precalculate hashes 
    // TODO get real number of hashes for first filter
    // 0.001% - 17; 0.0001 - 20; 0.00001 - 24
    uint32_t hashes[MAX_HASHES];
    for(uint32_t i=0; i<MAX_HASHES; ++i) murmur3_hash32(line, strlen(line), i, &hashes[i]);

    while (s!=NULL) {
        // индекс сегмента
        khiter_t ki = kh_get(key_bloom_hm, fl, s);
        if(ki != kh_end(fl)) { 
            bloom_t* f = kh_value(fl, ki);
            bloom_set(f, line, hashes);
        }

        // next token
        s = strtok_r(NULL, "/", &sv);
    }
}

// free keys created by strdup()
// void 
// _destroy_stat() {
//     // segment counters
//     for (khiter_t ki=kh_begin(fl); ki!=kh_end(fl); ++ki) {
//         if (kh_exist(fl, ki)) {
//             free((char*) kh_key(fl, ki));
//         }
//     }
//     // destroy structure
//     kh_destroy(key_bloom_hm, fl);
// }e
