add_executable(test-payload payload_test.cpp)
target_link_libraries(test-payload PRIVATE monitor-core)
add_test(NAME payload COMMAND test-payload)

add_executable(test-stream-view stream_view_test.cpp)
target_link_libraries(test-stream-view PRIVATE monitor-core)
add_test(NAME stream_view COMMAND test-stream-view)

add_executable(test-broker-log broker_log_test.cpp)
target_link_libraries(test-broker-log PRIVATE monitor-core)
add_test(NAME broker_log COMMAND test-broker-log)
