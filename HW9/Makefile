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
	$(RM) test/*5.* \

rebuild: clean build

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	filename="22307110206-胥昊天"
	zip -q -r "docs/$$filename-hw9.zip" \
	  docs/report.pdf include lib

.PHONY: run run-one gentests gentest-one patchdemo 

GENAST = $(CURDIR)/../HW5/build/tools/generatetests/gentests
GENIR = $(CURDIR)/../HW5/build/tools/main/main
GENQUAD = $(CURDIR)/../HW6/build/tools/main/main
GENSSA = $(CURDIR)/../HW7/build/tools/main/main
MAIN = $(BUILD_DIR)/tools/main/main
GENTESTS = $(BUILD_DIR)/tools/generatetests/gentests
PATCHDEMO = $(BUILD_DIR)/tools/patchdemo/patchdemo

define run_test
	cd $(CURDIR)/test/$(1) && \
	for file in $$(ls .); do \
		if [ "$${file#*.}" = "4-prepared-xml.quad" ]; then \
			echo "Reading $${file}"; \
			$(MAIN) "$${file%%.*}"; \
		fi; \
	done; \
	cd .. > /dev/null 2>&1
endef

run: $(MAIN)
	$(call run_test,input_example)

run-assem: $(MAIN)
	cd $(CURDIR)/test && \
	for file in $$(ls .); do \
		if [ "$${file#*.}" = "4-prepared-xml.quad" ]; then \
			echo "Reading ${file}.4-prepared-xml.quad and ${file}.4-xml.clr"; \
			arm-linux-gnueabihf-gcc -mcpu=cortex-a72 -Wall -Wextra --static -o "$${file%%.*}" "$${file%%.*}.s" ../vendor/libsysy/libsysy32.s -lm; \
			echo "Running the final assembly program........." ; \
			qemu-arm -B 0x1000 $${file%%.*}; \
		fi; \
	done; \
	cd .. > /dev/null 2>&1 

FILE=test1
