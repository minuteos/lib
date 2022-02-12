#
# Copyright (c) 2018 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# Base.mk - main makefile to be included from project Makefile
#
# Example project Makefile
# (this repository is expected to be cloned as a submodule in directory 'lib'):
#
# TARGETS = host
# COMPONENTS = kernel
#
# include lib/Base.mk
#

BASE_MAKEFILE := $(lastword $(MAKEFILE_LIST))
BASE_DIR = $(dir $(BASE_MAKEFILE))

include $(BASE_DIR)Functions.mk
include $(BASE_DIR)Host.mk

#
# Let's provide some defaults
#

# the project root is the directory containing this repository
PROJECT_ROOT = $(call parentdir,$(BASE_DIR))

# the host running the build is the default target
TARGET ?= host
TARGETS ?= $(TARGET)

# default configuration is release
CONFIG ?= Release

# default output name is the name of the directory holding the project
NAME ?= $(notdir $(realpath $(PROJECT_ROOT)))

#
# Now we collect various source directories
#
# ALL_TARGETS - names of all the targets including dependencies,
#               with the "all" target at the end
# LIB_ROOTS - all the modules that can provide components
# TARGET_ROOTS - all directories that can contain specific target directory
#                (including a 'targets' directory in the project root)
#
# TARGET_DIRS - active target directories (located under TARGET_ROOTS)
# COMPONENT_DIRS - active component directories (located under TARGET_DIRS)
# PROJECT_SOURCE_DIR - directory holding the project specific source files
# SOURCE_DIR - directory holding the source files to be compiled
# LIB_DIRS - all directories containing module source files
#

ALL_TARGETS = $(call uniq,$(filter-out all,$(TARGETS))) all
LIB_ROOTS := $(call subdirs,$(PROJECT_ROOT),lib*)
TARGET_ROOTS := $(call subdirs,$(PROJECT_ROOT) $(LIB_ROOTS),targets)
TARGET_DIRS = $(call subdirs2,$(TARGET_ROOTS),$(ALL_TARGETS))
COMPONENT_DIRS = $(call subdirs2,$(TARGET_DIRS),$(COMPONENTS))
LIB_DIRS = $(TARGET_DIRS) $(COMPONENT_DIRS)
PROJECT_SOURCE_DIR = $(PROJECT_ROOT)src/
SOURCE_DIR = $(PROJECT_SOURCE_DIR)

#
# Prepare compilation settings, so the component makefiles can modify them
#

# Output directories and primary output files
OUTTARGET = out/$(firstword $(TARGETS))/
OUTDIR    = $(OUTTARGET)$(CONFIG)/
OBJDIR    = $(OUTDIR)obj/
OUTPUT    = $(OUTDIR)$(NAME)
PRIMARY_EXT = .elf
PRIMARY_OUTPUT = $(OUTPUT)$(PRIMARY_EXT)

# Include directories are the directories which contain components
INCLUDE_DIRS = $(INCLUDE_OVERRIDE_DIRS) $(PROJECT_SOURCE_DIR) $(TARGET_DIRS) $(TARGET_ROOTS)

# Source directories are all component directories
SOURCE_DIRS = $(sort $(SOURCE_DIR) $(TARGET_DIRS) $(COMPONENT_DIRS))

# Source file extensions
SOURCE_EXTS = .c .cpp .S

# Default compilation flags to minimize output size
COMMON_FLAGS = -g -Wall -fmessage-length=0 -fno-exceptions -fdata-sections -ffunction-sections
C_FLAGS = -std=gnu11
CXX_FLAGS = -std=gnu++17 -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit
OPT_FLAGS = -O3 -Os

# Include configuration-specific makefiles
sinclude $(BASE_DIR)$(CONFIG).mk
sinclude $(PROJECT_ROOT)$(CONFIG).mk

#
# Include all makefiles from used components and targets (LIB_DIRS)
#
# Since Include.mk can add more dependencies, they are processed in
# steps until every required component and target is included.
#
# Add more dependency resolution steps if required, but please consider
# simplifying the dependency tree first :)
#

INCLUDE_MAKEFILES = $(call subfiles,$(LIB_DIRS),Include.mk $(CONFIG).mk)
REMAINING_MAKEFILES = $(filter-out $(MAKEFILE_LIST),$(INCLUDE_MAKEFILES))

sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)
sinclude $(REMAINING_MAKEFILES)

ifneq (,$(REMAINING_MAKEFILES))
  $(error Too many dependency levels, consider simplifying them or adding more resolution steps to Base.mk)
endif

sinclude $(call subfiles,$(LIB_DIRS),IncludePost.mk)

#
# Fix variables before compilation
#

COMPONENTS := $(call uniq,$(COMPONENTS))

#
# Toolchain
#

CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
SIZE = $(TOOLCHAIN_PREFIX)size
RM = rm
MKDIR = mkdir
ECHO = echo
CAT = cat
DATE = date
TEE = tee
LN = ln

#
# Fix everything for compilation
#

SOURCES := $(GENERATED_SOURCES) $(call subfiles,$(SOURCE_DIRS),$(addprefix *,$(SOURCE_EXTS))) $(ADDITIONAL_SOURCES)
OBJS    := $(addprefix $(OBJDIR),$(addsuffix .o,$(basename $(SOURCES))))
BLOBS   := $(call subfiles,$(SOURCE_DIRS),*.o) $(ADDITIONAL_BLOBS)
DEPS    := $(OBJS:.o=.d) $(ADDITIONAL_DEPS)
# all components get a macro C<component_name>, useful for optional dependencies
DEFINES += $(subst /,_,$(subst -,_,$(addprefix C,$(COMPONENTS))))
DEFINES += $(subst /,_,$(subst -,_,$(addprefix T,$(TARGETS))))

DEP_OPT = -MMD -MP
DEF_OPT = $(addprefix -D,$(DEFINES))
INC_OPT = $(call diropt,-I,$(call subdirs,$(dir $<),private) $(INCLUDE_DIRS))
LIB_OPT = $(call diropt,-L,$(LIB_DIRS) $(LINK_DIRS)) $(addprefix -l,$(LIBS))

C_OPT    = $(DEF_OPT) $(INC_OPT) $(DEP_OPT) $(CPP_FLAGS) $(ARCH_FLAGS) $(COMMON_FLAGS) $(OPT_FLAGS)
CC_OPT   = $(C_OPT) $(C_FLAGS) $(C_FLAGS_EXTRA)
CXX_OPT  = $(C_OPT) $(CXX_FLAGS) $(CXX_FLAGS_EXTRA)
LINK_OPT = $(ARCH_FLAGS) $(LIB_OPT) $(LINK_FLAGS)

#
# Automatic precompiled header support
#
# Just place a "precompiled.hpp" in the project source directory, which includes all the desired headers
#
PCH := $(call subfiles,$(PROJECT_SOURCE_DIR),precompiled.hpp)

ifneq (,$(PCH))

PCH_OPT := -include $(OBJDIR)precompiled -Winvalid-pch
PCH_FILE := $(OBJDIR)precompiled.gch
DEPS += $(OBJDIR)precompiled.d

$(PCH_FILE): $(PCH) | prebuild
	@$(MKDIR) -p $(OBJDIR)
	$(CXX) -c $< $(CXX_OPT) -o $@

endif

#
# Actual compilation rules
#

.DEFAULT_GOAL := all

.PHONY: all main prebuild disassembly test objs FORCE

.SUFFIXES:

all: main

main: prebuild $(PRIMARY_OUTPUT) disassembly

disassembly: $(OUTPUT).S $(OUTPUT).SS

objs: $(OBJS)

prebuild:
	$(info )
	$(info Compilation flags:)
	$(info DEFINES:  $(DEF_OPT))
	$(info INCLUDE:  $(INC_OPT))
	$(info ARCH:     $(ARCH_FLAGS))
	$(info COMMON:   $(COMMON_FLAGS))
	$(info OPTIMIZE: $(OPT_FLAGS))
	$(info C only:   $(C_FLAGS) $(C_FLAGS_EXTRA))
	$(info C++ only: $(CXX_FLAGS) $(CXX_FLAGS_EXTRA))
	$(info PCH:      $(PCH_OPT))
	$(info )
	$(info Output root         $(OBJDIR))
	$(info Source extensions:  $(SOURCE_EXTS))
	$(info Source directories: $(SOURCE_DIRS))
	$(info )

$(OUTDIR):
	@$(MKDIR) -p $(OUTDIR)

$(OBJDIR):
	@$(MKDIR) -p $(OBJDIR)

$(OBJDIR)%.o: %.c | prebuild
	@$(MKDIR) -p $(dir $@)
	$(info $(CC) -c $<)
	@$(CC) -c $< $(CC_OPT) -o $@

$(OBJDIR)%.o: %.S | prebuild
	@$(MKDIR) -p $(dir $@)
	$(info $(CC) -c $<)
	@$(CC) -c $< $(CC_OPT) -o $@

$(OBJDIR)%.nopch.o: %.nopch.cpp | prebuild
	@$(MKDIR) -p $(dir $@)
	$(info $(CXX) -c $<   # without PCH)
	@$(CXX) -c $< $(CXX_OPT) -o $@

$(OBJDIR)%.o: %.cpp $(PCH_FILE) | prebuild
	@$(MKDIR) -p $(dir $@)
	$(info $(CXX) -c $<)
	@$(CXX) -c $< $(CXX_OPT) $(PCH_OPT) -o $@

$(PRIMARY_OUTPUT): $(OBJS) $(BLOBS) | prebuild
	@$(MKDIR) -p $(dir $@)
	$(CXX) -o $@ $(sort $(OBJS) $(BLOBS)) $(LINK_OPT)
	@$(SIZE) $@

$(OUTPUT).S: $(PRIMARY_OUTPUT)
	-$(OBJDUMP) -d $< > $@

$(OUTPUT).SS: $(PRIMARY_OUTPUT)
	-$(OBJDUMP) -d -S $< > $@

# Include generated dependency files, unless we're cleaning
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifneq ($(DEPS),)
  sinclude $(DEPS)
endif
endif

clean:
	@$(RM) -f $(OBJS) $(DEPS) $(GENERATED_SOURCES) $(PCH_FILE) $(OUTPUT).*

distclean:
	@$(RM) -rf $(OBJDIR)

configclean:
	@$(RM) -rf $(OUTDIR)

targetclean:
	@$(RM) -rf $(OUTTARGET)

hardclean:
	@$(RM) -rf out

ifneq (,$(findstring test,$(MAKECMDGOALS)))
include $(BASE_DIR)Tests.mk
endif
