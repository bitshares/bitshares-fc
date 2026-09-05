if( ECC_IMPL STREQUAL secp256k1 )
    add_executable( blind all_tests.cpp crypto/blind.cpp )
    target_link_libraries( blind fc )
endif()

add_executable( task_cancel_test all_tests.cpp thread/task_cancel.cpp )
target_link_libraries( task_cancel_test fc )


add_executable( bloom_test all_tests.cpp bloom_test.cpp )
target_link_libraries( bloom_test fc )

add_executable( hmac_test hmac_test.cpp )
target_link_libraries( hmac_test fc )

add_executable( ecc_test crypto/ecc_test.cpp )
target_link_libraries( ecc_test fc )

add_executable( ws_test_server ws_test_server.cpp )
target_link_libraries( ws_test_server fc )

add_executable( ws_test_client ws_test_client.cpp )
target_link_libraries( ws_test_client fc )

#add_executable( test_aes aes_test.cpp )
#target_link_libraries( test_aes fc ${rt_library} ${pthread_library} )
#add_executable( test_sleep sleep.cpp )
#target_link_libraries( test_sleep fc )
#add_executable( test_rate_limiting rate_limiting.cpp )
#target_link_libraries( test_rate_limiting fc )

add_executable( all_tests all_tests.cpp
                          compress/compress.cpp
                          crypto/aes_test.cpp
                          crypto/array_initialization_test.cpp
                          crypto/base_n_tests.cpp
                          crypto/bigint_test.cpp
                          crypto/blind.cpp
                          crypto/rand_test.cpp
                          crypto/sha_tests.cpp
                          io/json_tests.cpp
                          io/stream_tests.cpp
                          io/tcp_test.cpp
                          io/varint_tests.cpp
                          network/ip_tests.cpp
                          network/http/websocket_test.cpp
                          thread/task_cancel.cpp
                          thread/thread_tests.cpp
                          thread/parallel_tests.cpp
                          bloom_test.cpp
                          reflection_tests.cpp
                          safe_tests.cpp
                          serialization_test.cpp
                          stacktrace_test.cpp
                          time_test.cpp
                          utf8_test.cpp
                          variant_test.cpp
                          logging_tests.cpp
                          api_tests.cpp
                          )
target_link_libraries( all_tests fc )
