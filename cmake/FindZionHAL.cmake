# FindZionHAL.cmake — consumed by Zion-Client CMakeLists.txt.
#
# The HAL is built as a sibling directory via:
#   add_subdirectory(../Zion-HAL ${CMAKE_BINARY_DIR}/zion_hal_build EXCLUDE_FROM_ALL)
# This creates the `acu_full_test` static library target (all HAL modules combined).
#
# Consumers link with:
#   target_link_libraries(<target> PRIVATE acu_full_test)
#
# Includes (propagated automatically via PUBLIC include on acu_full_test):
#   #include "lib_base.h"
#   #include "cpu_lib/cpu_lib.h"
#   #include "thermal_management_lib/thermal_management_lib.h"
#   … (pattern: <module_dir>/<module_header>.h)
#
# Phase 3 note: acu_full_test uses file(GLOB_RECURSE src/*.cpp) which will be
# replaced by an explicit source list in Phase 3 of the remediation plan.
