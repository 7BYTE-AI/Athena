#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"
#include "Athena/Math/Vector.h"
#include "Athena/Math/Quaternion.h"


namespace Athena
{
	struct EditorState
	{
		UUID SelectedEntity = { 0 };
		FilePath ActiveScene;

		Vector3 CameraPos = Vector3(0);
		Vector2 CameraPitchYaw = Vector2(0, 0);
		float CameraSpeed = 0.3f;
	};

	struct ProjectConfig
	{
		// General
		String Name = "UnNamed";
		FilePath StartScene;

		// Scripting
		FilePath AthenaSourceDirectory;
		FilePath AthenaBinaryDirectory;

		// Physics
		uint32 VelocityIterations = 6;
		uint32 PositionIterations = 2;
		Vector2 Gravity = { 0, -9.8 };

		EditorState EditorSavedState;
	};

	class ATHENA_API Project
	{
	public:
		static const FilePath& GetProjectDirectory()
		{
			ATN_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectDirectory;
		}

		static FilePath GetAssetDirectory()
		{
			return GetProjectDirectory() / "Assets";
		}

		static FilePath GetAssetFileSystemPath(const FilePath& path)
		{
			return GetAssetDirectory() / path;
		}

		static FilePath GetRelativeAssetPath(const FilePath& path)
		{
			return std::filesystem::relative(path, GetAssetDirectory());
		}

		static FilePath GetScriptsDirectory()
		{
			return GetProjectDirectory() / "Scripts";
		}

		static FilePath GetLogsDirectory()
		{
			return GetProjectDirectory() / "Logs";
		}

		static FilePath GetScriptsBinaryPath()
		{
			return GetScriptsDirectory() / "Build/Binaries/ScriptsLibrary/ScriptsLibrary.dll";
		}

		FilePath GetProjectPath() const;
		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }

		static Ref<Project> New(const String& name, const FilePath& path);
		static Ref<Project> Load(const FilePath& path);
		static bool SaveActive(const FilePath& path);
		static bool SaveActive();

	private:
		ProjectConfig m_Config;
		FilePath m_ProjectDirectory;

		inline static Ref<Project> s_ActiveProject;
	};
}
