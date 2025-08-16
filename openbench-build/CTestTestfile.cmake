# CMake generated Testfile for 
# Source directory: /workspace
# Build directory: /workspace/openbench-build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(board_tests "/workspace/bin/test_board")
set_tests_properties(board_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;309;add_test;/workspace/CMakeLists.txt;0;")
add_test(fen_comprehensive_tests "/workspace/bin/test_fen_comprehensive")
set_tests_properties(fen_comprehensive_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;310;add_test;/workspace/CMakeLists.txt;0;")
add_test(fen_edge_cases_tests "/workspace/bin/test_fen_edge_cases")
set_tests_properties(fen_edge_cases_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;311;add_test;/workspace/CMakeLists.txt;0;")
add_test(board_safety_tests "/workspace/bin/test_board_safety")
set_tests_properties(board_safety_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;312;add_test;/workspace/CMakeLists.txt;0;")
add_test(en_passant_check_evasion_tests "/workspace/bin/test_en_passant_check_evasion")
set_tests_properties(en_passant_check_evasion_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;313;add_test;/workspace/CMakeLists.txt;0;")
