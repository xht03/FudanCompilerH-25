.PHONY: build clean handin compile run run-one

BUILD_DIR = $(CURDIR)/build

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean: 
	@$(RM) build ; \
	cd $(CURDIR)/test && \
    for file in $$(ls .); do \
        if [ "$${file##*.}" != "fmj" ]; then \
            echo "Removing $$file"; \
            rm -f "$$file"; \
        fi; \
    done; \
    cd .. > /dev/null 2>&1

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	filename="22307110206-胥昊天"; \
	zip -q -r "docs/$$filename-final.zip" \
	  docs/report.pdf include lib 

MAIN = $(BUILD_DIR)/tools/main/main

compile: build
	cd $(CURDIR)/test && \
	for file in $$(ls .); do \
        if [ "$${file##*.}" = "fmj" ]; then \
            echo "Compiling $${file}"; \
			$(MAIN) "$${file%%.*}"; \
			arm-linux-gnueabihf-gcc -mcpu=cortex-a72 -Wall -Wextra --static -o "$${file%%.*}" "$${file%%.*}.4.s" ../vendor/libsysy/libsysy32.s -lm; \
        fi \
	done; \
	cd .. > /dev/null 2>&1

run: compile
	cd $(CURDIR)/test && \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "s" ]; then \
			echo "Running the program $${file} ........."; \
			qemu-arm -B 0x1000 "$${file%%.*}"; \
		fi; \
	done; \
	cd .. > /dev/null 2>&1

run-one: compile
	@if [ -z "$(PROG)" ]; then \
		echo "Usage: make run-one PROG=<program_name> (e.g., make run-one PROG=hw10test00)"; \
		exit 1; \
	fi
	@cd $(CURDIR)/test && \
	if [ -f "$(PROG)" ]; then \
		echo "Running $(PROG) ..."; \
		qemu-arm -B 0x1000 "$(PROG)" || true; \
	else \
		echo "Error: $(PROG) not found in test directory."; \
		exit 1; \
	fi
