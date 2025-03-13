.PHONY: build clean test

BUILD_DIR = ./build
TEST_DIR = ../../test/3.12a

build:
	@mkdir -p $(BUILD_DIR)
	@g++ -o $(BUILD_DIR)/main.out main.cc

clean:
	@rm -rf $(BUILD_DIR)

test: build
	@for file in $(TEST_DIR)/*.txt; do \
		echo "Testing $$(basename $$file):"; \
		$(BUILD_DIR)/main.out < "$$file"; \
		echo "-----------------------------------"; \
	done