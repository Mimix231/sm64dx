# ----------------------
# Dynamic Options System
# ----------------------

DYNOS_INPUT_DIR := ./dynos
DYNOS_OUTPUT_DIR := $(DIST_DIR)/moonos
DYNOS_PACKS_DIR := $(DIST_DIR)/moonos/packs
DYNOS_INIT := \
    mkdir -p $(DYNOS_INPUT_DIR); \
    mkdir -p $(DYNOS_OUTPUT_DIR); \
    mkdir -p $(DYNOS_PACKS_DIR);

DYNOS_DO := $(shell $(call DYNOS_INIT))
INCLUDE_CFLAGS += -DDYNOS
