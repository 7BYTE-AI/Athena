# Third Party 
set(ASSIMP_INCLUDE_DIR "${THIRD_PARTY_DIR}/assimp/include")
set(BOX2D_INCLUDE_DIR "${THIRD_PARTY_DIR}/Box2D/include")
set(ENTT_INCLUDE_DIR "${THIRD_PARTY_DIR}/entt")
set(FILEWATCH_INCLUDE_DIR "${THIRD_PARTY_DIR}/filewatch")
set(GLFW_INCLUDE_DIR "${THIRD_PARTY_DIR}/GLFW/include")
set(IMGUI_INCLUDE_DIR "${THIRD_PARTY_DIR}/ImGui")
set(IMGUIZMO_INCLUDE_DIR "${THIRD_PARTY_DIR}/ImGuizmo")
set(OPTICK_INCLUDE_DIR "${THIRD_PARTY_DIR}/Optick/OptickCore/src")
set(PFD_INCLUDE_DIR "${THIRD_PARTY_DIR}/portable-file-dialogs")
set(SPDLOG_INCLUDE_DIR "${THIRD_PARTY_DIR}/spdlog/include")
set(STBIMAGE_INCLUDE_DIR "${THIRD_PARTY_DIR}/stb_image/")
set(YAML_CPP_INCLUDE_DIR "${THIRD_PARTY_DIR}/yaml-cpp/include")
set(MSDF_ATLAS_GEN_INCLUDE_DIR "${THIRD_PARTY_DIR}/msdf-atlas-gen")
set(MSDF_GEN_INCLUDE_DIR "${THIRD_PARTY_DIR}/msdf-atlas-gen/msdfgen")


macro(IncludeDependenciesDirs TARGET_NAME)
	target_include_directories(${TARGET_NAME} PRIVATE 
		${ASSIMP_INCLUDE_DIR}
		${THIRD_PARTY_DIR}
		${BOX2D_INCLUDE_DIR}
		${GLAD_INCLUDE_DIR}
		${GLFW_INCLUDE_DIR}
		${IMGUI_INCLUDE_DIR}
		${OPTICK_INCLUDE_DIR}
		${PFD_INCLUDE_DIR}
		${SPDLOG_INCLUDE_DIR}
		${YAML_CPP_INCLUDE_DIR}
		${MSDF_ATLAS_GEN_INCLUDE_DIR}
		${MSDF_GEN_INCLUDE_DIR}
	)
endmacro()


# Enable in cross-platform way multi-processor compilation
macro(EnableMPCompilation TARGET_NAME)
    if(MSVC)
	    target_compile_options(${TARGET_NAME} PUBLIC /MP)
    endif(MSVC)
    #TODO: support other compilers
endmacro()
