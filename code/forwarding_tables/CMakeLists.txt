file(GLOB_RECURSE FORWARDING_TABLES_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_library(forwarding_tables STATIC ${FORWARDING_TABLES_SOURCES})
target_link_libraries(forwarding_tables PUBLIC core PRIVATE project_compilation_options)
