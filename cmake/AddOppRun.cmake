#[==[.rst:
AddOppRun
---------

This module provides mechanisms to run OMNeT++ simulations conveniently without relying on the OMNeT++ IDE.

*The documentation of this module is still a stub!*
#]==]

include(CMakeParseArguments)
include(GetNedFolders)
set(_OPP_CMAKE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

find_program(GDB_COMMAND gdb DOC "GNU debugger")
find_program(VALGRIND_COMMAND valgrind DOC "Valgrind executable")
set(VALGRIND_FLAGS "--track-origins=yes" CACHE STRING "Flags passed to Valgrind for memcheck targets")
set(VALGRIND_EXEC_FLAGS "-u Cmdenv" CACHE STRING "Flags passed to executable run by Valgrind")
set(RUN_FLAGS "" CACHE STRING "Flags appended to run command (and debug)")
mark_as_advanced(GDB_COMMAND VALGRIND_COMMAND)

function(_get_opp_run_dependencies target output)
    set(dependencies "")
    get_target_property(target_type ${target} TYPE)
    if(${target_type} STREQUAL "SHARED_LIBRARY")
        get_target_property(is_opp_library ${target} OMNETPP_LIBRARY)
        if(is_opp_library)
            list(APPEND dependencies ${target})
        endif()
    endif()

    get_target_property(interface_libraries ${target} INTERFACE_LINK_LIBRARIES)
    foreach(interface_library IN LISTS interface_libraries)
        if(TARGET ${interface_library})
            _get_opp_run_dependencies(${interface_library} interface_dependencies)
            list(APPEND dependencies ${interface_dependencies})
        endif()
    endforeach()

    list(REMOVE_DUPLICATES dependencies)
    set(${output} ${dependencies} PARENT_SCOPE)
endfunction()

function(_get_opp_run_libraries target output)
    set(libraries "")
    _get_opp_run_dependencies(${target} dependencies)
    foreach(dependency IN LISTS dependencies)
        set(libraries -l $<TARGET_FILE:${dependency}> ${libraries})
    endforeach()
    set(${output} ${libraries} PARENT_SCOPE)
endfunction()

function(_build_opp_run_command)
    cmake_parse_arguments(args "" "TARGET;OUTPUT" "NED_FOLDERS" ${ARGN})

    # collect all NED folders for given target
    get_ned_folders(${args_TARGET} target_ned_folders)
    set(ned_folders "")
    foreach(ned_folder IN LISTS target_ned_folders args_NED_FOLDERS)
        set(ned_folders "${ned_folders};${ned_folder}")
    endforeach()
    if(ned_folders)
        string(SUBSTRING "${ned_folders}" 1 -1 ned_folders)
        string(REPLACE ";" "$<SEMICOLON>" ned_folders "${ned_folders}")
    endif()

    # select opp_run binary depending on build type
    if(NOT CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(opp_run ${OMNETPP_RUN_DEBUG})
    else()
        set(opp_run ${OMNETPP_RUN})
    endif()

    # build opp_run command depending on target type
    get_target_property(target_type ${args_TARGET} TYPE)
    if(${target_type} STREQUAL "EXECUTABLE")
        set(exec $<TARGET_FILE:${args_TARGET}> -n "${ned_folders}")
    elseif(${target_type} STREQUAL "SHARED_LIBRARY")
        # if the target is depending on libraries like INET, 
        # add them before the actual target so that they get loaded first by opp_run
        _get_opp_run_libraries(${args_TARGET} opp_libraries)
        set(exec ${opp_run} -n "${ned_folders}" ${opp_libraries} -l $<TARGET_FILE:${args_TARGET}>)
    else()
        _get_opp_run_libraries(${args_TARGET} opp_libraries)
        set(exec ${opp_run} -n "${ned_folders}" ${opp_libraries})
    endif()

    set(${args_OUTPUT} ${exec} PARENT_SCOPE)
endfunction()

#[==[.rst:
.. cmake:command:: add_opp_run

Add run and debug targets.
#]==]
function(add_opp_run name)
    set(options_args "")
    set(one_value_args "CONFIG;DEPENDENCY;WORKING_DIRECTORY")
    set(multi_value_args "NED_FOLDERS")
    cmake_parse_arguments(args "${options_args}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(SEND_ERROR "add_opp_run called with invalid arguments: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_CONFIG)
        set(config "${args_CONFIG}")
    else()
        set(config "omnetpp.ini")
    endif()

    if(args_DEPENDENCY)
        set(target "${args_DEPENDENCY}")
    else()
        set(target "artery")
    endif()

    if(args_WORKING_DIRECTORY)
        if(IS_ABSOLUTE "${args_WORKING_DIRECTORY}")
            set(working_directory "${args_WORKING_DIRECTORY}")
        else()
            set(working_directory "${CMAKE_CURRENT_SOURCE_DIR}/${args_WORKING_DIRECTORY}")
        endif()
    else()
        set(working_directory "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    _build_opp_run_command(TARGET ${target} OUTPUT exec NED_FOLDERS ${args_NED_FOLDERS})

    string(REPLACE " " ";" run_flags "${RUN_FLAGS}")
    add_custom_target(run_${name}
        COMMAND ${exec} ${config} ${run_flags}
        WORKING_DIRECTORY ${working_directory}
        VERBATIM)

    set_target_properties(run_${name} PROPERTIES
        OPP_RUN_TARGET ${target}
        OPP_RUN_CONFIG_FILE ${config}
        OPP_RUN_WORKING_DIRECTORY ${working_directory}
        OPP_RUN_NED_FOLDERS "${args_NED_FOLDERS}")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND GDB_COMMAND)
        add_custom_target(debug_${name}
            COMMAND ${GDB_COMMAND} --args ${exec} ${config} ${run_flags}
            WORKING_DIRECTORY ${working_directory}
            USES_TERMINAL
            VERBATIM)
    endif()

    get_property(vscode_addon GLOBAL PROPERTY VSCODE_ADDON)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND vscode_addon)
        # update_launch_json.py requires at least Python 3.4
        find_package(Python3 3.4 COMPONENTS Interpreter REQUIRED)

        # add build option for selecting debug configuration
        set(VSCODE_DEBUG_CONFIG "GDB" CACHE STRING "VSCode launch configurations (GDB or CodeLLDB)")

        if(NOT TARGET update_launch_json)
            add_custom_target(update_launch_json ALL DEPENDS update_launch_json.stamp)
        endif()

        add_custom_command(
            OUTPUT update_launch_json.stamp
            COMMAND ${Python3_EXECUTABLE} ${_OPP_CMAKE_BASE_DIR}/update_launch_json.py
                --gdb-command ${GDB_COMMAND}
                --omnetpp-root ${OMNETPP_ROOT}
                --debug-config ${VSCODE_DEBUG_CONFIG}
                --setup-commands ${PROJECT_SOURCE_DIR}/.vscode/omnetpp-debug-setup-commands.json
                ${PROJECT_SOURCE_DIR}/.vscode/launch.json
            COMMAND ${CMAKE_COMMAND} -E touch update_launch_json.stamp
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Updating launch.json of VSCode"
            VERBATIM)

        list(GET exec 0 _launch_exec)
        list(SUBLIST exec 1 -1 _launch_args)
        list(APPEND _launch_args ${config} ${run_flags})
        list(LENGTH _launch_args _launch_args_length)
        if(_launch_args_length GREATER 0)
            list(JOIN _launch_args "\", \"" _launch_args)
            set(_launch_args "\"${_launch_args}\"")
        else()
            set(_launch_args "")
        endif()
        file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/vscode-debug/${name}.json
            CONTENT "{\"working_directory\": \"${working_directory}\", \"exec\": \"${_launch_exec}\", \"args\": [${_launch_args}]}"
            CONDITION "$<CONFIG:Debug>")
    endif()

    if(VALGRIND_COMMAND)
        string(REPLACE " " ";" valgrind_flags "${VALGRIND_FLAGS}")
        string(REPLACE " " ";" valgrind_exec_flags "${VALGRIND_EXEC_FLAGS}")
        add_custom_target(memcheck_${name}
            COMMAND ${VALGRIND_COMMAND} ${valgrind_flags} ${exec} ${valgrind_exec_flags} ${config}
            WORKING_DIRECTORY ${working_directory}
            VERBATIM)
    endif()
endfunction()

#[==[.rst:
.. cmake:command:: add_opp_test

Add a smoke test target.
#]==]
function(add_opp_test name)
    set(one_value_args "CONFIG;RUN;SIMTIME_LIMIT;SUFFIX")
    set(multi_value_args "")
    cmake_parse_arguments(args "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(SEND_ERROR "add_opp_test called with invalid arguments: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_SUFFIX)
        set(suffix "${args_SUFFIX}")
    else()
        message(SEND_ERROR "add_opp_test called without required SUFFIX argument")
    endif()

    set(opp_run_args "-uCmdenv")
    if(args_CONFIG)
        list(APPEND opp_run_args "-c${args_CONFIG}")
    endif()
    if(args_RUN)
        list(APPEND opp_run_args "-r${args_RUN}")
    endif()
    if(args_SIMTIME_LIMIT)
        list(APPEND opp_run_args "--sim-time-limit=${args_SIMTIME_LIMIT}")
    endif()

    if(NOT TARGET run_${name})
        message(FATAL_ERROR "add_opp_test(${name} ...) requires a prior add_opp_run(${name} ...)")
    endif()

    get_target_property(target run_${name} OPP_RUN_TARGET)
    get_target_property(working_directory run_${name} OPP_RUN_WORKING_DIRECTORY)
    get_target_property(config run_${name} OPP_RUN_CONFIG_FILE)
    get_target_property(ned_folders run_${name} OPP_RUN_NED_FOLDERS)
    _build_opp_run_command(TARGET ${target} OUTPUT exec NED_FOLDERS ${ned_folders})

    add_test(NAME "${name}-${suffix}"
        COMMAND ${exec} ${config} ${opp_run_args}
        WORKING_DIRECTORY ${working_directory})
endfunction(add_opp_test)

#[==[.rst:
.. cmake:command:: vscode_addon_option

Adds a CMake cache option which allows users to toggle the Visual Studio Code add-on on and off.
If a *\.vscode* directory exists in the project's root directory, the add-on is enabled by default then.
#]==]
function(vscode_addon_option)
    if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/.vscode)
        set(has_vscode_dir ON)
    else()
        set(has_vscode_dir OFF)
    endif()

    option(VSCODE_ADDON "Enable VSCode build add-on" ${has_vscode_dir})
    set_property(GLOBAL PROPERTY VSCODE_ADDON ${VSCODE_ADDON})
endfunction()

#[==[.rst:
.. cmake:command:: generate_run_script

Generates a shell script for running a simulation target.
#]==]
function(generate_run_script)
    set(option_args "INSTALL")
    set(one_value_args "TARGET;FILE;TEMPLATE")
    cmake_parse_arguments(args "${option_args}" "${one_value_args}" "" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(SEND_ERROR "generate_run_script called with invalid arguments: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT args_TARGET)
        message(SEND_ERROR "generate_run_script: TARGET argument is missing")
    endif()

    if(NOT args_FILE)
        message(SEND_ERROR "generate_run_script: FILE argument is missing")
    endif()

    # collect all NED folders for given target
    if(args_INSTALL)
        get_install_ned_folders(${args_TARGET} opp_run_ned_folders)
    else()
        get_ned_folders(${args_TARGET} opp_run_ned_folders)
    endif()
    set(opp_run_ned_folders "$<JOIN:${opp_run_ned_folders},:>")

    # collect libraries for opp_run
    set(opp_run_libraries "")
    _get_opp_run_dependencies(${args_TARGET} opp_run_dependencies)
    foreach(opp_run_dependency IN LISTS opp_run_dependencies)
        if(args_INSTALL)
            list(APPEND opp_run_libraries "-l${CMAKE_INSTALL_PREFIX}/lib/$<TARGET_FILE_NAME:${opp_run_dependency}>")
        else()
            list(APPEND opp_run_libraries "-l$<TARGET_FILE:${opp_run_dependency}>")
        endif()
    endforeach()
    set(opp_run_libraries "$<JOIN:${opp_run_libraries}, >")

    # select opp_run executable depending on build type
    if(NOT CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(opp_run_executable ${OMNETPP_RUN_DEBUG})
    else()
        set(opp_run_executable ${OMNETPP_RUN})
    endif()
    set(opp_runall_script ${OMNETPP_RUNALL})

    # substitute variables first, then generator expressions
    if(NOT args_TEMPLATE)
        configure_file(${_OPP_CMAKE_BASE_DIR}/run_opp.sh.in ${args_FILE} @ONLY)
    else()
        configure_file(${args_TEMPLATE} ${args_FILE} @ONLY)
    endif()
    file(GENERATE OUTPUT ${args_FILE} INPUT ${args_FILE})
endfunction()
