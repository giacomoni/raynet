#[==[.rst:
AddOppTarget
------------

*This documentation is still a stub!*

.. cmake:command:: add_opp_target
#]==]

include(CMakeParseArguments)

function(add_opp_target)
    set(options_args MSG4)
    set(single_args ROOT_DIR SOURCE_DIR TARGET DLL_SYMBOL)
    set(multi_args DEPENDS OPP_MAKEMAKE)
    cmake_parse_arguments(args "${options_args}" "${single_args}" "${multi_args}" ${ARGN})

    if(NOT args_SOURCE_DIR)
        set(args_SOURCE_DIR ${args_ROOT_DIR}/src)
    endif()

    file(GLOB_RECURSE cpp_files ${args_SOURCE_DIR}/*.cc)
    file(GLOB_RECURSE msg_files ${args_SOURCE_DIR}/*.msg)

    # remove in-tree sources of generated messages
    file(GLOB_RECURSE cpp_msg_files ${args_SOURCE_DIR}/*_m.cc)
    if(cpp_msg_files)
        list(REMOVE_ITEM cpp_files ${cpp_msg_files})
    endif()

    # process opp_makemake options (only -X for now)
    set(exclude_regex "")
    foreach(option IN LISTS args_OPP_MAKEMAKE)
        string(SUBSTRING "${option}" 0 2 option_name)
        string(SUBSTRING "${option}" 2 -1 option_value)
        if (option_name STREQUAL "-X" AND option_value)
            file(TO_CMAKE_PATH ${option_value} _path)
            list(APPEND exclude_regex "${args_SOURCE_DIR}/${_path}")
        endif()
    endforeach()
    string(REPLACE ";" "|" exclude_regex "^(${exclude_regex})")

    # remove excluded source files
    if(NOT exclude_regex STREQUAL "^()")
        foreach(file IN LISTS cpp_files msg_files)
            string(REGEX MATCH "${exclude_regex}" exclude_match ${file})
            if(exclude_match)
                list(APPEND files_excluded ${file})
            endif()
        endforeach()
        if(files_excluded)
            list(REMOVE_ITEM cpp_files ${files_excluded})
            list(REMOVE_ITEM msg_files ${files_excluded})
        endif()
    endif()

    # On Windows, if no DLL-Symbol is given, handle default
    if(WIN32 OR MSVC AND NOT args_DLL_SYMBOL)
        set(args_DLL_SYMBOL "${args_TARGET}_API")
    endif()

    # generate OMNeT++ message code in build directory
    set(msg_gen_dir ${PROJECT_BINARY_DIR}/${args_TARGET}_gen)
    foreach(msg_file IN LISTS msg_files)
        get_filename_component(msg_name "${msg_file}" NAME_WE)
        get_filename_component(msg_dir "${msg_file}" DIRECTORY)
        file(RELATIVE_PATH msg_prefix ${args_SOURCE_DIR} ${msg_dir})

        set(gen_opp_msg_opt_args "")
        if(args_MSG4)
            list(APPEND gen_opp_msg_opt_args "MSG4")
        endif()

        generate_opp_message(
            ${msg_file}
            OUTPUT_ROOT             ${msg_gen_dir}
            DIRECTORY               ${msg_prefix}
            GEN_SOURCES             _cpp_files
            ADDITIONAL_NED_PATHS    ${args_SOURCE_DIR}
            DLL_SYMBOL              ${args_DLL_SYMBOL}
            ${gen_opp_msg_opt_args}
        )

        list(APPEND cpp_files ${_cpp_files})
    endforeach()

    # set up target for OMNeT++ project
    add_library(${args_TARGET} SHARED ${cpp_files} ${args_DEPENDS})
    target_include_directories(${args_TARGET} PUBLIC ${msg_gen_dir} ${args_SOURCE_DIR})
    target_link_libraries(${args_TARGET} PUBLIC OmnetPP::envir)
    set_property(TARGET ${args_TARGET} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/extern)
    set_property(TARGET ${args_TARGET} PROPERTY OMNETPP_LIBRARY TRUE)
    install(TARGETS ${args_TARGET} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    # look up NED folders, use source directory as default if no .nedfolders file exists
    set(ned_folders ${args_SOURCE_DIR})
    if(EXISTS "${args_ROOT_DIR}/.nedfolders")
        file(STRINGS "${args_ROOT_DIR}/.nedfolders" ned_folders)
    endif()

    # determine absolute NED paths and install NED files
    set(ned_folders_abs "")
    foreach(ned_folder IN LISTS ned_folders)
        get_filename_component(ned_folder_abs ${ned_folder} ABSOLUTE BASE_DIR ${args_ROOT_DIR})
        list(APPEND ned_folders_abs ${ned_folder_abs})
        set(ned_folder_install ${CMAKE_INSTALL_DATADIR}/ned/${args_TARGET}/${ned_folder})
        set_property(TARGET ${args_TARGET} APPEND PROPERTY INSTALL_NED_FOLDERS ${ned_folder_install})
        install(DIRECTORY ${ned_folder_abs}/ DESTINATION ${ned_folder_install} FILES_MATCHING PATTERN "*.ned")
    endforeach()
    set_property(TARGET ${args_TARGET} PROPERTY NED_FOLDERS ${ned_folders_abs})
endfunction()
