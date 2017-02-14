.PHONY:
all: clocktest threaded_memtest kernel-security alsa_test

.PHONY: clean
clean:
	rm -f clocktest threaded_memtest

.PHONY: kernel-security
kernel-security:
	cp -r ../../src/kernel-security .
	make -C kernel-security

threaded_memtest: CFLAGS += -pthread
threaded_memtest: CFLAGS += -Wno-unused-but-set-variable
clocktest: CFLAGS += -D_POSIX_C_SOURCE=199309L
clocktest: LDLIBS += -lrt
alsa_test: CXXFLAGS += -std=c++11
alsa_test: LDLIBS += -lasound -pthread

CFLAGS += -Wall
