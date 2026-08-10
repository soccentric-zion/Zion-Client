# ---------------------------------------------------------------------------
# src/CMakeLists.txt
# Adds each module as a static library then defines the main executable.
# Subdirectories are ordered leaf-first so inter-target references resolve.
# --------------------------------------------------------------------------- add_subdirectory(config)
add_subdirectory(diag)
add_subdirectory(spool)
add_subdirectory(stream)
add_subdirectory(hal_bridge)
add_subdirectory(registry)
add_subdirectory(ota)
add_subdirectory(log)
add_subdirectory(janitor)
add_subdirectory(run)
add_subdirectory(hal_service) # ---------------------------------------------------------------------------
# Main executable
# --------------------------------------------------------------------------- add_executable(zion-client main.cpp app.cpp
) target_include_directories(zion-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/../Zion-HAL/src
) target_link_libraries(zion-client PRIVATE zion_config zion_diag zion_hal_bridge zion_hal_service zion_janitor zion_log zion_ota zion_registry zion_run zion_spool zion_stream zion_protos spdlog::spdlog tomlplusplus::tomlplusplus yaml-cpp::yaml-cpp SQLite::SQLite3 ${ZION_GRPC_LIBRARIES} protobuf::libprotobuf
) target_compile_definitions(zion-client PRIVATE ZION_CLIENT_VERSION="${PROJECT_VERSION}"
) set_compiler_warnings(zion-client)
