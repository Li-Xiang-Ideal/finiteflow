# Taken from symengine and modified by T.P.

include(LibFindMacros)

libfind_library(gmp gmp)
set(GMP_LIBRARIES ${GMP_LIBRARY})
set(GMP_TARGETS gmp)

libfind_include(gmp.h gmp)
set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_LIBRARIES
    GMP_INCLUDE_DIRS)

mark_as_advanced(GMP_INCLUDE_DIR GMP_LIBRARY)

# Fix for MinGW: cloak the GMP DLL to avoid name conflicts
option(CLOAK_GMP_DLL "Cloak GMP DLL on MinGW to avoid name conflicts?" OFF)

if(MINGW AND GMP_FOUND AND CLOAK_GMP_DLL)
    message(STATUS "MinGW detected. Initializing GMP cloaking logic...")

    find_program(GENDEF_EXECUTABLE gendef)
    find_program(DLLTOOL_EXECUTABLE dlltool)

    get_filename_component(_GMP_LIB_DIR "${GMP_LIBRARY}" DIRECTORY)
    set(_GMP_BIN_DIR "${_GMP_LIB_DIR}/../bin")

    file(GLOB _ALL_CANDIDATES "${_GMP_BIN_DIR}/libgmp*.dll")
    
    set(_VALID_SYSTEM_DLLS "")

    foreach(_candidate ${_ALL_CANDIDATES})
        get_filename_component(_fname "${_candidate}" NAME)
        if(_fname MATCHES "^libgmp[^A-Za-z]*\\.dll$")
            list(APPEND _VALID_SYSTEM_DLLS "${_candidate}")
        endif()
    endforeach()

    list(LENGTH _VALID_SYSTEM_DLLS _NUM_VALID)
    if(_NUM_VALID GREATER 0 AND GENDEF_EXECUTABLE AND DLLTOOL_EXECUTABLE)
        list(SORT _VALID_SYSTEM_DLLS) 
        list(REVERSE _VALID_SYSTEM_DLLS)
        list(GET _VALID_SYSTEM_DLLS 0 SYSTEM_GMP_DLL)

        message(STATUS "Found System GMP DLL: ${SYSTEM_GMP_DLL}")

        get_filename_component(_ORIG_NAME "${SYSTEM_GMP_DLL}" NAME)
        string(REGEX REPLACE "^libgmp" "libgmp-fflow" CLOAKED_GMP_NAME "${_ORIG_NAME}")
        
        set(CLOAKED_DLL "${CMAKE_BINARY_DIR}/${CLOAKED_GMP_NAME}")
        set(CLOAKED_LIB "${CMAKE_BINARY_DIR}/${CLOAKED_GMP_NAME}.a")
        set(CLOAKED_DEF "${CMAKE_BINARY_DIR}/${CLOAKED_GMP_NAME}.def")

        message(STATUS "Cloaking target: ${CLOAKED_GMP_NAME}")

        add_custom_command(
            OUTPUT "${CLOAKED_LIB}" "${CLOAKED_DLL}"
            COMMAND ${CMAKE_COMMAND} -E copy "${SYSTEM_GMP_DLL}" "${CLOAKED_DLL}"
            COMMAND "${GENDEF_EXECUTABLE}" - "${SYSTEM_GMP_DLL}" > "${CLOAKED_DEF}"
            COMMAND "${DLLTOOL_EXECUTABLE}" -d "${CLOAKED_DEF}" -l "${CLOAKED_LIB}" -D "${CLOAKED_GMP_NAME}"
            DEPENDS "${SYSTEM_GMP_DLL}"
            COMMENT "Cloaking GMP: ${_ORIG_NAME} -> ${CLOAKED_GMP_NAME}"
            VERBATIM
        )

        if(NOT TARGET gmp_cloaking_job)
            add_custom_target(gmp_cloaking_job ALL DEPENDS "${CLOAKED_LIB}")
        endif()

        set(GMP_LIBRARIES "${CLOAKED_LIB}")
        set(GMP_TARGETS "${CLOAKED_LIB}")
        
        set(GMP_CLOAKED_DLL_PATH "${CLOAKED_DLL}" CACHE INTERNAL "Path to cloaked GMP DLL")
        set(GMP_CLOAKED_ENABLED TRUE CACHE INTERNAL "Is GMP cloaking enabled?")

        install(FILES "${CLOAKED_DLL}" DESTINATION bin)
        
    else()
        if(NOT GENDEF_EXECUTABLE OR NOT DLLTOOL_EXECUTABLE)
             message(WARNING "Cloaking skipped: 'gendef' or 'dlltool' not found.")
        elseif(_NUM_VALID EQUAL 0)
             message(WARNING "Cloaking skipped: No valid 'libgmp[^A-Za-z]*.dll' found in ${_GMP_BIN_DIR}. (Found candidates: ${_ALL_CANDIDATES})")
        endif()
    endif()
endif()