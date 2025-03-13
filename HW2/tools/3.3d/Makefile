.PHONY: build clean test

BUILD_DIR = ./build
TEST_DIR = ../../test/3.3d

build:
	@mkdir -p $(BUILD_DIR)
	@bison -d -o $(BUILD_DIR)/main.tab.c main.y -Wcounterexamples
	@flex -o $(BUILD_DIR)/lex.yy.c main.l
	@gcc -o $(BUILD_DIR)/main.out $(BUILD_DIR)/main.tab.c $(BUILD_DIR)/lex.yy.c -lfl

clean:
	@rm -rf $(BUILD_DIR)

test: build
	@for file in $(TEST_DIR)/*.txt; do \
        echo "Testing $$(basename $$file):"; \
        $(BUILD_DIR)/main.out < "$$file"; \
		echo "-----------------------------------"; \
    done