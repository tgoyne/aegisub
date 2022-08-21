cmake_minimum_required(VERSION 3.15)
project(Aegisub LANGUAGES C CXX OBJC OBJCXX ASM)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CPM)

# Configuration options
option(AEGISUB_SYSTEM_PACKAGES "" ON)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

# Project-wide build flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum version of macOS")

if(CMAKE_SYSTEM_NAME MATCHES "^Windows")
  add_compile_definitions(
    # Excluse some of the garbage that windows.h imports by default
    WIN32_LEAN_AND_MEAN
    # Prefer Unicode variants of APIs
    UNICODE
    _UNICODE
    # Require Windows 10
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
    NTDDI_VERSION=0x0A000000
  )
endif()

if(MSVC)
  add_compile_options(/MP)
endif()

CPMAddPackage(
  NAME freetype
  VERSION 2.12.1
  URL https://download.savannah.gnu.org/releases/freetype/freetype-2.12.1.tar.xz
  URL_HASH SHA256=4766f20157cc4cf0cd292f80bf917f92d1c439b243ac3018debf6b9140c41a7f
)

CPMAddPackage(
  NAME fribidi
  VERSION 1.0.12
  GIT_REPOSITORY https://github.com/fribidi/fribidi
  DOWNLOAD_ONLY YES
)

function(fribidi_tab name input)
  list(TRANSFORM input PREPEND "${fribidi_SOURCE_DIR}/gen.tab/")
  set(output "${fribidi_BINARY_DIR}/gen.tab/${name}.tab.i")

  add_executable(fribidi-gen-${name}
    "${fribidi_SOURCE_DIR}/gen.tab/gen-${name}-tab.c"
    "${fribidi_SOURCE_DIR}/gen.tab/packtab.c")
  target_compile_definitions(fribidi-gen-${name} PRIVATE
    DONT_HAVE_FRIBIDI_CONFIG_H=1
    DONT_HAVE_FRIBIDI_UNICODE_VERSION_H=1
    HAVE_MEMORY_H=1
    HAVE_STDLIB_H=1
    HAVE_STRINGIZE=1
    HAVE_STRINGS_H=1
    HAVE_STRING_H=1
    STDC_HEADERS=1)
  target_include_directories(fribidi-gen-${name} PRIVATE
    $<BUILD_INTERFACE:${fribidi_SOURCE_DIR}>/lib
    ${fribidi_BINARY_DIR}/include
  )
  # target_compile_options(fribidi-gen-${name} PRIVATE -Wno-implicit-function-declaration)
  add_custom_command(
    COMMAND fribidi-gen-${name} 2 ${input} > ${output}
    DEPENDS ${input}
    OUTPUT ${output})
  list(APPEND FRIBIDI_GENERATED_SOURCES "${output}")
  set(FRIBIDI_GENERATED_SOURCES ${FRIBIDI_GENERATED_SOURCES} PARENT_SCOPE)
endfunction()

fribidi_tab(bidi-type unidata/UnicodeData.txt)
fribidi_tab(joining-type "unidata/UnicodeData.txt;unidata/ArabicShaping.txt")
fribidi_tab(arabic-shaping unidata/UnicodeData.txt)
fribidi_tab(mirroring unidata/BidiMirroring.txt)
fribidi_tab(brackets "unidata/BidiBrackets.txt;unidata/UnicodeData.txt")
fribidi_tab(brackets-type unidata/BidiBrackets.txt)

add_library(fribidi STATIC
  ${fribidi_SOURCE_DIR}/lib/fribidi.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-arabic.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-bidi.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-bidi-types.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets-cap-rtl.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets-cp1255.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets-cp1256.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets-iso8859-6.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets-iso8859-8.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-char-sets-utf8.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-deprecated.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-joining.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-joining-types.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-mirroring.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-brackets.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-run.c
  ${fribidi_SOURCE_DIR}/lib/fribidi-shape.c
  ${FRIBIDI_GENERATED_SOURCES}
)
configure_file(thirdparty/fribidi/config.h.in ${fribidi_BINARY_DIR}/include/config.h)

target_compile_definitions(fribidi PRIVATE
  -DHAVE_CONFIG_H
  -DFRIBIDI_BUILD)
target_compile_definitions(fribidi INTERFACE
  DONT_HAVE_FRIBIDI_CONFIG_H=1
  DONT_HAVE_FRIBIDI_UNICODE_VERSION_H=1)
target_include_directories(fribidi PUBLIC
  $<BUILD_INTERFACE:${fribidi_SOURCE_DIR}>/lib
)
target_include_directories(fribidi PRIVATE
  ${fribidi_BINARY_DIR}/include
  ${fribidi_BINARY_DIR}/gen.tab
)

CPMAddPackage(
  NAME ICU
  VERSION 67.1
  URL https://github.com/unicode-org/icu/releases/download/release-67-1/icu4c-67_1-src.tgz
  URL_HASH SHA256=94a80cd6f251a53bd2a997f6f1b5ac6653fe791dfab66e1eb0227740fb86d5dc
  DOWNLOAD_ONLY YES
)
add_library(icu STATIC
  ${ICU_SOURCE_DIR}/source/common/appendable.cpp
  ${ICU_SOURCE_DIR}/source/common/bmpset.cpp
  ${ICU_SOURCE_DIR}/source/common/brkeng.cpp
  ${ICU_SOURCE_DIR}/source/common/brkiter.cpp
  ${ICU_SOURCE_DIR}/source/common/bytesinkutil.cpp
  ${ICU_SOURCE_DIR}/source/common/bytestream.cpp
  ${ICU_SOURCE_DIR}/source/common/bytestrie.cpp
  ${ICU_SOURCE_DIR}/source/common/characterproperties.cpp
  ${ICU_SOURCE_DIR}/source/common/chariter.cpp
  ${ICU_SOURCE_DIR}/source/common/charstr.cpp
  ${ICU_SOURCE_DIR}/source/common/cmemory.cpp
  ${ICU_SOURCE_DIR}/source/common/cstring.cpp
  ${ICU_SOURCE_DIR}/source/common/dictbe.cpp
  ${ICU_SOURCE_DIR}/source/common/dictionarydata.cpp
  ${ICU_SOURCE_DIR}/source/common/edits.cpp
  ${ICU_SOURCE_DIR}/source/common/errorcode.cpp
  ${ICU_SOURCE_DIR}/source/common/filteredbrk.cpp
  ${ICU_SOURCE_DIR}/source/common/filterednormalizer2.cpp
  ${ICU_SOURCE_DIR}/source/common/loadednormalizer2impl.cpp
  ${ICU_SOURCE_DIR}/source/common/locavailable.cpp
  ${ICU_SOURCE_DIR}/source/common/locbased.cpp
  ${ICU_SOURCE_DIR}/source/common/locdispnames.cpp
  ${ICU_SOURCE_DIR}/source/common/locid.cpp
  ${ICU_SOURCE_DIR}/source/common/loclikely.cpp
  ${ICU_SOURCE_DIR}/source/common/locmap.cpp
  ${ICU_SOURCE_DIR}/source/common/locresdata.cpp
  ${ICU_SOURCE_DIR}/source/common/locutil.cpp
  ${ICU_SOURCE_DIR}/source/common/normalizer2.cpp
  ${ICU_SOURCE_DIR}/source/common/normalizer2impl.cpp
  ${ICU_SOURCE_DIR}/source/common/normlzr.cpp
  ${ICU_SOURCE_DIR}/source/common/parsepos.cpp
  ${ICU_SOURCE_DIR}/source/common/patternprops.cpp
  ${ICU_SOURCE_DIR}/source/common/propname.cpp
  ${ICU_SOURCE_DIR}/source/common/putil.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbi.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbi_cache.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbidata.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbinode.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbirb.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbiscan.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbisetb.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbistbl.cpp
  ${ICU_SOURCE_DIR}/source/common/rbbitblb.cpp
  ${ICU_SOURCE_DIR}/source/common/resbund.cpp
  ${ICU_SOURCE_DIR}/source/common/resbund_cnv.cpp
  ${ICU_SOURCE_DIR}/source/common/resource.cpp
  ${ICU_SOURCE_DIR}/source/common/ruleiter.cpp
  ${ICU_SOURCE_DIR}/source/common/schriter.cpp
  ${ICU_SOURCE_DIR}/source/common/serv.cpp
  ${ICU_SOURCE_DIR}/source/common/servlk.cpp
  ${ICU_SOURCE_DIR}/source/common/servlkf.cpp
  ${ICU_SOURCE_DIR}/source/common/servls.cpp
  ${ICU_SOURCE_DIR}/source/common/servnotf.cpp
  ${ICU_SOURCE_DIR}/source/common/servrbf.cpp
  ${ICU_SOURCE_DIR}/source/common/servslkf.cpp
  ${ICU_SOURCE_DIR}/source/common/sharedobject.cpp
  ${ICU_SOURCE_DIR}/source/common/stringpiece.cpp
  ${ICU_SOURCE_DIR}/source/common/stringtriebuilder.cpp
  ${ICU_SOURCE_DIR}/source/common/uarrsort.cpp
  ${ICU_SOURCE_DIR}/source/common/ubidi_props.cpp
  ${ICU_SOURCE_DIR}/source/common/ucase.cpp
  ${ICU_SOURCE_DIR}/source/common/ucasemap.cpp
  ${ICU_SOURCE_DIR}/source/common/ucasemap_titlecase_brkiter.cpp
  ${ICU_SOURCE_DIR}/source/common/uchar.cpp
  ${ICU_SOURCE_DIR}/source/common/ucharstrie.cpp
  ${ICU_SOURCE_DIR}/source/common/ucharstriebuilder.cpp
  ${ICU_SOURCE_DIR}/source/common/ucharstrieiterator.cpp
  ${ICU_SOURCE_DIR}/source/common/uchriter.cpp
  ${ICU_SOURCE_DIR}/source/common/ucln_cmn.cpp
  ${ICU_SOURCE_DIR}/source/common/ucmndata.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv2022.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_bld.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_cb.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_cnv.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_ct.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_err.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_ext.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_io.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_lmb.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_u16.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_u32.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_u7.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnv_u8.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvbocu.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvhz.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvisci.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvlat1.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvlat1.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvmbcs.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvmbcs.cpp
  ${ICU_SOURCE_DIR}/source/common/ucnvscsu.cpp
  ${ICU_SOURCE_DIR}/source/common/ucol_swp.cpp
  ${ICU_SOURCE_DIR}/source/common/ucptrie.cpp
  ${ICU_SOURCE_DIR}/source/common/udata.cpp
  ${ICU_SOURCE_DIR}/source/common/udatamem.cpp
  ${ICU_SOURCE_DIR}/source/common/udataswp.cpp
  ${ICU_SOURCE_DIR}/source/common/uenum.cpp
  ${ICU_SOURCE_DIR}/source/common/uhash.cpp
  ${ICU_SOURCE_DIR}/source/common/uhash_us.cpp
  ${ICU_SOURCE_DIR}/source/common/uinvchar.cpp
  ${ICU_SOURCE_DIR}/source/common/uiter.cpp
  ${ICU_SOURCE_DIR}/source/common/ulist.cpp
  ${ICU_SOURCE_DIR}/source/common/uloc.cpp
  ${ICU_SOURCE_DIR}/source/common/uloc_keytype.cpp
  ${ICU_SOURCE_DIR}/source/common/uloc_tag.cpp
  ${ICU_SOURCE_DIR}/source/common/umapfile.cpp
  ${ICU_SOURCE_DIR}/source/common/umath.cpp
  ${ICU_SOURCE_DIR}/source/common/umutablecptrie.cpp
  ${ICU_SOURCE_DIR}/source/common/umutex.cpp
  ${ICU_SOURCE_DIR}/source/common/unames.cpp
  ${ICU_SOURCE_DIR}/source/common/unifiedcache.cpp
  ${ICU_SOURCE_DIR}/source/common/unifilt.cpp
  ${ICU_SOURCE_DIR}/source/common/unifunct.cpp
  ${ICU_SOURCE_DIR}/source/common/uniset.cpp
  ${ICU_SOURCE_DIR}/source/common/uniset_props.cpp
  ${ICU_SOURCE_DIR}/source/common/unisetspan.cpp
  ${ICU_SOURCE_DIR}/source/common/unistr.cpp
  ${ICU_SOURCE_DIR}/source/common/unistr_case.cpp
  ${ICU_SOURCE_DIR}/source/common/unistr_case_locale.cpp
  ${ICU_SOURCE_DIR}/source/common/unistr_cnv.cpp
  ${ICU_SOURCE_DIR}/source/common/unistr_titlecase_brkiter.cpp
  ${ICU_SOURCE_DIR}/source/common/uobject.cpp
  ${ICU_SOURCE_DIR}/source/common/uprops.cpp
  ${ICU_SOURCE_DIR}/source/common/uresbund.cpp
  ${ICU_SOURCE_DIR}/source/common/uresdata.cpp
  ${ICU_SOURCE_DIR}/source/common/uscript_props.cpp
  ${ICU_SOURCE_DIR}/source/common/uset.cpp
  ${ICU_SOURCE_DIR}/source/common/usetiter.cpp
  ${ICU_SOURCE_DIR}/source/common/ustack.cpp
  ${ICU_SOURCE_DIR}/source/common/ustr_cnv.cpp
  ${ICU_SOURCE_DIR}/source/common/ustr_titlecase_brkiter.cpp
  ${ICU_SOURCE_DIR}/source/common/ustrcase.cpp
  ${ICU_SOURCE_DIR}/source/common/ustrcase_locale.cpp
  ${ICU_SOURCE_DIR}/source/common/ustrenum.cpp
  ${ICU_SOURCE_DIR}/source/common/ustrfmt.cpp
  ${ICU_SOURCE_DIR}/source/common/ustring.cpp
  ${ICU_SOURCE_DIR}/source/common/ustrtrns.cpp
  ${ICU_SOURCE_DIR}/source/common/utext.cpp
  ${ICU_SOURCE_DIR}/source/common/utf_impl.cpp
  ${ICU_SOURCE_DIR}/source/common/util.cpp
  ${ICU_SOURCE_DIR}/source/common/utrace.cpp
  ${ICU_SOURCE_DIR}/source/common/utrie.cpp
  ${ICU_SOURCE_DIR}/source/common/utrie2.cpp
  ${ICU_SOURCE_DIR}/source/common/utrie2_builder.cpp
  ${ICU_SOURCE_DIR}/source/common/utrie_swap.cpp
  ${ICU_SOURCE_DIR}/source/common/utypes.cpp
  ${ICU_SOURCE_DIR}/source/common/uvector.cpp
  ${ICU_SOURCE_DIR}/source/common/uvectr32.cpp
  ${ICU_SOURCE_DIR}/source/common/uvectr64.cpp
  ${ICU_SOURCE_DIR}/source/i18n/bocsu.cpp
  ${ICU_SOURCE_DIR}/source/i18n/coleitr.cpp
  ${ICU_SOURCE_DIR}/source/i18n/coll.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collation.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationbuilder.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationcompare.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationdata.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationdatareader.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationdatawriter.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationfastlatin.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationfcd.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationiterator.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationkeys.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationroot.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationsets.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationsettings.cpp
  ${ICU_SOURCE_DIR}/source/i18n/collationtailoring.cpp
  ${ICU_SOURCE_DIR}/source/i18n/rulebasedcollator.cpp
  ${ICU_SOURCE_DIR}/source/i18n/sortkey.cpp
  ${ICU_SOURCE_DIR}/source/i18n/ucln_in.cpp
  ${ICU_SOURCE_DIR}/source/i18n/ucol.cpp
  ${ICU_SOURCE_DIR}/source/i18n/ucol_res.cpp
  ${ICU_SOURCE_DIR}/source/i18n/uitercollationiterator.cpp
  ${ICU_SOURCE_DIR}/source/i18n/utf16collationiterator.cpp
  ${ICU_SOURCE_DIR}/source/i18n/utf8collationiterator.cpp
  ${ICU_SOURCE_DIR}/source/stubdata/stubdata.cpp


  ${ICU_SOURCE_DIR}/source/common/unicode/appendable.h
  ${ICU_SOURCE_DIR}/source/common/unicode/brkiter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/bytestream.h
  ${ICU_SOURCE_DIR}/source/common/unicode/bytestrie.h
  ${ICU_SOURCE_DIR}/source/common/unicode/bytestriebuilder.h
  ${ICU_SOURCE_DIR}/source/common/unicode/caniter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/casemap.h
  ${ICU_SOURCE_DIR}/source/common/unicode/char16ptr.h
  ${ICU_SOURCE_DIR}/source/common/unicode/chariter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/dbbi.h
  ${ICU_SOURCE_DIR}/source/common/unicode/docmain.h
  ${ICU_SOURCE_DIR}/source/common/unicode/dtintrv.h
  ${ICU_SOURCE_DIR}/source/common/unicode/edits.h
  ${ICU_SOURCE_DIR}/source/common/unicode/enumset.h
  ${ICU_SOURCE_DIR}/source/common/unicode/errorcode.h
  ${ICU_SOURCE_DIR}/source/common/unicode/filteredbrk.h
  ${ICU_SOURCE_DIR}/source/common/unicode/icudataver.h
  ${ICU_SOURCE_DIR}/source/common/unicode/icuplug.h
  ${ICU_SOURCE_DIR}/source/common/unicode/idna.h
  ${ICU_SOURCE_DIR}/source/common/unicode/localebuilder.h
  ${ICU_SOURCE_DIR}/source/common/unicode/localematcher.h
  ${ICU_SOURCE_DIR}/source/common/unicode/localpointer.h
  ${ICU_SOURCE_DIR}/source/common/unicode/locdspnm.h
  ${ICU_SOURCE_DIR}/source/common/unicode/locid.h
  ${ICU_SOURCE_DIR}/source/common/unicode/messagepattern.h
  ${ICU_SOURCE_DIR}/source/common/unicode/normalizer2.h
  ${ICU_SOURCE_DIR}/source/common/unicode/normlzr.h
  ${ICU_SOURCE_DIR}/source/common/unicode/parseerr.h
  ${ICU_SOURCE_DIR}/source/common/unicode/parsepos.h
  ${ICU_SOURCE_DIR}/source/common/unicode/platform.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ptypes.h
  ${ICU_SOURCE_DIR}/source/common/unicode/putil.h
  ${ICU_SOURCE_DIR}/source/common/unicode/rbbi.h
  ${ICU_SOURCE_DIR}/source/common/unicode/rep.h
  ${ICU_SOURCE_DIR}/source/common/unicode/resbund.h
  ${ICU_SOURCE_DIR}/source/common/unicode/schriter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/simpleformatter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/std_string.h
  ${ICU_SOURCE_DIR}/source/common/unicode/strenum.h
  ${ICU_SOURCE_DIR}/source/common/unicode/stringoptions.h
  ${ICU_SOURCE_DIR}/source/common/unicode/stringpiece.h
  ${ICU_SOURCE_DIR}/source/common/unicode/stringtriebuilder.h
  ${ICU_SOURCE_DIR}/source/common/unicode/symtable.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ubidi.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ubiditransform.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ubrk.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucasemap.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucat.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uchar.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucharstrie.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucharstriebuilder.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uchriter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uclean.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucnv.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucnv_cb.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucnv_err.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucnvsel.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uconfig.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucpmap.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucptrie.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ucurr.h
  ${ICU_SOURCE_DIR}/source/common/unicode/udata.h
  ${ICU_SOURCE_DIR}/source/common/unicode/udisplaycontext.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uenum.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uidna.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uiter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uldnames.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uloc.h
  ${ICU_SOURCE_DIR}/source/common/unicode/umachine.h
  ${ICU_SOURCE_DIR}/source/common/unicode/umisc.h
  ${ICU_SOURCE_DIR}/source/common/unicode/umutablecptrie.h
  ${ICU_SOURCE_DIR}/source/common/unicode/unifilt.h
  ${ICU_SOURCE_DIR}/source/common/unicode/unifunct.h
  ${ICU_SOURCE_DIR}/source/common/unicode/unimatch.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uniset.h
  ${ICU_SOURCE_DIR}/source/common/unicode/unistr.h
  ${ICU_SOURCE_DIR}/source/common/unicode/unorm.h
  ${ICU_SOURCE_DIR}/source/common/unicode/unorm2.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uobject.h
  ${ICU_SOURCE_DIR}/source/common/unicode/urename.h
  ${ICU_SOURCE_DIR}/source/common/unicode/urep.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ures.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uscript.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uset.h
  ${ICU_SOURCE_DIR}/source/common/unicode/usetiter.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ushape.h
  ${ICU_SOURCE_DIR}/source/common/unicode/usprep.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ustring.h
  ${ICU_SOURCE_DIR}/source/common/unicode/ustringtrie.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utext.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utf.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utf16.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utf32.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utf8.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utf_old.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utrace.h
  ${ICU_SOURCE_DIR}/source/common/unicode/utypes.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uvernum.h
  ${ICU_SOURCE_DIR}/source/common/unicode/uversion.h
)
target_compile_options(icu PRIVATE -fno-exceptions)
target_compile_definitions(icu PRIVATE
  -DU_COMMON_IMPLEMENTATION
  -DU_COMBINED_IMPLEMENTATION
  )
target_include_directories(icu PUBLIC
  $<BUILD_INTERFACE:${ICU_SOURCE_DIR}>/source/common
  $<BUILD_INTERFACE:${ICU_SOURCE_DIR}>/source/i18n
)

CPMAddPackage(
  NAME BOOST
  VERSION 1.80.0
  URL https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_1_80_0.tar.gz
  URL_HASH SHA256=4b2136f98bdd1f5857f1c3dea9ac2018effe65286cf251534b6ae20cc45e1847
  DOWNLOAD_ONLY YES
)

add_library(boost STATIC
  ${BOOST_SOURCE_DIR}/libs/locale/src/boost/locale/util/default_locale.cpp
  )

target_compile_definitions(boost PRIVATE
    -DBOOST_THREAD_NO_LIB=1
    -DBOOST_LOCALE_NO_POSIX_BACKEND=1
    -DBOOST_LOCALE_NO_WINAPI_BACKEND=1
    -DBOOST_LOCALE_NO_STD_BACKEND=1
    -DBOOST_LOCALE_WITH_ICONV=1
    -DBOOST_LOCALE_WITH_ICU=1
  )
target_link_libraries(boost icu)
target_include_directories(boost
  SYSTEM PUBLIC ${BOOST_SOURCE_DIR}
  PRIVATE ${BOOST_SOURCE_DIR}/libs/locale/src)

CPMAddPackage(
  NAME luajit
  GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT
  GIT_TAG 03080b795aa3496ed62d4a0697c9f4767e7ca7e5
  VERSION 2.1.0
  DOWNLOAD_ONLY YES
)

if(luajit_ADDED)
  file(COPY
    ${luajit_SOURCE_DIR}/src/lauxlib.h
    ${luajit_SOURCE_DIR}/src/lua.h
    ${luajit_SOURCE_DIR}/src/lualib.h
    ${luajit_SOURCE_DIR}/src/lua.hpp
    ${luajit_SOURCE_DIR}/src/luaconf.h
    ${luajit_SOURCE_DIR}/src/luajit.h
    DESTINATION ${luajit_BINARY_DIR}/include)

  add_executable(minilua ${luajit_SOURCE_DIR}/src/host/minilua.c)
  set(BUILDVM_ARCH_H ${luajit_BINARY_DIR}/buildvm_arch.h)
  add_custom_command(
    OUTPUT ${BUILDVM_ARCH_H}
    COMMAND minilua ${luajit_SOURCE_DIR}/dynasm/dynasm.lua -D ENDIAN_LE -D P64 -D JIT -D FFI -D DUALNUM -D FPU -D HFABI -D VER=80 -o ${BUILDVM_ARCH_H} ${luajit_SOURCE_DIR}/src/vm_arm64.dasc
    DEPENDS minilua ${luajit_SOURCE_DIR}/dynasm/dynasm.lua)

  add_executable(luajit-buildvm
    ${luajit_SOURCE_DIR}/src/host/buildvm.c
    ${luajit_SOURCE_DIR}/src/host/buildvm_asm.c
    ${luajit_SOURCE_DIR}/src/host/buildvm_fold.c
    ${luajit_SOURCE_DIR}/src/host/buildvm_lib.c
    ${luajit_SOURCE_DIR}/src/host/buildvm_peobj.c
    ${BUILDVM_ARCH_H}
    )
  target_compile_definitions(luajit-buildvm PRIVATE -DLUAJIT_ENABLE_LUA52COMPAT)
  target_include_directories(luajit-buildvm PRIVATE ${luajit_SOURCE_DIR}/src ${luajit_BINARY_DIR})

  set(LUAJIT_LIB_SOURCES
    ${luajit_SOURCE_DIR}/src/lib_base.c
    ${luajit_SOURCE_DIR}/src/lib_bit.c
    ${luajit_SOURCE_DIR}/src/lib_buffer.c
    ${luajit_SOURCE_DIR}/src/lib_debug.c
    ${luajit_SOURCE_DIR}/src/lib_ffi.c
    ${luajit_SOURCE_DIR}/src/lib_io.c
    ${luajit_SOURCE_DIR}/src/lib_jit.c
    ${luajit_SOURCE_DIR}/src/lib_math.c
    ${luajit_SOURCE_DIR}/src/lib_os.c
    ${luajit_SOURCE_DIR}/src/lib_package.c
    ${luajit_SOURCE_DIR}/src/lib_string.c
    ${luajit_SOURCE_DIR}/src/lib_table.c)
  set(LUAJIT_FOLDDEF_PATH ${luajit_BINARY_DIR}/lj_folddef.h)
  set(LUAJIT_GENERATED_HEADERS ${LUAJIT_FOLDDEF_PATH})

  macro(buildvm_header name)
    set(lj_header ${luajit_BINARY_DIR}/lj_${name}.h)
    list(APPEND LUAJIT_GENERATED_HEADERS ${lj_header})
    add_custom_command(
      OUTPUT ${lj_header}
      COMMAND luajit-buildvm -m ${name} -o ${lj_header} ${LUAJIT_LIB_SOURCES}
      DEPENDS luajit-buildvm ${LUAJIT_LIB_SOURCES})
  endmacro()
  buildvm_header(recdef)
  buildvm_header(ffdef)
  buildvm_header(bcdef)
  buildvm_header(vmdef)
  buildvm_header(libdef)

  add_custom_command(
    OUTPUT ${LUAJIT_FOLDDEF_PATH}
    COMMAND luajit-buildvm -m folddef -o ${LUAJIT_FOLDDEF_PATH} ${luajit_SOURCE_DIR}/src/lj_opt_fold.c
    DEPENDS luajit-buildvm ${luajit_SOURCE_DIR}/src/lj_opt_fold.c)
  add_custom_command(
    OUTPUT ${luajit_BINARY_DIR}/lj_vm.S
    COMMAND luajit-buildvm -m machasm -o ${luajit_BINARY_DIR}/lj_vm.S
    DEPENDS luajit-buildvm)
  add_custom_target(luajit_generate_headers DEPENDS ${LUAJIT_GENERATED_HEADERS} ${luajit_BINARY_DIR}/lj_vm.S)

  add_library(luajit STATIC
    ${luajit_SOURCE_DIR}/src/ljamalg.c
    ${luajit_BINARY_DIR}/lj_vm.S
    ${luajit_BINARY_DIR}/include/lauxlib.h
    ${luajit_BINARY_DIR}/include/lua.h
    ${luajit_BINARY_DIR}/include/lualib.h
    ${luajit_BINARY_DIR}/include/lua.hpp
    ${luajit_BINARY_DIR}/include/luaconf.h
    ${luajit_BINARY_DIR}/include/luajit.h
    )
  add_dependencies(luajit luajit_generate_headers)
  target_compile_definitions(luajit PRIVATE -DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_UNWIND_EXTERNAL)
  target_compile_options(luajit PRIVATE -fexceptions -funwind-tables -fomit-frame-pointer)
  target_include_directories(luajit PRIVATE ${luajit_SOURCE_DIR}/src ${luajit_BINARY_DIR})
  target_include_directories(luajit PUBLIC ${luajit_BINARY_DIR}/include)

  add_executable(luajit-aegisub ${luajit_SOURCE_DIR}/src/luajit.c)
  target_link_libraries(luajit-aegisub luajit)
endif()

CPMAddPackage(
  NAME luabins
  GIT_REPOSITORY https://github.com/agladysh/luabins
  VERSION 0.3
  DOWNLOAD_ONLY YES
)
if(luabins_ADDED)
  add_library(luabins STATIC
    ${luabins_SOURCE_DIR}/src/fwrite.c
    ${luabins_SOURCE_DIR}/src/load.c
    ${luabins_SOURCE_DIR}/src/luabins.c
    ${luabins_SOURCE_DIR}/src/luainternals.c
    ${luabins_SOURCE_DIR}/src/savebuffer.c
    ${luabins_SOURCE_DIR}/src/save.c
    ${luabins_SOURCE_DIR}/src/write.c
    )
  target_compile_definitions(luabins PRIVATE -DluaL_reg=luaL_Reg -DLUAI_BITSINT=32)
  target_link_libraries(luabins PRIVATE luajit)
endif()

CPMAddPackage(
  NAME wxWidgets
  GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets
  VERSION 3.2.0
)

CPMAddPackage(
  NAME harfbuzz
  GIT_TAG 5.3.1
  GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz
  DOWNLOAD_ONLY YES
)

if(harfbuzz_ADDED)
  add_library(harfbuzz STATIC
    ${harfbuzz_SOURCE_DIR}/src/harfbuzz.cc)
  target_link_libraries(harfbuzz freetype)
  target_include_directories(harfbuzz PUBLIC ${harfbuzz_SOURCE_DIR}/src)
endif()

CPMAddPackage(
  NAME ass
  GIT_REPOSITORY https://github.com/libass/libass
  GIT_TAG 0.16.0
  DOWNLOAD_ONLY YES
)
if(ass_ADDED)
  file(COPY
    ${ass_SOURCE_DIR}/libass/ass.h
    ${ass_SOURCE_DIR}/libass/ass_types.h
    DESTINATION ${ass_BINARY_DIR}/include/ass)
  configure_file(thirdparty/libass/config.h.in
    ${ass_BINARY_DIR}/include/config.h)
  add_library(ass STATIC
    ${ass_SOURCE_DIR}/libass/ass.c
    ${ass_SOURCE_DIR}/libass/ass.h
    ${ass_SOURCE_DIR}/libass/ass_bitmap.c
    ${ass_SOURCE_DIR}/libass/ass_bitmap.h
    ${ass_SOURCE_DIR}/libass/ass_blur.c
    ${ass_SOURCE_DIR}/libass/ass_cache.c
    ${ass_SOURCE_DIR}/libass/ass_cache.h
    ${ass_SOURCE_DIR}/libass/ass_cache_template.h
    ${ass_SOURCE_DIR}/libass/ass_compat.h
    ${ass_SOURCE_DIR}/libass/ass_drawing.c
    ${ass_SOURCE_DIR}/libass/ass_drawing.h
    ${ass_SOURCE_DIR}/libass/ass_filesystem.c
    ${ass_SOURCE_DIR}/libass/ass_filesystem.h
    ${ass_SOURCE_DIR}/libass/ass_font.c
    ${ass_SOURCE_DIR}/libass/ass_font.h
    ${ass_SOURCE_DIR}/libass/ass_fontselect.c
    ${ass_SOURCE_DIR}/libass/ass_fontselect.h
    ${ass_SOURCE_DIR}/libass/ass_func_template.h
    ${ass_SOURCE_DIR}/libass/ass_library.c
    ${ass_SOURCE_DIR}/libass/ass_library.h
    ${ass_SOURCE_DIR}/libass/ass_outline.c
    ${ass_SOURCE_DIR}/libass/ass_outline.h
    ${ass_SOURCE_DIR}/libass/ass_parse.c
    ${ass_SOURCE_DIR}/libass/ass_parse.h
    ${ass_SOURCE_DIR}/libass/ass_priv.h
    ${ass_SOURCE_DIR}/libass/ass_rasterizer.c
    ${ass_SOURCE_DIR}/libass/ass_rasterizer.h
    ${ass_SOURCE_DIR}/libass/ass_rasterizer_c.c
    ${ass_SOURCE_DIR}/libass/ass_render.c
    ${ass_SOURCE_DIR}/libass/ass_render.h
    ${ass_SOURCE_DIR}/libass/ass_render_api.c
    ${ass_SOURCE_DIR}/libass/ass_shaper.c
    ${ass_SOURCE_DIR}/libass/ass_shaper.h
    ${ass_SOURCE_DIR}/libass/ass_string.c
    ${ass_SOURCE_DIR}/libass/ass_string.h
    ${ass_SOURCE_DIR}/libass/ass_strtod.c
    ${ass_SOURCE_DIR}/libass/ass_types.h
    ${ass_SOURCE_DIR}/libass/ass_utils.c
    ${ass_SOURCE_DIR}/libass/ass_utils.h
    ${ass_SOURCE_DIR}/libass/wyhash.h

    ${ass_SOURCE_DIR}/libass/ass_coretext.c
    ${ass_SOURCE_DIR}/libass/ass_coretext.h
    )
  # target_compile_definitions(ass PRIVATE -DluaL_reg=luaL_Reg -DLUAI_BITSINT=32)
  target_link_libraries(ass PRIVATE freetype fribidi harfbuzz)
  target_include_directories(ass PUBLIC ${ass_BINARY_DIR}/include)
endif()

add_subdirectory(automation)
add_subdirectory(libaegisub)
# add_subdirectory(packages)
# add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)
