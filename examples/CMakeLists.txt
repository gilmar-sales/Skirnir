add_executable(SimpleInjection simple_injection/main.cpp)
add_executable(MultiImpl multi_impl/main.cpp)
add_executable(OptionsBinding options_binding/main.cpp)

target_link_libraries(SimpleInjection skirnir::skirnir)
target_link_libraries(MultiImpl skirnir::skirnir)
target_link_libraries(OptionsBinding skirnir::skirnir)
