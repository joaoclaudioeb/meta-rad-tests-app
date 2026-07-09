BUILD_DIR = build
MAKEFLAGS += --no-print-directory

.Phony:
all: meson.build
	@meson setup $(BUILD_DIR)
	@meson compile -C $(BUILD_DIR)

.Phony:
clean: meson.build
	@ninja clean -C $(BUILD_DIR)

.Phony:
install: meson.build install.sh
	@sudo ./install.sh

.Phony:
format:
	@find src include -name '*.cpp' -o -name '*.hpp' -o -name '*.c' -o -name '*.h' | xargs clang-format -i
