cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile the Plugin implementation as a shared library so the Hotswapper can
# reload it at runtime. The same translation unit is also linked into the host
# executable so the linker can satisfy the typeid/vtable references emitted by
# the SKR_TRACK template machinery in main.cpp; the DLL takes over at runtime
# via Skr_GetModuleInterface.
add_library(hot_swap_demo_plugin SHARED Plugin.cpp)
target_link_libraries(hot_swap_demo_plugin PRIVATE skirnir::skirnir)
target_compile_definitions(hot_swap_demo_plugin PRIVATE SKR_HOTSWAP_API_BUILDING=1)
set_target_properties(hot_swap_demo_plugin PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Make sure the runtime loader can find the freshly-built plugin.
set_target_properties(hot_swap_demo_plugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(hot_swap_demo main.cpp Plugin.cpp)
target_link_libraries(hot_swap_demo PRIVATE skirnir::skirnir)
