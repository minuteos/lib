#
# Copyright (c) 2021 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# GitAppVersion.mk
#
# Defines macro APP_VERSION via version tags in git
#

GIT_DESCRIBE := $(subst -, ,$(shell git describe --tags --match="v*" --long --dirty --abbrev=8))

GIT_DESCRIBE_TAG = $(word 1,$(GIT_DESCRIBE))
GIT_DESCRIBE_DISTANCE = $(word 2,$(GIT_DESCRIBE))
GIT_DESCRIBE_COMMIT = $(patsubst g%,%,$(word 3,$(GIT_DESCRIBE)))
GIT_DESCRIBE_DIRTY = $(word 4,$(GIT_DESCRIBE))

ifneq (,$(GIT_DESCRIBE_COMMIT))
  DEFINES += APP_VERSION_COMMIT=$(GIT_DESCRIBE_COMMIT) APP_VERSION_COMMIT_32=0x$(GIT_DESCRIBE_COMMIT)uL
endif

GIT_APP_VERSION = $(patsubst v%,%,$(GIT_DESCRIBE_TAG))

ifneq (0,$(GIT_DESCRIBE_DISTANCE))
  GIT_APP_VERSION := $(GIT_APP_VERSION).$(GIT_DESCRIBE_DISTANCE)
endif

ifneq (,$(GIT_DESCRIBE_DIRTY))
  DEFINES += APP_VERSION_DIRTY=1
endif

GIT_APP_VERSION_PARTS = $(subst ., ,$(GIT_APP_VERSION))

GIT_APP_VERSION_32 = ($(or $(word 1,$(GIT_APP_VERSION_PARTS)),0)<<24|$(or $(word 2,$(GIT_APP_VERSION_PARTS)),0)<<16|$(or $(word 3,$(GIT_APP_VERSION_PARTS)),0)<<8|$(or $(word 4,$(GIT_APP_VERSION_PARTS)),0))

DEFINES += APP_VERSION=$(GIT_APP_VERSION) APP_VERSION_32="$(GIT_APP_VERSION_32)"
