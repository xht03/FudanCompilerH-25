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
	$(RM) test/HW3/*.4.quad test/HW4/*.4.quad test/Other/*.4.quad ; \

rebuild: clean build

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	filename="22300240008-曹奕伦"; \
	zip -q -r "docs/$$filename-hw5.zip" \
	  docs/report.pdf include lib

.PHONY: run run-one

MAIN = $(BUILD_DIR)/tools/main/main
PATCHDEMO = $(BUILD_DIR)/tools/patchdemo/patchdemo

run: $(MAIN)
	cd $(CURDIR)/test && \
	for file in $$(ls .); do \
        if [ "$${file##*.}" = "fmj" ]; then \
            echo "Reading $${file%%.*}"; \
			$(MAIN) "$${file%%.*}"; \
        fi \
	done; \
	cd .. > /dev/null 2>&1 

define run_test
	cd $(CURDIR)/test/$(1) && \
	for file in $$(ls .); do \
		if [ "$${file#*.}" = "4-xml.quad" ]; then \
			echo "Reading $${file}"; \
			$(MAIN) "$${file%%.*}"; \
		fi; \
	done; \
	cd .. > /dev/null 2>&1
endef

# run: $(MAIN)
# 	$(call run_test, HW3)
# 	$(call run_test,HW4)
# 	$(call run_test,Other)

run-assem: $(MAIN)
	cd $(CURDIR)/test && \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "s" ]; then \
			echo "Processing assembly file: $$file"; \
			echo "Compiling $${file}"; \
			arm-linux-gnueabihf-gcc -mcpu=cortex-a72 -Wall -Wextra --static -o "$${file%%.*}" "$$file" ../vendor/libsysy/libsysy32.s -lm; \
			echo "Running the final assembly program........."; \
			qemu-arm -B 0x1000 "$${file%%.*}"; \
		fi; \
	done; \
	cd .. > /dev/null 2>&1 