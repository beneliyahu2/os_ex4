//
// Created by Avigail Ben Eliyahu on Dec-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct fifo_queue{ //todo fix
    DIR *first;
    DIR *last;
} fifo_queue;

void insert_to_quque(fifo_queue *q, DIR *dir){ //todo fix
    q->last = dir;
}

int is_dir(const char *path) {
    struct stat stat_buf;
    if (stat(path, &stat_buf) != 0)
        return 0;
    return S_ISDIR(stat_buf.st_mode); //S_ISDIR macro returns non-zero if the file is a directory
}

// -----------------------------------  search_in_dir: -------------------------------------------------------

int search_in_dir(char *root_path, DIR *dir, char *rel_dir_path, char *term){   //, fifo_queue queue){
    int found_files_cnt = 0;
    struct dirent *entry;
    char entry_full_path[PATH_MAX];
    char entry_rel_path[PATH_MAX];

    while((entry = readdir(dir)) != NULL){ //returns NULL when get to the end of the directory
        char *entry_name = entry->d_name;
        sprintf(entry_rel_path, "%s/%s", rel_dir_path, entry_name);
        sprintf(entry_full_path, "%s/%s", root_path, entry_rel_path);
        printf("%s\n",entry_full_path);

        if (strcmp(entry_name, ".")==0 || strcmp(entry_name, "..")==0){
            printf("ignoring . and ..\n\n"); //todo del
            continue;
        }
        if (is_dir(entry_full_path) != 0){
            //add to queue
            printf("is a dir\n\n"); //todo del
            continue; //todo del
        }
        else{ //regular file
            if (strstr(entry_name, term)){
                printf("found one!!! relpath to it: %s\n\n",entry_rel_path); //todo del
//                printf("%s\n", entry_rel_path); //todo uncomment
                found_files_cnt++;
            } else{
                printf("reg file without term\n\n"); //todo del
            }
        }
    }
    return found_files_cnt;
}

// -------------------------------------------------------------------------------------------
// --------------------------------- main: ---------------------------------------------------
// -------------------------------------------------------------------------------------------

int main(int argc, char *argv[]){
    char *root_path = argv[1];
    char *rel_path = argv[2]; //todo del
    char *term = argv[3]; // todo argv[2]
    //int threads_num = (int)argv[3]; //todo convert with function instead of casting

    printf ("root path: %s\n", root_path);
    printf ("rel path: %s\n", rel_path);
    char full_path[PATH_MAX];
    sprintf(full_path, "%s/%s", root_path, rel_path);
    printf ("full path: %s\n\n", full_path);
    DIR *dir = opendir(full_path);
    int found_dir_num = search_in_dir(root_path, dir, rel_path, term);
    printf("Done searching, found %d files\n", found_dir_num);

}