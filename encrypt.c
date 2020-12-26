#include "vector.h"
#include "encrypt.h"
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
    char_vector key_test = CV_create(0);
    CV_push(&key_test, 'c');
    CV_push(&key_test, 'e');
    CV_push(&key_test, 'a');
    CV_push(&key_test, 'y');
    CV_push(&key_test, 'f');
    int* key = key_to_permutation(key_test);
    int* inverse = inverse_of_key(key, 5);
    for(int i = 0; i < 5; i++) printf("%d ", key[i]);
    puts("");
    for(int i = 0; i < 5; i++) printf("%d ", inverse[i]);
    puts("");
    // oneringtorulethemall
    char_vector res = encrypt(CV_create_from_cstr("oneringtorulethemall", strlen("oneringtorulethemall")), key_test);
    printf("\nsize: %d\n", res.size);

    for(int i = 0; i < res.size; i++) printf("%c", res.data[i]);
    puts("");


    res = decrypt(CV_create_from_cstr("neoirgtnroleuhtmaell", strlen("neoirgtnroleuhtmaell")), key_test);
    printf("\nsize: %d\n", res.size);

    for(int i = 0; i < res.size; i++) printf("%c", res.data[i]);
    puts("");

    return 0;
}

#endif


int order_of_char(char c) {
    return c - 'a';
}

int* inverse_of_key(int* key, int size) {
    int i;
    int* inverse_key = kmalloc(sizeof(int) * size, GFP_KERNEL);
    for(i=0; i<size; i++) {
        inverse_key[key[i]] = i;
    }
    // The caller should free this memory
    return inverse_key;
}

int* key_to_permutation(char_vector key_char) {
    int i;
    int size = key_char.size;
    int* key = kmalloc(sizeof(int) * size, GFP_KERNEL);
    // ac => 1 3 => 2
    int *is_exist = (int*) kmalloc(sizeof(int) * 30, GFP_KERNEL);
    memset(is_exist, 0, sizeof (int) * 30);
    for(i=0; i<size; i++) {
        // c => 0 based
        int c = order_of_char(CV_get_index(&key_char, i));
        is_exist[c] = 1;
        key[i] = c;
    }
    for(i = 1; i < 26; i++) {
        is_exist[i] += is_exist[i - 1];
    }
    for(i=0; i<size; i++) {
        key[i] = is_exist[key[i]] - 1;
    }
    // The caller should free the memory
    return key;
}


extern char_vector encrypt(char_vector text, char_vector key){
    while(text.size % key.size != 0){
        CV_push(&text, '0');
    }
    int* tmp = key_to_permutation(key);
    int* permutation = inverse_of_key(tmp, key.size);
    kfree(tmp);
    
    char_vector ret_val = permiter(text, permutation, key.size);
    kfree(permutation);
    return ret_val;
}

char_vector decrypt(char_vector text, char_vector key){
    int* permutation = key_to_permutation(key);

    char_vector ret_val = permiter(text, permutation, key.size);
    kfree(permutation);
    return ret_val;
}

char_vector permiter(char_vector text, int *permutation, int key_size){
    char_vector_2D encrpyted_matrix = CV2D_create(0);
    int i, j;
    for(i=0; i<text.size / key_size; i++){
        char_vector row = CV_create(key_size);
        for(j=0; j<key_size; j++){
            CV_set_index(&row, permutation[j], CV_get_index(&text, i*key_size + j));
        }
        CV2D_push(&encrpyted_matrix, row);
    }
    char_vector encrypted_string = CV_create(0);
    for(i=0; i<text.size; i++) {
        CV_push(&encrypted_string, CV2D_get(&encrpyted_matrix, i / key_size, i % key_size));
    }
    return encrypted_string;
}

