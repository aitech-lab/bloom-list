/*

1) Акумулирует статистику по входным данным
2) По статистике строит список блум фильтров
3) Сохраняет фильтры

*/

#include <stdio.h>

#include "bloom_list.h"

int usage();

int 
main(int argc, char *argv[]) {
    
    if(argc!=3) return usage();
    
    bl_t* bl = bl_encode(argv[1], 0.01);
    bl_print(bl);
    bl_save(bl, argv[2]);
    bl_destroy(bl);

    return 0;
}

int 
usage() {
    printf("Usage:\n");
    printf("\t./encode input.txt output-bloom.blm\n");
    return 1;
}