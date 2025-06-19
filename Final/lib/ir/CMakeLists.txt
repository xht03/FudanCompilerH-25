aux_source_directory(. IR_SRCS)
add_library(ir STATIC ${IR_SRCS})
target_link_libraries(ir PUBLIC ast)