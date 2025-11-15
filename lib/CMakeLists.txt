file(GLOB DEF_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.def")
cmake_path(CONVERT "${DEF_FILES}" TO_NATIVE_PATH_LIST DEF_FILES)

foreach(DEF_FILE ${DEF_FILES})
    get_filename_component(DEF_NAME_WE "${DEF_FILE}" NAME_WE)

    execute_process(
        COMMAND lib
            /machine:x64
            "/def:${DEF_FILE}"
            "/out:${CMAKE_CURRENT_SOURCE_DIR}\\${DEF_NAME_WE}.lib"
        RESULT_VARIABLE LIB_RESULT
        ERROR_VARIABLE LIB_ERROR)
    
    if(NOT LIB_RESULT EQUAL 0)
        message(FATAL_ERROR "${LIB_ERROR}")
    endif()
endforeach()
