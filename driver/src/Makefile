TARGET = pcboven_usb

KVERSION = $(shell uname -r)
KBUILD_DIR = /lib/modules/$(KVERSION)/build
obj-m = $(TARGET).o

.PHONY: $(obj-m)

all: $(TARGET).ko

%.ko: %.o
	$(MAKE) -C $(KBUILD_DIR) M=$(PWD) modules

clean:
	make -C $(KBUILD_DIR) M=$(PWD) clean
