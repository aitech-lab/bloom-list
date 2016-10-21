/*

1) Акумулирует статистику по входным данным
2) По статистике строит список блум фильтров
3) Сохраняет фильтры

*/

#include <stdio.h>

#include "filter_list.h"

int usage();

int 
main(int argc, char *argv[]) {
    
    if(argc!=2) return usage();
    
    fl_t* fl = fl_encode(argv[1], 0.01);
    fl_print(fl);
    fl_save(fl, argv[2]);
    fl_destroy(fl);

    return 0;
}

int 
usage() {
    printf("Usage:\n");
    printf("\t./encode input.txt output-bloom.blm\n");
    return 1;
}