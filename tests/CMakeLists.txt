############################
# Tests
############################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -fexceptions")
string (REPLACE "-std=gnu++11" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


SET(Tests_SRCS
    tests/TestMain.cpp
    tests/abortutil.cpp
    tests/compositiontest.cpp
    tests/effectstest.cpp
    tests/groupstest.cpp
    tests/keyframetest.cpp
    tests/markertest.cpp
    tests/modeltest.cpp
    tests/regressions.cpp
    tests/snaptest.cpp
    tests/test_utils.cpp
    tests/timewarptest.cpp
    tests/treetest.cpp
    tests/trimmingtest.cpp
    PARENT_SCOPE
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${MLT_INCLUDE_DIR}
    ${MLTPP_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/external
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ../src
    )

