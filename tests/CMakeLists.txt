# Unit tests for the pure-logic parts of the backend (no device or UI needed).
# Enable with: cmake -B build -DBUILD_TESTS=ON && cmake --build build && ctest --test-dir build

function(qmon_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE qmonstatek_backend Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

qmon_add_test(tst_crc16)
qmon_add_test(tst_rpc_frame)
qmon_add_test(tst_github_checker)
