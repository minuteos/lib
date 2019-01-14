#
# Copyright (c) 2019 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# targets/host/Include.mk
#
# Makefile modifications when targeting host system
#

.PHONY: run

run: $(OUTPUT).elf
	@$(OUTPUT).elf
