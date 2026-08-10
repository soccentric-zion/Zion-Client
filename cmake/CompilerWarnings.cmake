# Compiler warning flags for zion-client targets.
# Warnings-as-errors is opt-in so local dev builds don't fail on new warnings,
# but CI can set -DWARNINGS_AS_ERRORS=ON.

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(set_compiler_warnings target)
  set(flags
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Woverloaded-virtual
    -Wnull-dereference
  )

  if(WARNINGS_AS_ERRORS)
    list(APPEND flags -Werror)
  endif()

  target_compile_options(${target} PRIVATE ${flags})
endfunction()
