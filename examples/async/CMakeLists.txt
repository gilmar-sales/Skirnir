add_executable(AsyncTimer timer/main.cpp)
target_link_libraries(AsyncTimer skirnir::skirnir)
add_executable(AsyncScope scope/main.cpp)
target_link_libraries(AsyncScope skirnir::skirnir)