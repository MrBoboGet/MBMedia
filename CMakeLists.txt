project(MBMedia)
set(PROJECT_NAME "MBMedia")
##BEGIN MBPM_VARIABLES
set(MBPM_DEPENDENCIES 
	#ffmpeg
	MBUtility
)
set(MBPM_TARGET_EXTPACKET_LIBRARIES )
set(MBPM_TARGET_COMPILE_OPTIONS )
set(MBPM_TARGET_LINK_OPTIONS )
#MBPM_Functions#variables MBPM_DEPENDENCIES
#MBPM_TARGET_EXTPACKET_LIBRARIES
#MBPM_TARGET_COMPILE_OPTIONS
#MBPM_TARGET_LINK_OPTIONS
#Environment variables MBPM_PACKETS_INSTALL_DIRECTORY
set(MBPM_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(MBPM_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
set(MBPM_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
macro(MBPM_UpdateLibraries TargetToUpdate LinkType BuildType)
	set(MBPM_TARGET_EXTPACKET_LIBRARIES "")
	foreach(PACKET IN LISTS MBPM_DEPENDENCIES)
		set(NewLibraryName "")
		set(NewLibraryToAdd "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/${PACKET}/MBPM_Builds/")
		if(${LinkType} STREQUAL "STATIC")
			set(NewLibraryName "${PACKET}_S")
		elseif(${LinkType} STREQUAL "DYNAMIC")
			set(NewLibraryName "${PACKET}_D")
		endif()
		if(${BuildType} STREQUAL "DEBUG")
			set(NewLibraryName "${NewLibraryName}D")
		elseif(${BuildType} STREQUAL "RELEASE")
			set(NewLibraryName "${NewLibraryName}R")
		endif()
		if(UNIX)
			set(NewLibraryName "lib${NewLibraryName}.")
			if(${LinkType} STREQUAL "STATIC")
				set(NewLibraryName "${NewLibraryName}a")
			else()
				set(NewLibraryName "${NewLibraryName}so")
			endif()
		else()
			if(${LinkType} STREQUAL "STATIC")
				set(NewLibraryName "${NewLibraryName}.lib")
			else()
				set(NewLibraryName "${NewLibraryName}.dll")				
			endif()
		endif()
		set(NewLibraryToAdd "${NewLibraryToAdd}${NewLibraryName}")
		list(APPEND MBPM_TARGET_EXTPACKET_LIBRARIES ${NewLibraryToAdd})
	endforeach()
endmacro()

macro(MBPM_UpdateTargetVariables TargetToUpdate LinkType BuildType)
	MBPM_UpdateLibraries(${TargetToUpdate} ${LinkType} ${BuildType})
endmacro()

macro(MBPM_UpdateOutput TargetToUpdate)
	set_target_properties(${TargetToUpdate} 
	PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	)
	#set_target_properties(${TargetToUpdate} PROPERTIES PREFIX "")
	#set_target_properties(${TargetToUpdate} PROPERTIES SUFFIX "")
	set_target_properties(${TargetToUpdate} PROPERTIES OUTPUT_NAME ${TargetToUpdate})
endmacro()

macro(MBPM_SetTargetCompileOptions TargetToUpdate LinkType BuildType)
	set(CMAKE_CXX_FLAGS " ")
	set(CMAKE_CXX_FLAGS_DEBUG " ")
	set(CMAKE_CXX_FLAGS_RELEASE " ")
	set(CMAKE_C_FLAGS " ")
	set(CMAKE_C_FLAGS_DEBUG " ")
	set(CMAKE_C_FLAGS_RELEASE " ")

	if(${BuildType} STREQUAL "DEBUG" AND ${LinkType} STREQUAL "STATIC")
		set(NEW_TARGET_COMPILE_OPTIONS "${MBPM_CXX_FLAGS} ${MBPM_CXX_FLAGS_DEBUG}")
	else()
		set(NEW_TARGET_COMPILE_OPTIONS "${MBPM_CXX_FLAGS} ${MBPM_CXX_FLAGS_RELEASE}")
	endif()
	string(REPLACE " " ";" NEW_TARGET_COMPILE_OPTIONS_LIST ${NEW_TARGET_COMPILE_OPTIONS})
	target_compile_options(${TargetToUpdate} PRIVATE ${NEW_TARGET_COMPILE_OPTIONS_LIST})
	message(${TargetToUpdate} " Compiled with follwing flags: ""${NEW_TARGET_COMPILE_OPTIONS_LIST}")
endmacro()

macro(MBPM_LinkTargetPacketLibraries Target)
	target_link_libraries(${Target} ${MBPM_TARGET_EXTPACKET_LIBRARIES})
endmacro()

macro(MBPM_SetTargetIncludeDirectories Target)
	target_include_directories(${Target} PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	"$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"
	"$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBUtility/"
	)
endmacro()

macro(MBPM_ApplyTargetConfiguration TargetToUpdate LinkType BuildType)
	MBPM_UpdateTargetVariables(${TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_UpdateOutput(${TargetToUpdate})
	MBPM_SetTargetCompileOptions(${TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_SetTargetIncludeDirectories(${TargetToUpdate})
endmacro()
##END MBPM_VARIABLES
set(PROJECT_SOURCES 

	"${CMAKE_CURRENT_SOURCE_DIR}/MBMedia.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ContainerMuxing.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/MBAudioUtility.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/MBMediaInternals.cpp"
)
set(PROJECT_HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}//MBMedia.h"
)
set(COMMON_INCLUDE_DIRECTORIES
   "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/" 
)
set(COMMON_FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS})
set(COMMON_DYNAMIC_LIBRARIES 
)
set (FFMPEG_LIBS
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libavformat/libavformat.lib"
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libavcodec/libavcodec.lib"
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libavdevice/libavdevice.lib"
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libavfilter/libavfilter.lib"
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libavutil/libavutil.lib"
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libswresample/libswresample.lib"
    "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg/libswscale/libswscale.lib"
)
set(COMMON_STATIC_LIBRARIES
    ${FFMPEG_LIBS}
	"Ws2_32.lib"
	"Secur32.lib"
    "Bcrypt.lib"
    "Mfplat.lib"
    "Mfuuid.lib"
    "Strmiids.lib"
)
set(COMMON_LIBRARIES ${COMMON_STATIC_LIBRARIES} ${COMMON_DYNAMIC_LIBRARIES})

set(EXECUTABLE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/MBMediaTest.cpp"
)

add_library(MBMedia_SD STATIC ${COMMON_FILES} "MBMediaTest.cpp" "MBMedia.h" "ContainerMuxing.cpp" "MBAudioUtility.cpp" "MBMediaInternals.cpp")
MBPM_ApplyTargetConfiguration("MBMedia_SD" "STATIC" "DEBUG")
target_compile_features("MBMedia_SD" PRIVATE cxx_std_17)
target_link_libraries("MBMedia_SD" ${COMMON_LIBRARIES})
target_include_directories("MBMedia_SD" PRIVATE ${COMMON_INCLUDE_DIRECTORIES})

add_library(MBMedia_SR STATIC ${COMMON_FILES} "MBMediaTest.cpp" "MBMedia.h" "ContainerMuxing.cpp" "MBAudioUtility.cpp" "MBMediaInternals.cpp")
MBPM_ApplyTargetConfiguration("MBMedia_SR" "STATIC" "RELEASE")
target_compile_features("MBMedia_SR" PRIVATE cxx_std_17)
target_link_libraries("MBMedia_SR" ${COMMON_LIBRARIES})
target_include_directories("MBMedia_SR" PRIVATE ${COMMON_INCLUDE_DIRECTORIES})

add_executable(MBMediaTest ${COMMON_FILES} ${EXECUTABLE_FILES} "MBMedia.h" "ContainerMuxing.cpp" "MBAudioUtility.cpp" "MBMediaInternals.cpp")
MBPM_ApplyTargetConfiguration("MBMediaTest" "STATIC" "DEBUG")
target_compile_features("MBMediaTest" PRIVATE cxx_std_17)
target_link_libraries("MBMediaTest" ${COMMON_LIBRARIES})
MBPM_LinkTargetPacketLibraries("MBMediaTest")
target_include_directories("MBMediaTest" PRIVATE ${COMMON_INCLUDE_DIRECTORIES})
