add_library(zion_diag STATIC DiagSocket.cc
) target_include_directories(zion_diag PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(zion_diag PRIVATE spdlog::spdlog
)
