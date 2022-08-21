set(SOURCES
    ass/dialogue_parser.cpp
    ass/karaoke.cpp
    ass/time.cpp
    ass/uuencode.cpp

    audio/provider_convert.cpp
    audio/provider.cpp
    audio/provider_dummy.cpp
    audio/provider_hd.cpp
    audio/provider_lock.cpp
    audio/provider_pcm.cpp
    audio/provider_ram.cpp

    common/calltip_provider.cpp
    common/character_count.cpp
    common/charset_6937.cpp
    common/charset_conv.cpp
    common/charset.cpp
    common/color.cpp
    common/file_mapping.cpp
    common/format.cpp
    common/fs.cpp
    common/hotkey.cpp
    common/io.cpp
    common/json.cpp
    common/kana_table.cpp
    common/karaoke_matcher.cpp
    common/keyframe.cpp
    common/line_iterator.cpp
    common/log.cpp
    common/mru.cpp
    common/option.cpp
    common/option_value.cpp
    common/parser.cpp
    common/path.cpp
    common/thesaurus.cpp
    common/unicode.cpp
    common/util.cpp
    common/vfr.cpp
    common/ycbcr_conv.cpp
    common/cajun/elements.cpp
    common/cajun/reader.cpp
    common/cajun/writer.cpp

    lua/modules.cpp
    lua/script_reader.cpp
    lua/utils.cpp
    lua/modules/lfs.cpp
    lua/modules/re.cpp
    lua/modules/unicode.cpp
    lua/modules/lpeg.c
)

set(HEADERS
    common/charset_6937.h
    common/parser.h

    include/libaegisub/access.h
    include/libaegisub/address_of_adaptor.h
    include/libaegisub/background_runner.h
    include/libaegisub/calltip_provider.h
    include/libaegisub/character_count.h
    include/libaegisub/charset.h
    include/libaegisub/charset_conv.h
    include/libaegisub/charset_conv_win.h
    include/libaegisub/color.h
    include/libaegisub/dispatch.h
    include/libaegisub/exception.h
    include/libaegisub/file_mapping.h
    include/libaegisub/format.h
    include/libaegisub/format_flyweight.h
    include/libaegisub/format_path.h
    include/libaegisub/fs.h
    include/libaegisub/hotkey.h
    include/libaegisub/io.h
    include/libaegisub/json.h
    include/libaegisub/kana_table.h
    include/libaegisub/karaoke_matcher.h
    include/libaegisub/keyframe.h
    include/libaegisub/line_iterator.h
    include/libaegisub/line_wrap.h
    include/libaegisub/log.h
    include/libaegisub/mru.h
    include/libaegisub/of_type_adaptor.h
    include/libaegisub/option.h
    include/libaegisub/option_value.h
    include/libaegisub/owning_intrusive_list.h
    include/libaegisub/path.h
    include/libaegisub/scoped_ptr.h
    include/libaegisub/signal.h
    include/libaegisub/spellchecker.h
    include/libaegisub/split.h
    include/libaegisub/thesaurus.h
    include/libaegisub/type_name.h
    include/libaegisub/unicode.h
    include/libaegisub/util.h
    include/libaegisub/util_osx.h
    include/libaegisub/vfr.h
    include/libaegisub/ycbcr_conv.h

    include/libaegisub/ass/dialogue_parser.h
    include/libaegisub/ass/karaoke.h
    include/libaegisub/ass/smpte.h
    include/libaegisub/ass/time.h
    include/libaegisub/ass/uuencode.h

    include/libaegisub/audio/provider.h

    include/libaegisub/cajun/elements.h
    include/libaegisub/cajun/reader.h
    include/libaegisub/cajun/visitor.h
    include/libaegisub/cajun/writer.h

    include/libaegisub/lua/ffi.h
    include/libaegisub/lua/modules.h
    include/libaegisub/lua/script_reader.h
    include/libaegisub/lua/utils.h

    include/lagi_pre.h
    include/lagi_pre_c.h
    lua/modules/lpeg.h
)

if(APPLE)
    list(APPEND SOURCES
        osx/dispatch.mm
        osx/spellchecker.mm
        osx/util.mm
        )
else()
    list(APPEND SOURCES common/dispatch.cpp)
endif()

if(WINDOWS)
    list(APPEND SOURCES
        windows/access.cpp
        windows/charset_conv_win.cpp
        windows/fs.cpp
        windows/log_win.cpp
        windows/path_win.cpp
        windows/util_win.cpp)
else()
    list(APPEND SOURCES
        unix/access.cpp
        unix/fs.cpp
        unix/log.cpp
        unix/path.cpp
        unix/util.cpp
        )
endif()


add_library(agi SHARED ${SOURCES} ${HEADERS})
target_include_directories(agi PUBLIC include)
target_link_libraries(agi PUBLIC
    boost
    luajit
    luabins
    icu
    iconv
    "-framework Foundation"
    "-framework AppKit"
    )

# function (_get_all_cmake_targets out_var current_dir)
#     get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
#     message(STATUS "targets: '${targets}'")
#     get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)
#
#     foreach(subdir ${subdirs})
#         _get_all_cmake_targets(subdir_targets ${subdir})
#         list(APPEND targets ${subdir_targets})
#     endforeach()
#
#     set(${out_var} ${targets} PARENT_SCOPE)
# endfunction()
#
# # Run at end of top-level CMakeLists
# _get_all_cmake_targets(all_targets ${CMAKE_CURRENT_LIST_DIR})
#
# # Get all propreties that cmake supports
# if(NOT CMAKE_PROPERTY_LIST)
#     execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
#     
#     # Convert command output into a CMake list
#     string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
#     string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
# endif()
#     
# function(print_properties)
#     message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
# endfunction()
#     
# function(print_target_properties target)
#     if(NOT TARGET ${target})
#       message(STATUS "There is no target named '${target}'")
#       return()
#     endif()
#
#     foreach(property ${CMAKE_PROPERTY_LIST})
#         string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})
#
#         # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
#         if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
#             continue()
#         endif()
#
#         get_property(was_set TARGET ${target} PROPERTY ${property} SET)
#         if(was_set)
#             get_target_property(value ${target} ${property})
#             message("${target} ${property} = ${value}")
#         endif()
#     endforeach()
# endfunction()
# print_target_properties(icu)



