include(CMakeParseArguments)

#[==[.rst:
GenerateOppMessage
------------------

This module provides the `generate_opp_message` function, which has the following signature.

.. code-block:: cmake

    generate_opp_message(<INPUT_MSG_FILE>
        [MSG4]
        [TARGET target]
        [OUTPUT_ROOT directory]
        [DIRECTORY directory]
        [GEN_SOURCES var]
        [GEN_INCLUDE_DIR var]
        [ADDITIONAL_NED_PATHS path ...]
    )

.. cmake:command:: generate_opp_message

At least the `<INPUT_MSG_FILE>` argument must be given, which is the *\*.msg* file passed to the OMNeT++ message compiler. The OMNeT++ message compiler must be known via the `OMNETPP_MSGC` variable, which is usually set by the :doc:`find-omnetpp`.

``MSG4``
    Forces the message compiler to process the input file as OMNeT++ 4.x message file.

``TARGET``
    Add the generated sources to the given target by calling `target_sources`.
    The `OUTPUT_ROOT` directory is added as include directory to this target as well.

``OUTPUT_ROOT``
    The root directory where the output files will be generated.
    `${PROJECT_BINARY_DIR}/opp_messages` is used as default directory.

``DIRECTORY``
    Optional sub-directory relative to `OUTPUT_ROOT`, i.e. the given path is appended to `OUTPUT_ROOT`.

``GEN_SOURCES``
    Name of an output variable which gets populated with the generated filenames.

``GEN_INCLUDE_DIR``
    Name of an output variable which gets populated with the include directory for using the generated messages.

``ADDITIONAL_NED_PATHS``
    Further import paths during message compilation.
    These paths are passed on to the message compiler as `-I` arguments.

#]==]

# generate sources for messages via opp_msgc
function(generate_opp_message msg_input)
    if(NOT EXISTS ${OMNETPP_MSGC})
        message(FATAL_ERROR "OMNeT++ message compiler is missing")
    endif()

    set(options_args MSG4)
    set(single_args TARGET DIRECTORY OUTPUT_ROOT GEN_SOURCES GEN_INCLUDE_DIR DLL_SYMBOL)
    set(multi_args ADDITIONAL_NED_PATHS)
    cmake_parse_arguments(args "${options_args}" "${single_args}" "${multi_args}" ${ARGN})

    if(args_UNPARSED_ARGUMENTS)
        message(SEND_ERROR "generate_opp_message called with invalid arguments: ${args_UNPARSED_ARGUMENTS}")
    endif()

    if(args_OUTPUT_ROOT)
        set(msg_output_root ${args_OUTPUT_ROOT})
    else()
        set(msg_output_root ${PROJECT_BINARY_DIR}/opp_messages)
    endif()

    get_filename_component(msg_full_name "${msg_input}" NAME)
    get_filename_component(msg_name "${msg_input}" NAME_WE)
    get_filename_component(msg_dir "${msg_input}" DIRECTORY)

    if(args_DIRECTORY)
        set(msg_prefix "${args_DIRECTORY}")
    else()
        file(RELATIVE_PATH msg_prefix ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/${msg_dir})
    endif()

    set(msg_output_dir "${msg_output_root}/${msg_prefix}")
    set(msg_output_source "${msg_output_dir}/${msg_name}_m.cc")
    set(msg_output_header "${msg_output_dir}/${msg_name}_m.h")

    # Prepare arguments for command
    list(APPEND _args "-s" "_m.cc")

    foreach(include_dir IN LISTS args_ADDITIONAL_NED_PATHS)
        list(APPEND _args "-I" ${include_dir})
    endforeach()

    # Handle message version
    if(args_MSG4)
        list(APPEND _args "--msg4")
    endif()

    # Handle DLL-Export
    if(WIN32 OR MSVC)
        if(args_DLL_SYMBOL)
            list(APPEND _args "-P ${args_DLL_SYMBOL}")
        endif()
    endif()

    # Create the output directory
    file(MAKE_DIRECTORY ${msg_output_dir})

    # Copy the msg file to the output directory (since the -h otion is gone in version 6)
    set(msg_input_process "${msg_output_dir}/${msg_full_name}")

    add_custom_command(OUTPUT ${msg_input_process}
        COMMAND ${CMAKE_COMMAND} -E copy ${msg_input} ${msg_input_process}
        COMMENT "Copying ${msg_full_name} to output directory"
        DEPENDS ${msg_input}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
    list(APPEND _args ${msg_input_process})

    add_custom_command(OUTPUT "${msg_output_source}" "${msg_output_header}"
        COMMAND ${OMNETPP_MSGC} ARGS ${_args}
        COMMAND_EXPAND_LISTS
        COMMENT "Generating ${msg_prefix}/${msg_name}"
        DEPENDS ${OMNETPP_MSGC} ${msg_input_process}
        WORKING_DIRECTORY ${msg_output_dir}
        VERBATIM
    )

    if (args_TARGET)
        target_sources(${args_TARGET} PRIVATE "${msg_output_source}" "${msg_output_header}")
        target_include_directories(${args_TARGET} PUBLIC ${msg_output_root})
    endif()

    if(args_GEN_SOURCES)
        set(${args_GEN_SOURCES} "${msg_output_source}" "${msg_output_header}" PARENT_SCOPE)
    endif()

    if(args_GEN_INCLUDE_DIR)
        set(${args_GEN_INCLUDE_DIR} ${msg_output_root} PARENT_SCOPE)
    endif()
endfunction()

#[==[.rst:
Earlier implementations provided a `clean_opp_messages` macro to delete the generated sources from the source tree.
Nowadays, message files are compiled in the build directory and CMake automatically removes generated artifacts with its `clean` target.
The `clean_opp_messages` is thus no longer needed and has been removed.
#]==]