add_library(zion_registry STATIC
    ModuleRegistry.cc
    AdapterRegistry.cc
    # ── Real adapters (35 HAL-backed modules) ────────────────────────────
    adapters/audio_interfaces_adapter.cc
    adapters/camera_interfaces_adapter.cc
    adapters/camera_module3_adapter.cc
    adapters/clock_and_timing_adapter.cc
    adapters/communication_interface_adapter.cc
    adapters/cpu_adapter.cc
    adapters/debug_and_development_interfaces_adapter.cc
    adapters/deep_learning_accelerator_adapter.cc
    adapters/display_interfaces_adapter.cc
    adapters/environmental_adapter.cc
    adapters/ethernet_adapter.cc
    adapters/ethernet_switch_adapter.cc
    adapters/external_connector_adapter.cc
    adapters/functional_safety_adapter.cc
    adapters/gnss_adapter.cc
    adapters/gpio_adapter.cc
    adapters/gpu_adapter.cc
    adapters/imu_adapter.cc
    adapters/mcu_interface_adapter.cc
    adapters/pcie_adapter.cc
    adapters/poe_hat_adapter.cc
    adapters/power_management_adapter.cc
    adapters/reliability_adapter.cc
    adapters/security_features_adapter.cc
    adapters/sense_hat_adapter.cc
    adapters/serializer_deserializer_adapter.cc
    adapters/software_interface_adapter.cc
    adapters/storage_adapter.cc
    adapters/stub_adapter.cc
    adapters/system_integration_adapter.cc
    adapters/system_memory_adapter.cc
    adapters/thermal_adapter.cc
    adapters/usb_adapter.cc
    adapters/video_encode_decode_adapter.cc
    adapters/vision_accelerator_adapter.cc
    adapters/watchdog_and_reset_adapter.cc
    # ── Phase 3 — 16 new real adapters (DISABLED: HAL libs not yet implemented) ──
    # adapters/accelerometer_adapter.cc
    # adapters/analog_input_adapter.cc
    # adapters/battery_adapter.cc
    # adapters/bluetooth_adapter.cc
    # adapters/can_bus_adapter.cc
    # adapters/emmc_adapter.cc
    # adapters/i2c_adapter.cc
    # adapters/mipi_csi_adapter.cc
    # adapters/mipi_dsi_adapter.cc
    # adapters/nvme_adapter.cc
    # adapters/pwm_output_adapter.cc
    # adapters/rs485_adapter.cc
    # adapters/rtc_adapter.cc
    # adapters/sd_card_adapter.cc
    # adapters/spi_adapter.cc
    # adapters/uart_adapter.cc
    # ── Honest stub for unrecognised modules ─────────────────────────────
)

target_include_directories(zion_registry PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../Zion-HAL/src
)

# acu_full_test is the combined HAL static library built by add_subdirectory
# in the root CMakeLists.  PUBLIC propagates HAL includes to zion-client.
target_link_libraries(zion_registry PUBLIC
    acu_full_test
)
target_link_libraries(zion_registry PRIVATE
    zion_protos
    yaml-cpp::yaml-cpp
    spdlog::spdlog
)
