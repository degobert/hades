all: src

src:
	$(MAKE) -C src -j10

clean:
	$(MAKE) clean -C src
	rm -rf packages

include version


PLATFORM_VERSION := $(shell uname -r| awk -F 'el' '{printf("%d", substr($$2,1,1))}')

LIBSTDCXX11=/usr/local/gcc-4.7.2/lib64/libstdc++.so.6.0.17 \
	    	/usr/local/gcc-4.7.2/lib64/libgcc_s.so.1

PACKAGE_NAME=$(MODULE_NAME)-$(MODULE_VERSION)
INSTALL=/usr/bin/install

install: src
	rm -rf packages/$(PACKAGE_NAME)
	mkdir -p packages/$(PACKAGE_NAME)/include/base_common
	mkdir -p packages/$(PACKAGE_NAME)/lib/
	cp -R src/*.h packages/$(PACKAGE_NAME)/include/base_common/
	cp -R src/lib$(MODULE_NAME).so.$(MODULE_VERSION)	packages/$(PACKAGE_NAME)/lib/
	find packages/$(PACKAGE_NAME) -type l|xargs rm -rf


package: clean install
	tar zcf packages/$(MODULE_NAME)-$(MODULE_VERSION)-el$(PLATFORM_VERSION).tgz -C packages $(PACKAGE_NAME)


.PHONY: all src package clean  install
