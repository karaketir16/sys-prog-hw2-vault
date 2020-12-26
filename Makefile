obj-m += asd.o
asd-objs := scull.o vector.o encrypt.o

all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean


