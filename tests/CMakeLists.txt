find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_rules
    test_rules.cpp
    ${CMAKE_SOURCE_DIR}/src/core/RuleGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/RuleParser.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Types.h
)

target_include_directories(test_rules PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(test_rules PRIVATE Qt6::Core Qt6::Test)

add_test(NAME test_rules COMMAND test_rules)
