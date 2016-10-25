/*

1) Акумулирует статистику по входным данным
2) По статистике строит список блум фильтров
3) Сохраняет фильтры

*/

#include <stdio.h>
#include <sys/stat.h>

#include "bloom_list.h"
#include "colors.h"


#define FILTER_ERROR 0.00001
#define TEST_FILE "data/100k"
#define BLOOM_FILE "data/100k.bl"
#define PRINT_KEYS 0

void test(bl_t* bl);
off_t fsize(const char *filename);

int 
main(int argc, char *argv[]) {
    
    // encode and save
    bl_t* bl = bl_encode(TEST_FILE, FILTER_ERROR);
    bl_save(bl, BLOOM_FILE);

    // calculate compression
    off_t original_size = fsize(TEST_FILE);
    off_t bloom_size = fsize(BLOOM_FILE);
    printf("Compression: "CGRN"%.2f%%"CRST"\n", (1.0-(float)bloom_size/(float)original_size)*100.0);
    
    printf("Saved filters: %d\n", bl->size);
    bl_destroy(bl);


    // load and test
    bl = bl_load(BLOOM_FILE);
    printf("Loaded filters: %d\n", bl->size);
    test(bl);
    bl_destroy(bl);
    
    return 0;
}

off_t 
fsize(const char *filename) {
    struct stat st; 
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1; 
}

static char* tokenize_line(char* line);

void
test(bl_t* bl) {

    printf("Testing:\n");
    
    FILE*  fp;
    char*  line = NULL;
    size_t len = 0;
    size_t read;

    fp = fopen(TEST_FILE, "r");
    char results[8192];
    uint32_t errors = 0;
    uint32_t lines  = 0;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        lines ++;

        char* seg =tokenize_line(line);
        char* key = line;

        if(PRINT_KEYS) printf("%s\n", key);

        if(seg != NULL && PRINT_KEYS) printf("/%s\n", seg); 

        results[0] = 0;
        bl_decode(bl, key, results);

        char* t = strtok(results, " ");
        while(t) {
            if(strstr(seg, t) == NULL) {
                if(PRINT_KEYS) printf("/"CRED"%s"CRST, t);
                errors++;
            } else {
                if(PRINT_KEYS) printf("/"CGRN"%s"CRST, t);
            }
            t = strtok(NULL, " ");
        }
        if(PRINT_KEYS) printf("\n\n");

    }
    fclose(fp);
    printf("Total lines: "CGRN"%d"CRST"\n", lines);
    printf("Bloom error: %.5f%%\n", FILTER_ERROR*100);
    printf("Errors: "CRED"%d"CRST" (%.2f%%)\n", errors, (float)errors/lines*100);
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
