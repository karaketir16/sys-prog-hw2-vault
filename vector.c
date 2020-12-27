#include "vector.h"
#include "linux/string.h"

#define DBG 
#undef DBG

#ifndef DBG
#include <linux/slab.h>		/* kmalloc() */
#else

#include <stdlib.h>

#define kmalloc(a,b) malloc(a)
#define kfree(a) free(a)

#include <stdio.h>

// int main(){
//     char_vector chv = CV_create();
//     for(char c = 'a'; c <='z'; c++){
//         CV_push(&chv, c);
//     }
//     char_vector_2D chvv = CV2D_create();
//     for(char c = 'a'; c <='z'; c++){
//         CV2D_push(&chvv, chv);
//     }
//     for(char i = 0; 'a' + i <='z'; i++){
//         char_vector tmp = CV2D_pop(&chvv);
//         for(char j = 0; 'a' + j <='z'; j++){
//             printf("test: %c\n", CV_pop(&tmp));
//         }
//     }
//     return 0;
// }

#endif

#define _ return;

char_vector null_vector = {.data=0,.size=0,.cap=0};

char_vector CV_create(int size){
    char_vector tmp;
    tmp.size = size;
    tmp.cap = tmp.size + 1;
    tmp.data = (char*) kmalloc(tmp.cap * sizeof(char), GFP_KERNEL);
    return tmp;
}

char_vector CV_create_from_cstr(char *chr,int size){
    char_vector tmp;
    tmp.size = 0;
    tmp.cap = tmp.size + 1;
    tmp.data = (char*) kmalloc(tmp.cap * sizeof(char), GFP_KERNEL);
    int i = 0;
    for( i =0; i < size;i++){
        CV_push(&tmp, chr[i]);
    }
    return tmp;
}

char CV_set_index(char_vector *this, int index, char ch){
    return this->data[index] = ch;
}

char CV_get_index(char_vector *this, int index){
    return this->data[index];
}

char CV_push(char_vector *this, char ch){
    CV_set_index(this, this->size, ch);
    this->size++;
    if(this->size == this->cap){
        char *tmp = (char*) kmalloc(this->cap * 2 * sizeof(char), GFP_KERNEL);
        memcpy(tmp, this->data, this->cap * sizeof(char));
        kfree(this->data);
        this->data = tmp;
        this->cap *= 2;
    }
    return ch;
}

char CV_pop(char_vector *this){
    this->size--;
    return CV_get_index(this, this->size);
}

void CV_free(char_vector *this){
    _
    if(this->data)
        kfree(this->data);
    this->data = NULL;
}

char_vector_2D CV2D_create(int size){
    char_vector_2D tmp;
    tmp.size = size;
    tmp.cap = tmp.size + 1;
    tmp.data = (char_vector*) kmalloc(tmp.cap * sizeof(tmp), GFP_KERNEL);
    return tmp;
}

char_vector CV2D_set_index(char_vector_2D *this, int index, char_vector chv){
    return this->data[index] = chv;
}
char_vector CV2D_get_index(char_vector_2D *this, int index){
    return this->data[index];
}

char_vector CV2D_push(char_vector_2D *this, char_vector chv){
    CV2D_set_index(this, this->size, chv);
    this->size++;
    if(this->size == this->cap){
        char_vector *tmp = (char_vector*) kmalloc(this->cap * 2 * sizeof(char_vector), GFP_KERNEL);
        memcpy(tmp, this->data, this->cap * sizeof(char_vector));
        kfree(this->data);
        this->data = tmp;
        this->cap *= 2;
    }
    return chv;
}
char_vector CV2D_pop(char_vector_2D *this){
    this->size--;
    return CV2D_get_index(this, this->size);
}

char CV2D_set(char_vector_2D *this, int index_i, int index_j, char ch){
    char_vector tmp = CV2D_get_index(this, index_i);
    return CV_set_index(&tmp, index_j, ch);
}

char CV2D_get(char_vector_2D *this, int index_i, int index_j){
    char_vector tmp = CV2D_get_index(this, index_i);
    return CV_get_index(&tmp, index_j);
}

void CV2D_free(char_vector_2D *this){
    int i;
    for(i=0;i<this->size;i++){
        CV_free(this->data + i);
    }
    kfree(this->data);
}


void swap_char_vector(char_vector *cv1, char_vector *cv2){
    char_vector tmp;
    tmp = *cv1;
    *cv1 = *cv2;
    *cv2 = tmp;
} 
