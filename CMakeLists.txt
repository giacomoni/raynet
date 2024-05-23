cmake_policy(SET CMP0048 NEW)
project(rlomnetbind VERSION 1.0.0 LANGUAGES CXX)

cmake_minimum_required(VERSION 3.4...3.18)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_DEBUG_POSTFIX _dbg)


set(INET_MODEL_PATH $ENV{HOME}/inet4.5/src)
set(TCPPACED_PATH ${PROJECT_SOURCE_DIR}/simlibs/TcpPaced/src)
set(RLCOMPONENTS_PATH ${PROJECT_SOURCE_DIR}/simlibs/RLComponents/src)
set(CARTPOLE_PATH ${PROJECT_SOURCE_DIR}/simlibs/cartpole/src)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


# Declare Release and Debug libraries
set(MY_LIB_LINK_LIBRARIES_RELEASE optimized oppsim optimized oppcommon optimized oppcmdenv optimized Cmdrlenv optimized oppnedxml optimized oppenvir optimized -Wl,--no-as-needed INET optimized -Wl,--no-as-needed TcpPaced optimized -Wl,--no-as-needed RLComponents optimized -Wl,--no-as-needed cartpole)
set(MY_LIB_LINK_LIBRARIES_DEBUG debug oppsim_dbg debug oppcommon_dbg debug oppcmdenv_dbg debug Cmdrlenv debug oppnedxml_dbg debug oppenvir_dbg debug -Wl,--no-as-needed INET_dbg debug -Wl,--no-as-needed TcpPaced_dbg debug -Wl,--no-as-needed TcpPaced_dbg debug -Wl,--no-as-needed RLComponents_dbg debug -Wl,--no-as-needed cartpole_dbg)


find_package(OmnetPP 6.0.1 REQUIRED)

include(OmnetppHelpers)

add_compile_options(-fpermissive -fno-omit-frame-pointer -fPIC -MMD -MP -MF )
add_compile_definitions($ENV{RAYNET_FEATURE} HAVE_SWAPCONTEXT  WITH_NETBUILDER)
include_directories(/usr/include/python3.8 ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/extern/pybind11/include ${OMNETPP_INCLUDE_DIR} ${OMNETPP_ROOT}/include ${OMNETPP_ROOT}/src ${TCPPACED_PATH}  ${RLCOMPONENTS_PATH} ${CARTPOLE_PATH})
link_directories(${OMNETPP_ROOT}/lib ${INET_MODEL_PATH} ${TCPPACED_PATH} ${RLCOMPONENTS_PATH} ${CARTPOLE_PATH})


add_library(rlomnet SHARED ${PROJECT_SOURCE_DIR}/src/GymApi.cc)
add_library(Cmdrlenv SHARED ${PROJECT_SOURCE_DIR}/src/Cmdrlenv.cc)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_subdirectory(pybind11)


    pybind11_add_module(omnetbind MODULE ${PROJECT_SOURCE_DIR}/src/GymApiBind.cc)

    target_link_libraries(Cmdrlenv oppcmdenv)
    target_link_libraries(omnetbind PRIVATE ${MY_LIB_LINK_LIBRARIES_RELEASE})
    target_link_libraries(rlomnet PRIVATE ${MY_LIB_LINK_LIBRARIES_RELEASE})
    target_link_libraries(omnetbind PRIVATE rlomnet)

    add_executable(testcpp ${PROJECT_SOURCE_DIR}/src/gymApiTest.cc)
    target_link_libraries(testcpp rlomnet oppsim)


elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    set_target_properties(Cmdrlenv PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    target_link_libraries(Cmdrlenv oppcmdenv_dbg)
  
    target_link_libraries(rlomnet PRIVATE ${MY_LIB_LINK_LIBRARIES_DEBUG})

    add_executable(testcpp ${PROJECT_SOURCE_DIR}/src/gymApiTest.cc)
    set_target_properties(testcpp PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    target_link_libraries(testcpp rlomnet oppsim_dbg)


endif()


