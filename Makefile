RACK_DIR ?= ../../tools/Rack-SDK

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk
