aux_source_directory(dsa UTILS_SRCS)
aux_source_directory(printer UTILS_SRCS)
add_library(utils OBJECT ${UTILS_SRCS})