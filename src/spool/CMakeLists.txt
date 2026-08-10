add_library(zion_spool STATIC Spool.cc
) target_include_directories(zion_spool PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(zion_spool PRIVATE zion_protos SQLite::SQLite3 spdlog::spdlog
)
