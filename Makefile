obj-m += vault.o
vault-objs := vault_code.o vector.o encrypt.o

all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean


