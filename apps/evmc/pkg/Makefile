include build.mk

all: prepare
	@export builddir=$(shell pwd)/build; \
	echo $(RPM_VERSION); \
	for pkg in $(PKG); do \
		pkg=$$(basename $${pkg} .spec); \
		echo "Building $${pkg}"; \
		path=$$(rpm -q --specfile --qf "build/RPMS/%{arch}/$${pkg}-${RPM_VERSION}-%{release}.%{arch}.rpm " $${pkg}.spec); \
		echo $${path};\
		rpmbuild --define="_module_name ${PROJ}" --define="_version ${RPM_VERSION}" --define="_topdir $${builddir}" --define="_major ${MAJOR}" --define="_minor {$MINOR}" -bb $${pkg}.spec ; \
			echo 'pwd:'; pwd ; \
			echo $${path};\
			mkdir -p backup-rpms ; \
			cp build/RPMS/*/*.rpm . ;\
			cp build/RPMS/*/*.rpm ./backup-rpms ;\
			continue; \
		echo "Build failed for $${path}, abort."; \
		break; \
	done

# Make rpm building base and source tarball
prepare: clean-build clean-rpm
	echo "Creating source package to build/SOURCES/$(SRC)"
	rm -rf $(TEMP_DIR)/apps
	rm -rf $(TEMP_DIR)/evpp
	rm -rf $(TEMP_DIR)/rpm
	rm -rf $(TEMP_DIR)/build.mk
	rm -rf $(TEMP_DIR)/$(DEPEND_PROJ)
	mkdir -p $(TEMP_DIR)/apps
	mkdir -p $(TEMP_DIR)/$(DEPEND_PROJ)
	ln -s $(ROOT_DIR) $(TEMP_DIR)/apps/$(PROJ)
	ln -s $(ROOT_DIR)/../../evpp  $(TEMP_DIR)/evpp
	ln -s $(ROOT_DIR)/../../rpm  $(TEMP_DIR)/rpm
	ln -s $(ROOT_DIR)/../../build.mk  $(TEMP_DIR)/build.mk
	ln -s $(HASHKIT_DIR) $(TEMP_DIR)/$(DEPEND_PROJ)/libhashkit
	ln -s $(MEMCACHE_DIR) $(TEMP_DIR)/$(DEPEND_PROJ)/memcached
	ln -s $(JSON_DIR) $(TEMP_DIR)/$(DEPEND_PROJ)/rapidjson
	$(MAKE) clean -C $(ROOT_DIR)
	tar zchf $(TEMP_DIR)/$(SRC) --exclude "*.tar.gz" --exclude "*.d" --exclude "*.o" --exclude install --exclude bin --exclude "*.so" --exclude "*.a" --exclude "*.rpm" -C $(TEMP_DIR)  evpp apps  $(DEPEND_PROJ) rpm build.mk
	mkdir -p build/{BUILD,RPMS,SRPMS,SPECS,SOURCES}
	mv $(TEMP_DIR)/$(SRC) build/SOURCES/

clean-build:
	rm -rf build

clean-rpm:
	rm -rf *rpm

t :
	@echo $(ROOT_DIR)
	@echo $(COMMITVERSION)

clean: clean-build clean-rpm

.PHONY: pkg prepare clean clean-build clean-rpm t

