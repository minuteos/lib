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

VSCODE_OS = $(subst Windows,Win32,$(subst Darwin,Mac,$(HOST_OS)))

LAUNCH_OUTPUT ?= $(PRIMARY_OUTPUT)
SWO_FREQUENCY ?= 1000000

.PHONY: sense vscode

sense: .vscode/c_cpp_properties.json

vscode: sense .vscode/launch.json .vscode/tasks.json

define C_CPP_PROPERTIES_TEMPLATE
{
	"configurations": [
		{
			"name": "$(VSCODE_OS)",
			"includePath": [
				$(foreach i,$(INCLUDE_DIRS),"$${workspaceFolder}/$i",)
				""
			],
			"defines": [
				$(foreach d,$(DEFINES),"$d",)
				""
			],
			"compilerPath": "$(TOOLCHAIN_PATH)$(CXX)",
			"intelliSenseMode": "gcc-x64",
			"cStandard": "gnu11",
			"cppStandard": "gnu++14",
			"macFrameworkPath": []
		}
	]
}
endef

define TASKS_TEMPLATE
{
	"version": "2.0.0",
	"tasks": [
		{
			"label": "Build Release",
			"type": "shell",
			"command": "make -j7 CONFIG=Release",
			"group": "build",
			"presentation": {
				"clear": true
			}
		},
		{
			"label": "Build Debug",
			"type": "shell",
			"command": "make -j7 CONFIG=Debug",
			"group": {
				"kind": "build",
				"isDefault": true
			},
			"presentation": {
				"clear": true
			}
		},
		{
			"label": "Clean Release",
			"type": "shell",
			"command": ["make clean CONFIG=Release"],
			"group": "test"
		},
		{
			"label": "Clean Debug",
			"type": "shell",
			"command": "make clean CONFIG=Debug",
			"group": "test"
		}
	]
}
endef

define LAUNCH_TEMPLATE
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch $(CONFIG)",
            "cwd": "$${workspaceRoot}",
            "executable": "$(LAUNCH_OUTPUT)",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "jlink",
            "preLaunchTask": "Build $(CONFIG)",
            "device": "$(JLINK_DEVICE)",
            "postStartSessionCommands": [
                "monitor SWO Start 0 $(SWO_FREQUENCY)"
            ],
            "swoConfig": {
                "enabled": true,
                "swoFrequency": $(SWO_FREQUENCY),
                "decoders": [
                    {
                        "port": 0,
                        "type": "console",
                        "label": "SWV",
                        "showOnStartup": true
                    }
                ]
            },
        },
        {
            "name": "Attach $(CONFIG)",
            "cwd": "$${workspaceRoot}",
            "executable": "$(LAUNCH_OUTPUT)",
            "request": "attach",
            "type": "cortex-debug",
            "servertype": "jlink",
            "device": "$(JLINK_DEVICE)",
            "postStartSessionCommands": [
                "monitor SWO Start 0 $(SWO_FREQUENCY)"
            ],
            "swoConfig": {
                "enabled": true,
                "swoFrequency": $(SWO_FREQUENCY),
                "decoders": [
                    {
                        "port": 0,
                        "type": "console",
                        "label": "SWV",
                        "showOnStartup": true
                    }
                ]
            },
        }
    ]
}
endef

.vscode/:
	@$(MKDIR) -p $@

.vscode/c_cpp_properties.json: export TEMPLATE=$(C_CPP_PROPERTIES_TEMPLATE)
.vscode/c_cpp_properties.json: .vscode/ FORCE
	@$(ECHO) "$$TEMPLATE" >$@

.vscode/tasks.json: export TEMPLATE=$(TASKS_TEMPLATE)
.vscode/tasks.json: .vscode/
	@$(ECHO) "$$TEMPLATE" >$@

.vscode/launch.json: export TEMPLATE=$(LAUNCH_TEMPLATE)
.vscode/launch.json: .vscode/
	@$(ECHO) "$$TEMPLATE" >$@
