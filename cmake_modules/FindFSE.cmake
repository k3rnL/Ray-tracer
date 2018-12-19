# This module defines
# FSE_LIBRARY, the name of the library to link against
# FSE_FOUND, if false, do not try to link to FSE
# FSE_INCLUDE_DIR, where to find SDL.h
#


SET(FSE_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	${FSE_PATH}
)

FIND_PATH(FSE_INCLUDE_DIR fse
	HINTS
	$ENV{FSEDIR}
	PATH_SUFFIXES include
	PATHS ${FSE_SEARCH_PATHS}
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(PATH_SUFFIXES lib64 lib/x64 lib)
else()
	set(PATH_SUFFIXES lib/x86 lib)
endif()

FIND_LIBRARY(FSE_LIBRARY_TEMP
	NAMES fse
	HINTS
	$ENV{FSEDIR}
	PATH_SUFFIXES ${PATH_SUFFIXES}
	PATHS ${FSE_SEARCH_PATHS}
)

# FSE may require threads on your system.
# The Apple build may not need an explicit flag because one of the
# frameworks may already provide it.
# But for non-OSX systems, I will use the CMake Threads package.
IF(NOT APPLE)
	FIND_PACKAGE(Threads)
ENDIF(NOT APPLE)

# MinGW needs an additional link flag, -mwindows
# It's total link flags should look like -lmingw32 -lFSEmain -lFSE -mwindows
IF(MINGW)
	SET(MINGW32_LIBRARY mingw32 "-mwindows" CACHE STRING "mwindows for MinGW")
ENDIF(MINGW)

	# Set the final string here so the GUI reflects the final state.
	SET(FSE_LIBRARY ${FSE_LIBRARY_TEMP} CACHE STRING "Where the FSE Library can be found")
	# Set the temp variable to INTERNAL so it is not seen in the CMake GUI
	SET(FSE_LIBRARY_TEMP "${FSE_LIBRARY_TEMP}" CACHE INTERNAL "")

# message("</FindFSE.cmake>")

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(FSE REQUIRED_VARS FSE_LIBRARY FSE_INCLUDE_DIR)
