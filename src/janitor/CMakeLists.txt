add_library(zion_janitor STATIC Janitor.cc
) target_include_directories(zion_janitor PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(zion_janitor PRIVATE zion_protos spdlog::spdlog
)
