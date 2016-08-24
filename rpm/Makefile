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
	mkdir -p $(TEMP_DIR)
	rm -f $(TEMP_DIR)/$(PROJ)
	ln -s $(ROOT_DIR) $(TEMP_DIR)/$(PROJ)
	$(MAKE) clean -C $(ROOT_DIR)
	tar zchf $(TEMP_DIR)/$(SRC) --exclude "*.tar.gz" --exclude install --exclude bin --exclude "*.so" --exclude "*.a" --exclude "*.rpm" -C $(TEMP_DIR) $(PROJ)
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

