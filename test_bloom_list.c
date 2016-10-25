/*

1) Акумулирует статистику по входным данным
2) По статистике строит список блум фильтров
3) Сохраняет фильтры

*/

#include <stdio.h>

#include "bloom_list.h"
#include "colors.h"

#define FILTER_ERROR 0.000001
void test(bl_t* bl);

int 
main(int argc, char *argv[]) {
    
    // encode and save
    bl_t* bl = bl_encode("data/100k", FILTER_ERROR);
    bl_save(bl, "data/100k.bl");
    printf("Saved size %d\n", bl->size);
    bl_destroy(bl);

    // load and test
    bl = bl_load("data/100k.bl");
    printf("Loaded size %d\n", bl->size);
    test(bl);
    bl_destroy(bl);
    
    return 0;
}


static char* tokenize_line(char* line);

void
test(bl_t* bl) {
    
    FILE*  fp;
    char*  line = NULL;
    size_t len = 0;
    size_t read;

    fp = fopen("data/100k", "r");
    char results[8192];
    uint32_t errors = 0;
    uint32_t lines  = 0;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        lines ++;

        char* seg =tokenize_line(line);
        char* key = line;

        printf("%s\n", key);

        if(seg != NULL) printf("/%s\n", seg); 

        results[0] = 0;
        bl_decode(bl, key, results);

        char* t = strtok(results, " ");
        while(t) {
            if(strstr(seg, t) == NULL) {
                printf("/"CRED"%s"CRST, t);
                errors++;
            } else {
                printf("/"CGRN"%s"CRST, t);
            }
            t = strtok(NULL, " ");
        }
        printf("\n\n");

    }
    fclose(fp);
    printf("LINES: "CGRN"%d"CRST"\n", lines);
    printf("FILTERS ERROR: %.5f%%\n", FILTER_ERROR*100);
    printf("ERRORS: "CRED"%d"CRST" (%.2f%%)\n", errors, (float)errors/lines*100);
}

static char*
tokenize_line(char* line) {

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
