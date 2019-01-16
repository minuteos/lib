#
# Copyright (c) 2018 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# Test.mk - root Makefile for building test suites
#
# Expected inputs variables:
#
# OUTDIR - directory where the build artifacts are to be put
# TEST - directory where the test suite sources are located
#

# force variables that would be incorrectly detected by Base.mk
override BASE_DIR = $(dir $(lastword $(MAKEFILE_LIST)))
override SOURCE_DIR = $(TEST)
override PCH = lib/testrunner/precompiled.hpp 

include $(BASE_DIR)Functions.mk

# derive the name of the output and default components from the input
NAME = $(call dirname,$(OUTDIR))
TEST_COMPONENT = $(call dirname,$(call parentdir,$(call parentdir,$(dir $(TEST)))))
COMPONENTS = testrunner $(TEST_COMPONENT)

# test can request additional components to be included
sinclude $(TEST)Include.mk

include $(BASE_DIR)Base.mk
