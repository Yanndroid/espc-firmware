#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp-clock-firmware

include $(IDF_PATH)/make/project.mk

info:
	$(PYTHON) $(IDF_PATH)/components/esptool_py/esptool/esptool.py --port $(ESPPORT) flash_id