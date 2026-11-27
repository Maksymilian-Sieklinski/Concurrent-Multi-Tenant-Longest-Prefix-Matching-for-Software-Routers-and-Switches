add_library(generator STATIC generator.cpp installed_prefixes.cpp operation_generator.cpp sampler.cpp)
target_link_libraries(generator PUBLIC core PRIVATE project_compilation_options)
