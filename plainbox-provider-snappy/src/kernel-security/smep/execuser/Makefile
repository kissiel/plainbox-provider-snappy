obj-m += execuser.o

all:
	make -C /usr/src/linux-headers-$(shell dpkg-query --showformat='${Version}' --show linux-headers-generic) SUBDIRS=$$PWD modules

clean:
	make -C /usr/src/linux-headers-$(shell dpkg-query --showformat='${Version}' --show linux-headers-generic) SUBDIRS=$$PWD clean
