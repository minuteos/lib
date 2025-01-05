#
# Copyright (c) 2018 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# Tests.mk - targets for building and running tests
#

# Any source directory can contain tests
ifeq (,$(TEST_COMPONENTS))
TEST_SOURCE_DIRS = $(SOURCE_DIR)
SOURCE_TEST_COMPONENTS ?= $(COMPONENTS)
TEST_COMPONENT_OVERRIDE = "TEST_COMPONENT=$(SOURCE_TEST_COMPONENTS)"
else
TEST_COMPONENT_DIRS = $(call subdirs2,$(TARGET_DIRS),$(TEST_COMPONENTS))
TEST_SOURCE_DIRS = $(sort $(TEST_COMPONENT_DIRS))
endif
TEST_DIRS = $(call subdirs,$(TEST_SOURCE_DIRS),tests)
TEST_SUITES ?= *
TEST_SUITE_DIRS = $(sort $(call subdirs,$(TEST_DIRS),$(TEST_SUITES)))

# output directory for built tests
OUTTEST = $(OUTDIR)tests/

# helper to get rid of the output directory prefix
t_stripout = $(patsubst $(OUTTEST)%,%,$(1))

TEST_OUTPUTS  := $(addprefix $(OUTTEST),$(TEST_SUITE_DIRS))
TEST_BINARIES := $(foreach t,$(TEST_OUTPUTS),$(addsuffix $(call dirname,$(t))$(PRIMARY_EXT),$t))
TEST_RESULTS  := $(TEST_BINARIES:$(PRIMARY_EXT)=.testres)
TEST_SUMMARY  := $(OUTTEST)summary.testres
TEST_RUN_ARGS ?= $(TEST_FILTERS)

$(TEST_SUMMARY): $(TEST_RESULTS)
	@$(DATE) >$@
	@$(foreach d,$(TEST_RESULTS),$(ECHO) "\n## $(call t_stripout,$(dir $(d)))\\n" >>$@; $(CAT) $(d) >>$@;)
	$(info )
	$(info Test log can be found in '$@')

.PHONY: $(TEST_BINARIES)

export TARGET CONFIG

$(TEST_BINARIES):
	$(info )
	$(info *** Building test suite '$(patsubst $(OUTTEST)%,%,$(dir $@))')
	$(info )
	@$(MAKE) -f $(BASE_DIR)Test.mk OUTDIR=$(dir $@) TEST=$(patsubst $(OUTTEST)%,%,$(dir $@)) $(TEST_COMPONENT_OVERRIDE) main

%.testres: %$(PRIMARY_EXT)
	$(info )
	$(info *** Running test suite '$(patsubst $(OUTTEST)%,%,$(dir $@))')
	$(info )
	@$(TEST_RUN) $< $(TEST_RUN_ARGS) 2>&1 | $(TEE) $@

.PHONY: tests test

tests: $(TEST_BINARIES)

test: $(TEST_SUMMARY)
