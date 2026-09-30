add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE odelab)
add_test(NAME core_tests COMMAND test_core)

add_executable(test_phase_line test_phase_line.cpp)
target_link_libraries(test_phase_line PRIVATE odelab)
add_test(NAME phase_line_tests COMMAND test_phase_line)

add_executable(test_expression test_expression.cpp)
target_link_libraries(test_expression PRIVATE odelab)
add_test(NAME expression_tests COMMAND test_expression)

add_executable(test_direction_field test_direction_field.cpp)
target_link_libraries(test_direction_field PRIVATE odelab)
add_test(NAME direction_field_tests COMMAND test_direction_field)

add_executable(test_hardening test_hardening.cpp)
target_link_libraries(test_hardening PRIVATE odelab)
add_test(NAME hardening_tests COMMAND test_hardening)
