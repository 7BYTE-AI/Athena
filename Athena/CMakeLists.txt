cmake_minimum_required (VERSION 3.24)

function(SetBuildDirectory TARGET_NAME)
    set_target_properties(${TARGET_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/ThirdParty/${TARGET_NAME}"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/ThirdParty/${TARGET_NAME}"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/ThirdParty/${TARGET_NAME}")
endfunction()

function(AddLibrary TARGET_NAME TARGET_PATH)
    add_subdirectory("${TARGET_PATH}")
    EnableMPCompilation(${TARGET_NAME})
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER Dependencies)
	SetBuildDirectory(${TARGET_NAME})

    target_link_libraries(Athena PRIVATE ${TARGET_NAME})
endfunction()

######################################
##### MAIN PROJECT
######################################

project(Athena)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	Athena.def
	"Source/*.h" 
	"Source/*.cpp" 
	"ThirdParty/filewatch/FileWatch.h"
	"ThirdParty/entt/entt.h"
	"ThirdParty/ImGuizmo/ImGuizmo.h"
	"ThirdParty/ImGuizmo/ImGuizmo.cpp"
	"ThirdParty/portable-file-dialogs/portable-file-dialogs.h"
	"ThirdParty/stb_image/stb_image.h"
	"ThirdParty/stb_image/stb_image_write.h"
	"ThirdParty/stb_image/stb_image.cpp"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

add_library(Athena SHARED ${SRC_FILES})

EnableMPCompilation(Athena)
set_target_properties(Athena
	PROPERTIES
	CXX_STANDARD 20
	FOLDER Core
	DEBUG_POSTFIX ""
)

target_compile_definitions(Athena PUBLIC
	$<$<CONFIG:Debug>:ATN_DEBUG>
	$<$<CONFIG:Release>:ATN_RELEASE>
	$<$<CONFIG:Dist>:ATN_DIST>
	ATN_SIMD
	STBIW_WINDOWS_UTF8
	_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
)

target_compile_definitions(Athena PRIVATE
	ATN_BUILD_DLL
)

set_target_properties(Athena
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena"
)

# Disable some warnings
# 4099 - require debug info
target_link_options(Athena PRIVATE /ignore:4099)

add_custom_command(TARGET Athena POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:Athena>"
     "${BUILD_FOLDER}/Athena-Editor/$<CONFIGURATION>/$<TARGET_FILE_NAME:Athena>" )

add_custom_command(TARGET Athena POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:Athena>"
     "${BUILD_FOLDER}/SandBox/$<CONFIGURATION>/$<TARGET_FILE_NAME:Athena>" )


# Precompile Header
target_precompile_headers(Athena PRIVATE
	<memory> <algorithm> <functional> <utility> <chrono>
	<iomanip> <thread> <filesystem> <limits> <random>
	<numeric> <math.h> <cmath> <iostream> <sstream>
	<fstream> <array> <vector> <string> <unordered_map> <unordered_set>

	Source/Athena/Core/Core.h
	Source/Athena/Core/Log.h
	Source/Athena/Core/Memory.h
	Source/Athena/Core/Profile.h
)

target_include_directories(Athena PRIVATE 
	"Source" 
)

######################################
##### DEPENDENCIES
######################################

IncludeDependenciesDirs(Athena)

AddLibrary("Box2D" "${THIRD_PARTY_DIR}/Box2D")
AddLibrary(GLFW "${THIRD_PARTY_DIR}/GLFW")
AddLibrary(ImGui "${THIRD_PARTY_DIR}/ImGui")
AddLibrary(spdlog "${THIRD_PARTY_DIR}/spdlog")

AddLibrary(yaml-cpp "${THIRD_PARTY_DIR}/yaml-cpp")
target_compile_definitions(yaml-cpp PUBLIC YAML_CPP_STATIC_DEFINE) # Static compile

AddLibrary(OptickCore "${THIRD_PARTY_DIR}/Optick/OptickCore")

AddLibrary(msdf-atlas-gen "${THIRD_PARTY_DIR}/msdf-atlas-gen")
set_property(TARGET msdf-atlas-gen PROPERTY FOLDER "Dependencies/msdfgen")

EnableMPCompilation(msdfgen)
set_property(TARGET msdfgen PROPERTY FOLDER "Dependencies/msdfgen")
SetBuildDirectory(msdfgen)
target_compile_definitions(Athena PUBLIC MSDFGEN_PUBLIC=)

EnableMPCompilation(freetype)
set_property(TARGET freetype PROPERTY FOLDER "Dependencies/msdfgen")
SetBuildDirectory(freetype)


if (Vulkan_FOUND)
	target_link_libraries(Athena PRIVATE Vulkan::Vulkan Vulkan::shaderc_combined)

	# Vulkan::SPIRV-Tools does not link required libs, so do this instead
	target_link_libraries(Athena PRIVATE
		$<$<CONFIG:Debug>: ${VULKAN_SDK_PATH}/Lib/spirv-cross-cored.lib>
		$<$<CONFIG:Release>: ${VULKAN_SDK_PATH}/Lib/spirv-cross-core.lib>
		$<$<CONFIG:Dist>: ${VULKAN_SDK_PATH}/Lib/spirv-cross-core.lib>)

	target_link_libraries(Athena PRIVATE
		$<$<CONFIG:Debug>: ${VULKAN_SDK_PATH}/Lib/shaderc_utild.lib>
		$<$<CONFIG:Release>: ${VULKAN_SDK_PATH}/Lib/shaderc_util.lib>
		$<$<CONFIG:Dist>: ${VULKAN_SDK_PATH}/Lib/shaderc_util.lib>)

endif(Vulkan_FOUND)

# Windows specific code
if(WIN32)
	target_link_libraries(Athena PRIVATE "${THIRD_PARTY_DIR}/assimp/libs/Windows/assimp-vc143-mt.lib")
	target_compile_definitions(Athena PUBLIC ATN_PLATFORM_WINDOWS UNICODE)
endif(WIN32)
