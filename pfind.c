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
#include <pthread.h>

//------------------------------------------------------------------------------------------------
// ------------------------------------- FIFO queue: ---------------------------------------------
//------------------------------------------------------------------------------------------------

typedef struct dir_node{
    char rel_path[PATH_MAX];
    struct dir_node *next;
}dir_node;

typedef struct fifo_queue{ //todo fix
    dir_node *head;
    dir_node *tail;
} fifo_queue;

fifo_queue *init_queue(){
    fifo_queue *q = malloc(sizeof(fifo_queue));
    q->head = NULL;
    q->tail = NULL;
    return q;
}

int insert_to_queue(fifo_queue *q, char *rel_path){ //todo fix
    // create new node:
    dir_node *new_node = malloc(sizeof(dir_node));
    if (!new_node){
        return -1;
    }
    strcpy(new_node->rel_path,rel_path);
    new_node->next = NULL;
    // insert it as head:
    if (q->head == NULL){
        q->head = new_node;
    }
    else{
        q->tail->next = new_node;
    }
    q->tail = new_node;
    return 0;
}

dir_node *dequeue_head(fifo_queue *q){
    dir_node *prev_head;
    if (q->head == NULL){
        return NULL;
    }
    prev_head = q->head;
    if (q->head->next == NULL){
        q->head = NULL;
        q->tail = NULL;
        return prev_head;
    }
    q->head = q->head->next;
    prev_head->next=NULL;
    return prev_head;
}

// ------------------------------------ utilities: -----------------------------------------------------------

int is_dir(const char *path) {
    void *ret_val;
    struct stat stat_buf;
    if (stat(path, &stat_buf) != 0)
        pthread_exit(ret_val); //todo - deal with the error and catch the exit in join (?)
    return S_ISDIR(stat_buf.st_mode); //S_ISDIR macro returns non-zero if the file is a directory
}
// -----------------------------------------------------------------------------------------------------------
// -----------------------------------  search_in_dir: -------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------

int search_in_dir(char *root_path, fifo_queue *q, char *rel_dir_path,const char *term){   //, fifo_queue queue){
    int found_files_cnt = 0;
    struct dirent *entry = NULL;
    char entry_full_path[PATH_MAX];
    char entry_rel_path[PATH_MAX];
    char dir_full_path[PATH_MAX];
    DIR *entry_dir;

    printf("scanning dir: '%s'\n", rel_dir_path); //todo del

    // open directory:
    sprintf(dir_full_path, "%s/%s",root_path, rel_dir_path);
    DIR *dir = opendir(dir_full_path);

    //scanning the entries of the directory:
    while((entry = readdir(dir)) != NULL){ //returns NULL when get to the end of the directory
        char *entry_name = entry->d_name;
        printf("entry: %s\n",entry_name); //todo del
        sprintf(entry_rel_path, "%s/%s", rel_dir_path, entry_name);
        sprintf(entry_full_path, "%s/%s", root_path, entry_rel_path);
//        printf("entry: %s\n",entry_full_path); //todo del

        if (strcmp(entry_name, ".")==0 || strcmp(entry_name, "..")==0){
//            printf("ignoring . and ..\n\n"); //todo del
        }
        else if (is_dir(entry_full_path) != 0){
            errno = 0;
            entry_dir = opendir(entry_full_path);
            if (entry_dir == NULL) { // directory can’t be searched
                printf("Directory %s: Permission denied.\n", entry_rel_path);
            }
            else{ //directory can be searched
                //add to queue:
                closedir(entry_dir);
                insert_to_queue(q, entry_rel_path);
//                printf("Directory %s: added to queue.\n", entry_name); //todo del
            }
        }
        else{ //regular file
            if (strstr(entry_name, term)){
//                printf("found one!!! relative path to it: %s\n\n",entry_rel_path); //todo del
//                printf("%s\n", entry_rel_path); //todo uncomment
                found_files_cnt++;
            } else{
//                printf("reg file without term\n\n"); //todo del
            }
        }
    }
    closedir(dir);
    printf("dir: '%s' have %d fit files\n\n", rel_dir_path, found_files_cnt); //todo del

    return found_files_cnt;
}

// -------------------------------------------- search in tree: ----------------------------------------

int search_in_tree(char *root_path,const char *term){
    int fit_files_in_tree;
    char curr_rel_path[PATH_MAX];

    fifo_queue *q = init_queue();
    insert_to_queue(q, "");

    dir_node *curr_node;
    while(1){
        curr_node = dequeue_head(q);
        if (curr_node == NULL){
            break;
        }
        strcpy(curr_rel_path, curr_node->rel_path);
        fit_files_in_tree += search_in_dir(root_path, q, curr_rel_path, term);
//        break;
    }
    return fit_files_in_tree;
}

// -------------------------------------------------------------------------------------------
// --------------------------------- main: ---------------------------------------------------
// -------------------------------------------------------------------------------------------

int main(int argc, char *argv[]){
    char *root_path = argv[1];
    char *term = argv[2];
    //int threads_num = atoi(argv[3]); //todo uncomment

    int res = search_in_tree(root_path, term);
    printf("Done searching, found %d files\n", res);
}