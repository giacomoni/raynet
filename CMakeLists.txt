cmake_policy(SET CMP0048 NEW)
project(rlomnetbind VERSION 1.0.0 LANGUAGES CXX)

cmake_minimum_required(VERSION 3.4...3.18)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_DEBUG_POSTFIX _dbg)


set(INET_MODEL_PATH $ENV{HOME}/inet/src)
set(CUSTOM_MODEL_PATH $ENV{HOME}/RLlibIntegration/model)
set(ECMP_PATH $ENV{HOME}/ecmp/src)
set(RDP_PATH $ENV{HOME}/rdp/src)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


# Declare Release and Debug libraries
set(MY_LIB_LINK_LIBRARIES_RELEASE optimized oppsim optimized oppcommon optimized oppcmdenv optimized cmdrlenv optimized oppnedxml optimized oppenvir optimized -Wl,--no-as-needed INET optimized -Wl,--no-as-needed model optimized -Wl,--no-as-needed rdp optimized -Wl,--no-as-needed ecmp)
set(MY_LIB_LINK_LIBRARIES_DEBUG debug oppsim_dbg debug oppcommon_dbg debug oppcmdenv_dbg debug cmdrlenv debug oppnedxml_dbg debug oppenvir_dbg debug -Wl,--no-as-needed INET_dbg debug -Wl,--no-as-needed model_dbg debug -Wl,--no-as-needed rdp_dbg debug -Wl,--no-as-needed ecmp_dbg)


find_package(OmnetPP 5.6.2 REQUIRED)

include(OmnetppHelpers)

add_compile_options(-fpermissive -fno-omit-frame-pointer -fPIC -MMD -MP -MF)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/extern/pybind11/include ${OMNETPP_INCLUDE_DIR} ${OMNETPP_ROOT}/include ${OMNETPP_ROOT}/src /usr/include/python3.8 ${CUSTOM_MODEL_PATH})
link_directories(${OMNETPP_ROOT}/lib ${INET_MODEL_PATH} ${CUSTOM_MODEL_PATH} ${ECMP_PATH} ${RDP_PATH})

add_subdirectory(pybind11)

add_compile_definitions(WITH_PARSIM HAVE_SWAPCONTEXT XMLPARSER=libxml WITH_NETBUILDER)

add_library(rlomnet SHARED ${PROJECT_SOURCE_DIR}/src/SimulationRunner.cc)
add_library(cmdrlenv SHARED ${PROJECT_SOURCE_DIR}/src/cmdrlenv.cc)

if(CMAKE_BUILD_TYPE STREQUAL Release)


    pybind11_add_module(omnetbind MODULE ${PROJECT_SOURCE_DIR}/src/SimulationRunnerBind.cc)

    target_link_libraries(cmdrlenv oppcmdenv)
    target_link_libraries(omnetbind PRIVATE ${MY_LIB_LINK_LIBRARIES_RELEASE})
    target_link_libraries(rlomnet PRIVATE ${MY_LIB_LINK_LIBRARIES_RELEASE})
    target_link_libraries(omnetbind PRIVATE rlomnet)
    add_executable(testcpp ${PROJECT_SOURCE_DIR}/src/cpptest.cc)
    set_target_properties(testcpp PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    target_link_libraries(testcpp rlomnet oppsim)


elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    set_target_properties(cmdrlenv PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    target_link_libraries(cmdrlenv oppcmdenv_dbg)
  
    target_link_libraries(rlomnet PRIVATE ${MY_LIB_LINK_LIBRARIES_DEBUG})

    add_executable(testcpp ${PROJECT_SOURCE_DIR}/src/cpptest.cc)
    set_target_properties(testcpp PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    target_link_libraries(testcpp rlomnet oppsim_dbg)


endif()


