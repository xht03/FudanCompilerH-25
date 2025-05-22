RM       = rm -rf
MAKEFLAGS = --no-print-directory

.PHONY: build clean rebuild handin

BUILD_DIR = $(CURDIR)/build

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja
# cd $(BUILD_DIR) && ninja -v

clean: 
	@$(RM) build ; \
	$(RM) test/*3*; \
	$(RM) test/*4.quad; \
	$(RM) test/*4-xml.quad; \
	$(RM) test/*4-ssa.quad; \
	$(RM) test/*4-ssa2.quad; \
	$(RM) test/*4-ssa-back.quad; \
	$(RM) test/*4-block.quad; \
	$(RM) test/*4-prepared.quad; \
	$(RM) test/*4-alloced.quad; \
	$(RM) test/*4-rpi.quad; \
	$(RM) test/*4-rpi-xml.quad; 

rebuild: clean build

deepclean: clean 
	${RM} test/*.4.quad

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw8.zip" \
	  docs/report.pdf include lib

.PHONY: run run-one gentests gentest-one patchdemo 

GENAST = $(CURDIR)/../HW5/build/tools/generatetests/gentests
GENIR = $(CURDIR)/../HW5/build/tools/main/main
GENQUAD = $(CURDIR)/../HW6/build/tools/main/main
GENSSA = $(CURDIR)/../HW7/build/tools/main/main
MAIN = $(BUILD_DIR)/tools/main/main
GENTESTS = $(BUILD_DIR)/tools/generatetests/gentests
PATCHDEMO = $(BUILD_DIR)/tools/patchdemo/patchdemo

run: $(MAIN)
	@mkdir -p $(CURDIR)/test/my_output && \
	for file in $$(find $(CURDIR)/test/input_example -name "*.4-ssa-xml.quad"); do \
		filename=$$(basename "$$file" .4-ssa-xml.quad); \
		echo "Processing: $$filename"; \
		cd $(CURDIR) && $(MAIN) "$(CURDIR)/test/input_example/$$filename"; \
		mv -f "$(CURDIR)/test/input_example/$$filename.4-prepared.quad" "$(CURDIR)/test/my_output/" 2>/dev/null || true; \
		mv -f "$(CURDIR)/test/input_example/$$filename.4-prepared-xml.quad" "$(CURDIR)/test/my_output/" 2>/dev/null || true; \
		mv -f "$(CURDIR)/test/input_example/$$filename.4-xml.clr" "$(CURDIR)/test/my_output/" 2>/dev/null || true; \
	done; \
	echo "所有文件处理完成，输出已保存到 test/my_output 目录"



