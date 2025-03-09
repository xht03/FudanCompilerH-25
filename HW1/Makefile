RM       = rm -rf
MAKEFLAGS = --no-print-directory

BUILD_DIR = $(CURDIR)/build
TEST_DIR = $(CURDIR)/test
TEST_EXTRA_DIR = $(CURDIR)/test/extra
MAIN_EXE = $(BUILD_DIR)/tools/main/main
TEST_EXE = $(BUILD_DIR)/tools/test/test

.PHONY: build clean veryclean rebuild test handin test-extra

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@$(RM) $(TEST_DIR)/*.ast

veryclean: clean 
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(TEST_EXE) "$${file%%.*}"; \
		fi \
	done

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw1.zip" \
	  docs/report.pdf include lib

test-extra: clean
	@cd $(TEST_EXTRA_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(TEST_EXE) "$${file%%.*}"; \
		fi \
	done

.PHONY: run run-one

run: clean
	cd $(TEST_DIR) && \
	for file in $$(ls .); do \
        if [ "$${file##*.}" = "fmj" ]; then \
            echo "Parsing $${file%%.*}"; \
			$(MAIN_EXE) "$${file%%.*}"; \
			echo "Comparing parsed write-out with load+then-write-out"; \
			diff "$${file%%.*}.2.ast" "$${file%%.*}.2-debug.ast" ; \
			echo "Comparing parsed write-out with load+then-clone-then-write-out"; \
			diff "$${file%%.*}.2.ast" "$${file%%.*}.2-debug3.ast" ; \
			echo "Comparing parsed write-out with load+then-clone-then-minusConst-converted-write-out"; \
			diff "$${file%%.*}.2.ast" "$${file%%.*}.2-debug4.ast" ; \
        fi \
	done; \
	cd .. > /dev/null 2>&1 

FILE = test1

run-one: clean
	cd $(CURDIR)/test && \
	$(MAIN_EXE) ${FILE} ; \
	echo "Parsing ${FILE}.fmj"; \
	$(MAIN_EXE) "${FILE}"; \
	echo "Comparing parsed write-out with load+then-write-out"; \
	diff "${FILE}.2.ast" "${FILE}.2-debug.ast" ; \
	echo "Comparing parsed write-out with load+then-clone-then-write-out"; \
	diff "${FILE}.2.ast" "${FILE}.2-debug3.ast" ; \
	echo "Comparing parsed write-out with load+then-clone-then-minusConst-converted-write-out"; \
	diff "${FILE}.2.ast" "${FILE}.2-debug4.ast" ; \
	cd .. > /dev/null 2>&1 
