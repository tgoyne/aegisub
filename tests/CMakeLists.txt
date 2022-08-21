CPMAddPackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG release-1.8.1
  VERSION 1.8.1
  OPTIONS
      "INSTALL_GTEST OFF"
      "gtest_force_shared_crt ON"
)

set(SOURCES
    support/main.cpp
    support/util.cpp

    tests/access.cpp
    tests/audio.cpp
    tests/cajun.cpp
    tests/calltip_provider.cpp
    tests/character_count.cpp
    tests/color.cpp
    tests/dialogue_lexer.cpp
    tests/format.cpp
    tests/fs.cpp
    tests/hotkey.cpp
    tests/iconv.cpp
    tests/ifind.cpp
    tests/karaoke_matcher.cpp
    tests/keyframe.cpp
    tests/line_iterator.cpp
    tests/line_wrap.cpp
    tests/mru.cpp
    tests/option.cpp
    tests/path.cpp
    tests/signals.cpp
    tests/split.cpp
    tests/syntax_highlight.cpp
    tests/thesaurus.cpp
    tests/time.cpp
    tests/type_name.cpp
    tests/util.cpp
    tests/uuencode.cpp
    tests/vfr.cpp
    tests/word_split.cpp
)

set(HEADERS
    support/main.h
    support/tests_pre.h
    support/util.h
)

add_executable(tests ${SOURCES} ${HEADERS})
target_link_libraries(tests agi gtest)
target_include_directories(tests PRIVATE support)
