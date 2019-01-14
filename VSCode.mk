#
# Copyright (c) 2019 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# Additional targets useful when working with Visual Studio code
#
# This file can be included from the main Makefile after Base.mk to provide the
# following targets:
#
# sense:
#   Generates the c_cpp_properties.json file corresponding to the make invocation
#

ifeq ($(OS),Windows_NT)
  VSCODE_OS=Win32
else
  ifeq ($(shell uname -s),Linux)
    VSCODE_OS=Linux
  else
    VSCODE_OS=Mac
  endif
endif

.PHONY: sense

sense: .vscode/c_cpp_properties.json

.vscode/c_cpp_properties.json: FORCE
	@$(ECHO) '{' >$@
	@$(ECHO) '	"configurations": [' >>$@
	@$(ECHO) '		{' >>$@
	@$(ECHO) '			"name": "$(VSCODE_OS)",' >>$@
	@$(ECHO) '			"includePath": [' >>$@
	@$(foreach i,$(INCLUDE_DIRS),\
	 $(ECHO) '				"$${workspaceFolder}/$i",' >>$@;)
	@$(ECHO) '				""' >>$@
	@$(ECHO) '			],' >>$@
	@$(ECHO) '			"defines": [' >>$@
	@$(foreach d,$(DEFINES),\
	 $(ECHO) '				"$d",' >>$@;)
	@$(ECHO) '				""' >>$@
	@$(ECHO) '			],' >>$@
	@$(ECHO) '			"compilerPath": "$(CC)"' >>$@
	@$(ECHO) '		}' >>$@
	@$(ECHO) '	],' >>$@
	@$(ECHO) '	"version": 4' >>$@
	@$(ECHO) '}' >>$@
