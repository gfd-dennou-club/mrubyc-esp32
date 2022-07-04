#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_DEPENDS := mrubyc-3.1
#COMPONENT_EXTRA_CLEAN = SRCFILES
CFLAGS = -mlongcalls -DMRBC_USE_HAL_ESP32

MRBC     = mrbc

SRCDIR   = $(PROJECT_PATH)/src
SRCFILES = $(wildcard $(SRCDIR)/*.rb)
OBJS     = $(patsubst %.rb,%.h,$(SRCFILES))

CLASSDIR   = $(PROJECT_PATH)/mrblib
CLASSFILES = $(wildcard $(CLASSDIR)/*.rb)
MYCLASS    = myclass_bytecode

main.o: $(OBJS)

$(SRCDIR)/%.h: $(SRCDIR)/%.rb
	@echo $(MRBC) -E -B $(basename $(notdir $@)) -o $(subst $(SRCDIR),$(COMPONENT_BUILD_DIR),$@) $^
	$(MRBC) -E -B $(basename $(notdir $@)) -o $(subst $(SRCDIR),$(COMPONENT_BUILD_DIR),$@) $^
	@echo $(MRBC) -E -B $(MYCLASS) --remove-lv -o $(PROJECT_PATH)/main/mrblib.c  $(CLASSFILES)
	$(MRBC) -E -B $(MYCLASS) --remove-lv -o $(PROJECT_PATH)/main/mrblib.c  $(CLASSFILES)
