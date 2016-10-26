/*

1) Акумулирует статистику по входным данным
2) По статистике строит список блум фильтров
3) Сохраняет фильтры

*/

#include <stdio.h>

#include "bloom_list.h"

int usage();

void query(const bl_t* bl, char* key);

int 
main(int argc, char *argv[]) {
    
    if(argc!=2) return usage();
    
    bl_t* bl = bl_load(argv[1]);
    printf("Loaded: %d filters\n", bl->size);

    char *key = NULL;
    size_t len = 0;
    while (getline(&key, &len, stdin) != -1) query(bl, key);
    free(key);

    bl_destroy(bl);
    return 0;
}

void
query(const bl_t* bl, char* key) {
    // new line
    key[strlen(key)-1] = 0;
    char results[8192];
    results[0] = 0;
    bl_decode(bl, key, results);
    printf("%s\n", results);
};

int 
usage() {
    printf("Usage:\n");
    printf("\t./query  bloom.bl\n");
    return 1;
}