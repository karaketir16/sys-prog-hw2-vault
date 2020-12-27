#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "scull_ioctl.h"

int main(){
    int f = open("/dev/scull", O_RDWR);
    int ret_val;
    if(f <= 0){
        printf("File can not open\n");
        return -1;
    }

    vault_key_t my_key;
    my_key.ptr = "ceayf";
    my_key.size = 5;

    printf("Seting key as 'ceayf'\n");
    ret_val = ioctl(f, SCULL_IOC_SET_KEY, &my_key);
    if(ret_val >= 0) {
        printf("Key setted as 'ceayf'\n");
    } else {
        printf("Cannot set, error code: %d\n", ret_val);
    }

    char read_buf[100];

    printf("Reading test string before writing\n");
    ret_val = read(f,read_buf,100);
    if(ret_val >= 0){
        printf("Read string: ");
        for(int i =0; i < ret_val;i++) printf("%c",read_buf[i]);
    } else {
        printf("Cannot read, error code: %d\n", ret_val);
    }

    printf("\n");
    printf("\n");
    char testString[] = "oneringtorulethemall";

    printf("Writing test string: %s\n", testString);
    ret_val = write(f,testString, sizeof(testString));
    if(ret_val > 0){
        printf("Written %d\n", ret_val);
    } else {
        printf("Cannot write, error code: %d\n", ret_val);
    }

    printf("\n");
    printf("\n");

    printf("Writing test string again, before reading: %s\n", testString);
    lseek(f,0,SEEK_SET);
    ret_val = write(f,testString, sizeof(testString));
    if(ret_val > 0){
        printf("Written %d\n", ret_val);
    } else {
        printf("Cannot write, error code: %d\n", ret_val);
    }

    printf("\n");
    printf("\n");

    printf("Reading test string after writing\n");
    lseek(f,0,SEEK_SET);
    ret_val = read(f,read_buf,100);
    if(ret_val >= 0){
        printf("Read string: ");
        for(int i =0; i < ret_val;i++) printf("%c",read_buf[i]);
    } else {
        printf("Cannot read, error code: %d\n", ret_val);
    }

    printf("\n");
    printf("\n");

    printf("Writing test string again, after reading: %s\n", testString);
    lseek(f,0,SEEK_SET);
    ret_val = write(f,testString, sizeof(testString));
    if(ret_val > 0){
        printf("Written %d\n", ret_val);
    } else {
        printf("Cannot write, error code: %d\n", ret_val);
    }
    return 0;    
}

// make clean && make all && insmod ./asd.ko && mknod /dev/scull c 240 0 && grep scull /proc/devices
// rm -f /dev/scull && rmmod asd