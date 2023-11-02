# 'Google_test' is the subproject name
project(Google_tests)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

find_package(Protobuf REQUIRED)

find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)
set(ICU_LIBS ICU::uc ICU::i18n)

find_library(libldap NAMES libldap)

# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests
add_executable(Google_Tests_run test-measure-unit.cpp
    ../MeasureUnit.cpp
    ../StockOperation.cpp
    ../RCQuery.cpp
    ../string-helper.cpp
    ../QueryProperties.cpp
    ../BoxName.cpp
    ../example-plugins/login-ad.cpp
)
target_link_libraries(Google_Tests_run gtest gtest_main ${ICU_LIBS} ${Protobuf_LIBRARIES} ldap)
