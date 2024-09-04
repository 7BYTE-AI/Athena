cmake_minimum_required (VERSION 3.24)

######################################
##### CONFIG	
######################################

include(Config.cmake)

set(THIRD_PARTY_DIR "${ATHENA_SOURCE_DIR}/Athena/ThirdParty")
include("${ATHENA_SOURCE_DIR}/Dependencies.cmake")

set(BUILD_FOLDER "${CMAKE_SOURCE_DIR}/Build/Binaries") 
set(SHADERS_FOLDER "${ATHENA_SOURCE_DIR}/Athena/EngineResources/ShaderPack")

######################################
##### GLOBAL
######################################

project (${PROJECT_NAME})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)


######################################
##### MAIN PROJECT
######################################

file(GLOB_RECURSE SRC_SCRIPT_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_SCRIPT_FILES} )

add_library(${PROJECT_NAME} SHARED ${SRC_SCRIPT_FILES})
EnableMPCompilation(${PROJECT_NAME})

# Option to use MD or MDd 
# TODO: support other compilers
if(USE_DEBUG_RUNTIME_LIBS)
	set(PROJECT_OUTPUT_NAME "ScriptsLibrary-d")
	if(MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE /MDd)
	endif(MSVC)
else(USE_DEBUG_RUNTIME_LIBS)
	set(PROJECT_OUTPUT_NAME "ScriptsLibrary")
	if(MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE /MD)
	endif(MSVC)
endif(USE_DEBUG_RUNTIME_LIBS)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	CXX_STANDARD 20
	OUTPUT_NAME ${PROJECT_OUTPUT_NAME}
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
	$<$<CONFIG:Debug>:ATN_DEBUG>
	$<$<CONFIG:Release>:ATN_RELEASE>
	ATN_SIMD
	STBIW_WINDOWS_UTF8
	_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
)

# Build folder (debug and release have the same dir)
set_target_properties(${PROJECT_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_FOLDER}/ScriptsLibrary"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_FOLDER}/ScriptsLibrary"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_FOLDER}/ScriptsLibrary"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_FOLDER}/ScriptsLibrary"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_FOLDER}/ScriptsLibrary"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_FOLDER}/ScriptsLibrary"
)

######################################
##### SHADERS PROJECT
######################################

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS 
	"${SHADERS_FOLDER}/*.glsl"
    "${SHADERS_FOLDER}/*.glslh"
    "${SHADERS_FOLDER}/*.hlsl"
    "${SHADERS_FOLDER}/*.hlsli")
source_group(TREE "${SHADERS_FOLDER}" FILES ${SHADER_FILES} )

add_library(Shaders ${SHADER_FILES})
set_target_properties(Shaders
	PROPERTIES
	LINKER_LANGUAGE CXX
	FOLDER Core
)

######################################
##### DEPENDENCIES
######################################

IncludeDependenciesDirs(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE 
	"${ATHENA_SOURCE_DIR}/Athena/Source" 
)

# Find Vulkan
find_package(Vulkan COMPONENTS shaderc_combined SPIRV-Tools REQUIRED)
if(Vulkan_FOUND)
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    message("-- Find VulkanSDK - ${VULKAN_SDK_PATH}")
else(Vulkan_FOUND)
	message("-- Failed to find VulkanSDK(Install VulkanSDK from official website or check if it is added to path)")
endif(Vulkan_FOUND)

macro(CopyDLLToBinaries DLL_PATH TARGET_NAME)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Release/${TARGET_NAME}" COPYONLY)
endmacro()

# copy assimp dlls
if(WIN32)  
    CopyDLLToBinaries("${THIRD_PARTY_DIR}/assimp/libs/Windows/assimp-vc143-mt.dll" "assimp-vc143-mt.dll")
	target_compile_definitions(${PROJECT_NAME} PUBLIC ATN_PLATFORM_WINDOWS UNICODE)
endif(WIN32)

target_link_libraries(${PROJECT_NAME} PRIVATE "${ATHENA_BINARY_DIR}/$<CONFIGURATION>/Athena.lib")

add_subdirectory("${ATHENA_SOURCE_DIR}/Athena" "${CMAKE_SOURCE_DIR}/Build/Projects/Athena")
