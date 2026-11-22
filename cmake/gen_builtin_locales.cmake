# Generates a C++ header embedding every locales/*.json file as raw UTF-8 byte arrays
# so locales/*.json is the single source of truth while binaries remain self-contained.
#
# Invoked at build/configure time with:
#   -DLOCALES_DIR=<path/to/locales>
#   -DOUTPUT_HEADER=<path/to/builtin_locales.hpp>

file(GLOB _locale_files "${LOCALES_DIR}/*.json")
list(SORT _locale_files)

set(_out "// Auto-generated from locales/*.json — do not edit by hand.\n")
string(APPEND _out "#pragma once\n")
string(APPEND _out "#include <cstddef>\n\n")
string(APPEND _out "namespace llk2::core::detail {\n\n")

set(_table_entries "")
set(_idx 0)
foreach(_f IN LISTS _locale_files)
    get_filename_component(_lang "${_f}" NAME_WE)
    file(READ "${_f}" _hex HEX)
    string(LENGTH "${_hex}" _hex_len)
    math(EXPR _byte_len "${_hex_len} / 2")

    if(_byte_len GREATER 0)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
        string(APPEND _out "inline constexpr unsigned char kLocaleData_${_idx}[] = {\n${_bytes}\n};\n")
        string(APPEND _table_entries "    {\"${_lang}\", kLocaleData_${_idx}, ${_byte_len}},\n")
    endif()
    math(EXPR _idx "${_idx} + 1")
endforeach()

string(APPEND _out "\nstruct BuiltinLocaleBlob {\n")
string(APPEND _out "    const char* lang;\n")
string(APPEND _out "    const unsigned char* data;\n")
string(APPEND _out "    std::size_t size;\n")
string(APPEND _out "};\n\n")
string(APPEND _out "inline constexpr BuiltinLocaleBlob kBuiltinLocales[] = {\n")
string(APPEND _out "${_table_entries}")
string(APPEND _out "};\n\n")
string(APPEND _out "} // namespace llk2::core::detail\n")

get_filename_component(_out_dir "${OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

set(_write_needed TRUE)
if(EXISTS "${OUTPUT_HEADER}")
    file(READ "${OUTPUT_HEADER}" _existing)
    if(_existing STREQUAL _out)
        set(_write_needed FALSE)
    endif()
endif()

if(_write_needed)
    file(WRITE "${OUTPUT_HEADER}" "${_out}")
endif()
