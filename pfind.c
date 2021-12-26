//
// Created by Abigail Ben Eliyahu on Dec-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

//mutex
pthread_mutex_t mutex;
pthread_cond_t cv;
pthread_cond_t not_empty;
pthread_cond_t all_created;
int sleeping_num = 0;

//------------------------------------------------------------------------------------------------
// ------------------------------------- FIFO queue: ---------------------------------------------
//------------------------------------------------------------------------------------------------

typedef struct dir_node{
    char rel_path[PATH_MAX];
    struct dir_node *next;
}dir_node;

typedef struct fifo_queue{
    dir_node *head;
    dir_node *tail;
} fifo_queue;

fifo_queue *init_queue(){
    fifo_queue *q = malloc(sizeof(fifo_queue));
    q->head = NULL;
    q->tail = NULL;
    return q;
}

int insert_to_queue(fifo_queue *q, char *rel_path){
    // create new node:
    dir_node *new_node = malloc(sizeof(dir_node));
    if (!new_node){
        return -1;
    }
    strcpy(new_node->rel_path,rel_path);
    new_node->next = NULL;

    // insert it as head:
    pthread_mutex_lock(&mutex); //lock
    if (q->head == NULL){
        q->head = new_node;
    }
    else{
        q->tail->next = new_node;
    }
    q->tail = new_node;
    // if some searching threads are sleeping waiting for work, should wake up one of them:
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex); //unlock
    return 0;
}

dir_node *dequeue_head(fifo_queue *q){
    dir_node *prev_head;
    pthread_mutex_lock(&mutex); //lock
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
    pthread_mutex_unlock(&mutex); //unlock
    prev_head->next=NULL;
    return prev_head;
}

// global variables:
char *root_path;
char *term;
int threads_num;
fifo_queue *q;

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

int search_in_dir(char *rel_dir_path){
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
        sprintf(entry_rel_path, "%s/%s", rel_dir_path, entry_name);
        sprintf(entry_full_path, "%s/%s", root_path, entry_rel_path);

        if (strcmp(entry_name, ".")==0 || strcmp(entry_name, "..")==0){
            // ignoring . and ..
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
            }
        }
        else{ //regular file
            if (strstr(entry_name, term)) {
                printf("%s\n", entry_rel_path);
                found_files_cnt++;
            }
        }
    }
    closedir(dir);
    printf("dir: '%s' have %d fit files\n\n", rel_dir_path, found_files_cnt); //todo del

    return found_files_cnt;
}
// --------------------------------------- thread_routine: --------------------------------------------

void *thread_routine(){

    //Wait for all other searching threads to be created:
    pthread_cond_wait(&all_created, &mutex);

    // Dequeue the head directory from the FIFO queue:
    dir_node *curr_node = dequeue_head(q);
    if (curr_node == NULL){ //empty queue
        //sleep (wait):
        sleeping_num++;
        pthread_cond_wait(&not_empty, &mutex);
        if (sleeping_num == threads_num){
            pthread_exit(0);
        }
        // todo - If all other searching threads are already waiting:
            //todo - all searching threads should exit
            //todo - return that the search in the tree is done
    } else{
        // extract the directory relative path from the node:
        char curr_rel_path[PATH_MAX];
        strcpy(curr_rel_path, curr_node->rel_path);

        // search the directory of the node:
        int num_of_finds_in_dir = search_in_dir(curr_rel_path);

        // return the search result:
        pthread_exit((void*)num_of_finds_in_dir);
    }
}

// -------------------------------------------- search in tree: ----------------------------------------

int search_in_tree(){

    int num_of_finds_in_tree; // the returned value

    // Create a FIFO queue that holds directories:
    q = init_queue();

    // Put the search root directory in the queue:
    insert_to_queue(q, "");


    // Create n searching threads:
    pthread_t threads[threads_num];
    for (int i = 0; i < threads_num; i++){
        pthread_create(&threads[i], NULL, thread_routine, NULL);
    }
    //wait for all threads to create, and then signals them to start searching:
    pthread_cond_signal(&all_created);

    //wait for all threads to finish and collect all their return values (pthread_join):
    int num_of_finds_in_dir;
    for (int i = 0; i < threads_num; i++){
        pthread_join(threads[i], (void**)&num_of_finds_in_dir);
        num_of_finds_in_tree += num_of_finds_in_dir;
    }

    return num_of_finds_in_tree;
}

// -------------------------------------------------------------------------------------------
// --------------------------------- main: ---------------------------------------------------
// -------------------------------------------------------------------------------------------

int main(int argc, char *argv[]){
    root_path = argv[1];
    term = argv[2];
    threads_num = atoi(argv[3]);

    // Initialize mutex and condition variable objects
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cv, NULL);

    int res = search_in_tree();
    printf("Done searching, found %d files\n", res);

    // destroy mutex and cv
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cv);
}