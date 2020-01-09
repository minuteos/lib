#
# Copyright (c) 2020 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# Host.mk
#
# Host OS identification and abstraction helpers
#

ifeq ($(OS),Windows_NT)
  HOST_OS := Windows
else
  HOST_OS := $(shell uname -s)

  ifeq (,$(filter $(HOST_OS),Linux Darwin))
    $(warning Unknown OS: $(HOST_OS))
  endif
endif
