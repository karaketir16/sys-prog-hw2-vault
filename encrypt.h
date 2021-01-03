#ifndef ENCRYPT
#define ENCRYPT

#include "vector.h"

#include <linux/module.h>

int order_of_char(char c);
int* inverse_of_key(int* key, int size) ;
int* key_to_permutation(char_vector *key_char);
char_vector encrypt(char_vector *text, char_vector *key);
char_vector permiter(char_vector *text, int *permutation, int key_size);
char_vector decrypt(char_vector *text, char_vector *key);


#endif
