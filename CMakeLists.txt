project(LowEngineZero)


cmake_minimum_required(VERSION 3.10)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
set(TARGET_BUILD_PLATFORM "windows")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(APP_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/app)
set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/pdb)

# First for the generic no-config case (e.g. with mingw)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR} )
set(CMAKE_PDB_OUTPUT_DIRECTORY ${PDB_OUTPUT_DIR})
# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${APP_OUTPUT_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${LIB_OUTPUT_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${LIB_OUTPUT_DIR} )
    set( CMAKE_PDB_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PDB_OUTPUT_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


FILE(GLOB SUBFOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
SET(SUBPROJECTS "")
SET(FOUND "")

add_compile_definitions(LOW_EDITOR_BUILD LOW_DATA_PATH="../../data")

# Excluded directories
# These directorories are not part of the build tree
SET(SUB_EXCEPTIONS
  LowScripts
)

# Adds all subfolders that start with the prefix "Low" and are not
# listet as excluded to the SUBPROJECTS list
FOREACH(child ${SUBFOLDERS})
	      IF (child MATCHES "^Low")
	      	 IF (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
		    LIST(FIND SUB_EXCEPTIONS ${child} FOUND)

		    IF (FOUND LESS 0)
		    		    LIST(APPEND SUBPROJECTS ${child})
		    ENDIF()
		 ENDIF()
	ENDIF()
ENDFOREACH()

# Adds the game subdirectory to the list of subprojects
#LIST(APPEND SUBPROJECTS zero)
#LIST(APPEND SUBPROJECTS gamestart)

SET(LINK_DIRS "")
SET(SUBPROJECTS_LOWERCASE "")
SET(PROJECT_LINK_LIBS "")

# Adds subproject as subdirectory to cmake buildtree
FOREACH(project ${SUBPROJECTS})
  add_subdirectory(${project})
ENDFOREACH()


