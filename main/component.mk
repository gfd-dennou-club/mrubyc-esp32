#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

#COMPONENT_DEPENDS := mrubyc
#COMPONENT_EXTRA_CLEAN = SRCFILES

MRBC     = mrbc
SRCDIR   = $(PROJECT_PATH)/mrblib
SRCFILES = $(wildcard $(SRCDIR)/*.rb)
OBJS     = $(patsubst %.rb,%.h,$(SRCFILES))

CLASSDIR   = $(SRCDIR)/models
CLASSFILES = $(wildcard $(CLASSDIR)/*.rb)

main.o: $(OBJS)

$(SRCDIR)/%.h: $(SRCDIR)/%.rb
	@echo $(MRBC) -E -B $(basename $(notdir $@)) -o $(subst $(SRCDIR),$(COMPONENT_BUILD_DIR),$@) $^
	$(MRBC) -E -B $(basename $(notdir $@)) -o $(subst $(SRCDIR),$(COMPONENT_BUILD_DIR),$@) $^
	@echo $(MRBC) -E -B mrblib_bytecode --remove-lv -o $(PROJECT_PATH)/main/mrblib.c  $(CLASSFILES)	
	$(MRBC) -E -B mrblib_bytecode --remove-lv -o $(PROJECT_PATH)/main/mrblib.c  $(CLASSFILES)
