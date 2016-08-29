obj-m += execuser.o

all:
	make -C /usr/src/linux-headers-$(shell dpkg-query --showformat='$${Version}' --show linux-headers-generic| awk -F '.' '{ print $$1"."$$2"."$$3"-"$$4"-generic" }') SUBDIRS=$$PWD modules

clean:
	make -C /usr/src/linux-headers-$(shell dpkg-query --showformat='$${Version}' --show linux-headers-generic| awk -F '.' '{ print $$1"."$$2"."$$3"-"$$4"-generic" }') SUBDIRS=$$PWD clean
