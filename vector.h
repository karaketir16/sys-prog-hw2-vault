#ifndef OUR_VECTOR
#define OUR_VECTOR

typedef struct char_vector
{
    char *data;
    int size;
    int cap;
} char_vector;

char_vector CV_create(void);
char CV_push(char_vector *this, char ch);
char CV_pop(char_vector *this);
char CV_set_index(char_vector *this, int index, char ch);
char CV_get_index(char_vector *this, int index);

typedef struct char_vector_2D
{
    char_vector *data;
    int size;
    int cap;
} char_vector_2D;


char_vector_2D CV2D_create(void);
char_vector CV2D_push(char_vector_2D *this, char_vector chv);
char_vector CV2D_pop(char_vector_2D *this);
char_vector CV2D_set_index(char_vector_2D *this, int index, char_vector chv);
char_vector CV2D_get_index(char_vector_2D *this, int index);

char CV2D_set(char_vector_2D *this, int index_i, int index_j, char ch);
char CV2D_get(char_vector_2D *this, int index_i, int index_j);

#endif //OUR_VECTOR