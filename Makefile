#
# Makefile for building and installing the ferret shared-object library 
# (libferret.so), the pyferret module with its shared-object library 
# (_pyferret.so), and the ferret.py script.
#

#
# Site-specific defines
#
include site_specific.mk

.PHONY : all
all : optimized

.PHONY : optimized
optimized : optimizedbuild install

.PHONY : debug
debug : debugbuild install

.PHONY : optimizedbuild
optimizedbuild :
	mkdir -p $(DIR_PREFIX)/lib
	$(MAKE) -C $(DIR_PREFIX)/fer optimized
	# $(MAKE) -C $(DIR_PREFIX)/external_functions
	@echo "***** NOTE: external function .so files not built *****"
	$(MAKE) pymod

.PHONY : debugbuild
debugbuild : 
	mkdir -p $(DIR_PREFIX)/lib
	$(MAKE) -C $(DIR_PREFIX)/fer debug
	# $(MAKE) -C $(DIR_PREFIX)/external_functions debug
	@echo "***** NOTE: external function .so files not built *****"
	$(MAKE) "CFLAGS += -O0 -g" pymod

.PHONY : pymod
pymod :
	rm -fr $(DIR_PREFIX)/build
	cd $(DIR_PREFIX) ; export HDF5_DIR=$(HDF5_DIR) ; export NETCDF_DIR=$(NETCDF_DIR) ; $(PYTHON_EXE) setup.py build

.PHONY : install
install :
ifeq ( $(strip $(FER_LIBS)), )
	@echo ""
	@echo " ERROR: environment variable FER_LIBS is not defined"
	@echo "        installation unsuccessful"
	@echo ""
else
	cp -f $(DIR_PREFIX)/fer/threddsBrowser/threddsBrowser.jar $(FER_LIBS)
	cd $(DIR_PREFIX) ; export HDF5_DIR=$(HDF5_DIR) ; export NETCDF_DIR=$(NETCDF_DIR) ; $(PYTHON_EXE) setup.py install $(PYTHON_INSTALL_FLAGS)
ifeq ( $(strip $(FER_LOCAL_EXTFCNS)), )
	@echo ""
	@echo " ERROR: environment variable FER_LOCAL_EXTFCNS is not defined"
	@echo "        external function .so files not installed"
	@echo ""
else
	# $(MAKE) -C $(DIR_PREFIX)/external_functions install
	@echo "***** NOTE: external function .so files not installed *****"
endif
endif

.PHONY : clean
clean :
	rm -fr $(DIR_PREFIX)/build ferret.jnl*
	$(MAKE) -C $(DIR_PREFIX)/external_functions clean
	$(MAKE) -C $(DIR_PREFIX)/fer clean
	rm -fr $(DIR_PREFIX)/lib
	@echo ""
	@echo "    NOTE: Only the build, external_functions, fer, fmt, ppl,"
	@echo "          and lib directories were cleaned.  Other directories"
	@echo "          (in particular, xgks) were not changed."
	@echo ""

#
