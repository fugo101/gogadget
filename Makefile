# Developer entry points. `make help` lists them. Firmware targets source the
# ESP-IDF v6.1 environment installed by EIM; override IDF_ACTIVATE if yours differs.
SHELL := /bin/bash
.DEFAULT_GOAL := help

IDF_ACTIVATE ?= $(HOME)/.espressif/tools/activate_idf_v6.1.sh
IDF := source "$(IDF_ACTIVATE)" >/dev/null && idf.py -C firmware
PORT ?=
PORT_ARG := $(if $(PORT),-p $(PORT),)

SIM_BUILD := sim/build
GOLDEN_BUILD := sim/build-golden
GOLDEN_DIR := ui/test/golden
C_SOURCES = $(shell git ls-files -co --exclude-standard -- '*.c' '*.h' | grep -v '^ui/fonts/')

.PHONY: help
help: ## List the targets
	@grep -hE '^[a-z][a-z-]+:.*## ' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  make %-12s %s\n", $$1, $$2}'

# ---- Firmware (ESP-IDF) -------------------------------------------------------

.PHONY: fw flash monitor fw-clean menuconfig
fw: ## Build the firmware
	$(IDF) build

flash: ## Flash the firmware (PORT=/dev/cu.usbmodemXXXX to pick a port)
	$(IDF) $(PORT_ARG) flash

monitor: ## Open the serial monitor (interactive; Ctrl+] to quit)
	$(IDF) $(PORT_ARG) monitor

menuconfig: ## Explore options; copy what you keep into firmware/sdkconfig.defaults
	$(IDF) menuconfig

fw-clean: ## Remove the firmware build and the generated sdkconfig
	rm -rf firmware/build firmware/sdkconfig firmware/sdkconfig.old

.PHONY: backup erase
backup: ## Save the whole 16 MB flash to backups/ (do this before the first flash of a board)
	@mkdir -p backups
	source "$(IDF_ACTIVATE)" >/dev/null && python -m esptool --chip esp32s3 $(PORT_ARG) -b 921600 \
		read-flash 0 0x1000000 backups/flash-$$(date +%Y%m%d-%H%M%S).bin

erase: ## Erase the whole flash (removes the old firmware and its stored Wi-Fi settings)
	$(IDF) $(PORT_ARG) erase-flash

# ---- Host: tests, golden screenshots, simulator --------------------------------

.PHONY: sim test golden viewer
sim: ## Configure and build the host tests (and the SDL viewer when SDL2 exists)
	cmake -S sim -B $(SIM_BUILD) -G Ninja >/dev/null
	cmake --build $(SIM_BUILD)

test: sim ## Run core unit tests and golden screenshot comparisons
	ctest --test-dir $(SIM_BUILD) --output-on-failure

golden: ## Re-create every golden screenshot (then review the PNG diff)
	rm -f $(GOLDEN_DIR)/*.png
	cmake -S sim -B $(GOLDEN_BUILD) -G Ninja -DGG_GOLDEN_UPDATE=ON -DGG_SIM_VIEWER=OFF \
		$(if $(wildcard $(SIM_BUILD)/_deps/lvgl-src),-DFETCHCONTENT_SOURCE_DIR_LVGL=$(CURDIR)/$(SIM_BUILD)/_deps/lvgl-src) \
		$(if $(wildcard $(SIM_BUILD)/_deps/unity-src),-DFETCHCONTENT_SOURCE_DIR_UNITY=$(CURDIR)/$(SIM_BUILD)/_deps/unity-src) >/dev/null
	cmake --build $(GOLDEN_BUILD) --target test_screens
	$(GOLDEN_BUILD)/test_screens >/dev/null
	@# The test module reports success even when it cannot write a reference.
	@missing=0; for n in $$(grep -o 'expect_screen("[^"]*")' ui/test/test_screens.c | sed 's/expect_screen("\(.*\)")/\1/'); do \
		[ -f "$(GOLDEN_DIR)/$$n.png" ] || { echo "golden: $$n.png was not written"; missing=1; }; done; \
		[ $$missing -eq 0 ] && echo "golden: references written to $(GOLDEN_DIR)"; exit $$missing

viewer: sim ## Run the interactive simulator (arrows/N = Next, Esc/B = Back, Enter = Enter, U = USB)
	$(SIM_BUILD)/gg_sim

# ---- Checks ------------------------------------------------------------------

.PHONY: format format-check secrets versions check fonts
format: ## Format C sources in place (clang-format 23, see .clang-format)
	clang-format -i $(C_SOURCES)

format-check: ## Fail if any C source is not formatted
	clang-format --dry-run -Werror $(C_SOURCES)

secrets: ## Fail if anything secret is tracked
	tools/check_secrets.sh

versions: ## Fail if firmware and simulator disagree on the LVGL version
	tools/check_versions.sh

check: format-check secrets versions test fw ## Everything CI runs

fonts: ## Regenerate ui/fonts from tools/fonts (needs Node.js)
	tools/fonts/generate.sh
