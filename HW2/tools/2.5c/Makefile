.PHONY: build clean test

BUILD_DIR = ./build
TEST_DIR = ../../test/2.5c

build:
	@mkdir -p $(BUILD_DIR)
	@flex -o $(BUILD_DIR)/lex.yy.c main.l
	@gcc -o $(BUILD_DIR)/main.out $(BUILD_DIR)/lex.yy.c -lfl

clean:
	@rm -rf $(BUILD_DIR)

test: build
	@echo "2.5c Testing:"
	@for file in $(TEST_DIR)/*.txt; do \
        echo "Testing $$(basename $$file):"; \
        $(BUILD_DIR)/main.out < "$$file"; \
		echo "-----------------------------------"; \
    done