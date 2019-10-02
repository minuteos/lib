#
# Copyright (c) 2018 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# Functions.mk - various helper functions used in Makefiles to make them
#                a bit more readable
#

# get the path of the current makefile
curmake = $(lastword $(MAKEFILE_LIST))

# gets the parent directories, either of a file or of a directory
parentdir = $(dir $(1:/=))

# gets the canonical name of a file or directory, essentially just stripping ./
# from the beginning
relcanon = $(1:./%=%)

# provides all existing files by combining both arguments
subfiles = $(call relcanon,$(foreach d,$(1),$(wildcard $(addprefix $(d),$(2)))))

# provides all existing combnations of directories from both arguments
subdirs = $(call relcanon,$(patsubst %/.,%/,$(wildcard $(foreach d,$(1),$(addprefix $(d),$(addsuffix /.,$(2)))))))

# provides all existing combnations of directories from both arguments, prefering the order in the second argument
subdirs2 = $(call relcanon,$(patsubst %/.,%/,$(wildcard $(foreach d,$(2),$(addsuffix $(d)/.,$(1))))))

# extracts directory names from a set of directories or files
dirname = $(notdir $(1:/=))

# passes directories as compilation options
diropt = $(addprefix $(1),$(2:/=))

# removes duplicates from the set
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
