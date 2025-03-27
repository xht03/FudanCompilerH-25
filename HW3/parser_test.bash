#!/bin/bash
# filepath: ~/FudanCompilerH2025/HW3/parser_test.bash

# 检查build文件夹是否存在，不存在则创建
if [ ! -d "build" ]; then
    echo "Build directory does not exist. Creating it..."
    mkdir -p build
fi

# 检查输出目录是否存在
if [ -d "../test/my_output" ]; then
    echo "Output directory exists. Cleaning it..."
    rm -f ../test/my_output/*.ast  # 删除所有.ast文件
else
    echo "Output directory does not exist. Creating it..."
    mkdir -p ../test/my_output
fi

cd build
cmake ..
make

# 如果编译成功，运行测试
if [ $? -eq 0 ]; then
    echo "Build successful. Running parser tests..."

    for fmj_file in ../test/*.fmj; do
        if [ -f "$fmj_file" ]; then
            filename="${fmj_file%.fmj}"  # 移除.fmj后缀
            base_name=$(basename "$fmj_file" .fmj)
            echo "Testing file: $base_name"
            
            # 运行测试程序
            ./tools/main/main "$filename"

            # 移动生成的.ast文件到my_output目录
            if [ -f "${filename}.2.ast" ]; then
                mv "${filename}.2.ast" "../test/my_output/${base_name}.2.ast"
                
            #     # 比较结果
            #     if [ -f "../test/output_example/${base_name}.2.ast" ]; then
            #         echo "Comparing with example output..."
            #         diff -u "../test/my_output/${base_name}.2.ast" "../test/output_example/${base_name}.2.ast"
                    
            #         if [ $? -eq 0 ]; then
            #             echo "Test PASS for $base_name (.2.ast)"
            #         else
            #             echo "Test FAIL for $base_name (.2.ast)"
            #         fi
            #     else
            #         echo "Warning: No example output found for $base_name (.2.ast)"
            #     fi
            # else
            #     echo "Error: No output generated for $base_name (.2.ast)"
            fi
            
            # 处理带语义信息的AST文件
            if [ -f "${filename}.2-semant.ast" ]; then
                mv "${filename}.2-semant.ast" "../test/my_output/${base_name}.2-semant.ast"
                
            #     # 比较结果
            #     if [ -f "../test/output_example/${base_name}.2-semant.ast" ]; then
            #         echo "Comparing with example output (semantic)..."
            #         diff -u "../test/my_output/${base_name}.2-semant.ast" "../test/output_example/${base_name}.2-semant.ast"
                    
            #         if [ $? -eq 0 ]; then
            #             echo "Test PASS for $base_name (.2-semant.ast)"
            #         else
            #             echo "Test FAIL for $base_name (.2-semant.ast)"
            #         fi
            #     else
            #         echo "Warning: No example output found for $base_name (.2-semant.ast)"
            #     fi
            # else
            #     echo "Note: No semantic output generated for $base_name (.2-semant.ast)"
            fi
            
            echo "----------------------------------------"
        fi
    done
    

else
    echo "Build failed. Please check the errors above."
fi

echo "Test completed."