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
int main(){
    char_vector chv = CR_create();
    for(char c = 'a'; c <='z'; c++){
        CR_push(&chv, c);
    }
    char_vector_2D chvv = CR2D_create();
    for(char c = 'a'; c <='z'; c++){
        CR2D_push(&chvv, chv);
    }
    for(char i = 0; 'a' + i <='z'; i++){
        char_vector tmp = CR2D_pop(&chvv);
        for(char j = 0; 'a' + j <='z'; j++){
            printf("test: %c\n", CR_pop(&tmp));
        }
    }
    return 0;
}

#endif



char_vector CR_create(){
    char_vector tmp;
    tmp.size = 0;
    tmp.cap = tmp.size + 1;
    tmp.data = (char*) kmalloc(tmp.cap * sizeof(char), GFP_KERNEL);
    return tmp;
}

char CR_set_index(char_vector *this, int index, char ch){
    return this->data[index] = ch;
}

char CR_get_index(char_vector *this, int index){
    return this->data[index];
}

char CR_push(char_vector *this, char ch){
    CR_set_index(this, this->size, ch);
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

char CR_pop(char_vector *this){
    this->size--;
    return CR_get_index(this, this->size);
}

char_vector_2D CR2D_create(){
    char_vector_2D tmp;
    tmp.size = 0;
    tmp.cap = tmp.size + 1;
    tmp.data = (char_vector*) kmalloc(tmp.cap * sizeof(tmp), GFP_KERNEL);
    return tmp;
}

char_vector CR2D_set_index(char_vector_2D *this, int index, char_vector chv){
    return this->data[index] = chv;
}
char_vector CR2D_get_index(char_vector_2D *this, int index){
    return this->data[index];
}

char_vector CR2D_push(char_vector_2D *this, char_vector chv){
    CR2D_set_index(this, this->size, chv);
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
char_vector CR2D_pop(char_vector_2D *this){
    this->size--;
    return CR2D_get_index(this, this->size);
}

char CR2D_set(char_vector_2D *this, int index_i, int index_j, char ch){
    char_vector tmp = CR2D_get_index(this, index_i);
    return CR_set_index(&tmp, index_j, ch);
}

char CR2D_get(char_vector_2D *this, int index_i, int index_j){
    char_vector tmp = CR2D_get_index(this, index_i);
    return CR_get_index(&tmp, index_j);
}