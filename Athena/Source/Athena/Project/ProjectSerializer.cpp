#include "ProjectSerializer.h"
#include "Athena/Core/YAMLTypes.h"

#include <yaml-cpp/yaml.h>


namespace Athena
{
	ProjectSerializer::ProjectSerializer(const Ref<Project>& project)
		: m_Project(project)
	{

	}

	bool ProjectSerializer::Serialize(const FilePath& path)
	{
		const auto& config = m_Project->GetConfig();

		YAML::Emitter out;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Project" << YAML::Value << YAML::BeginMap;
			{
				out << YAML::Key << "General";

				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << config.Name;
				out << YAML::Key << "StartScene" << YAML::Value << config.StartScene;
				out << YAML::EndMap;

				out << YAML::Key << "Scripting";

				out << YAML::BeginMap;
				out << YAML::Key << "AthenaSourceDirectory" << YAML::Value << config.AthenaSourceDirectory;
				out << YAML::Key << "AthenaBinaryDirectory" << YAML::Value << config.AthenaBinaryDirectory;
				out << YAML::EndMap;

				out<< YAML::Key << "Physics";

				out << YAML::BeginMap;
				out << YAML::Key << "VelocityIterations" << YAML::Value << config.VelocityIterations;
				out << YAML::Key << "PositionIterations" << YAML::Value << config.PositionIterations;
				out << YAML::Key << "Gravity" << YAML::Value << config.Gravity;
				out << YAML::EndMap;

				out<< YAML::Key << "EditorState";

				out << YAML::BeginMap;
				out << YAML::Key << "SelectedEntity" << YAML::Value << config.EditorSavedState.SelectedEntity;
				out << YAML::Key << "ActiveScene" << YAML::Value << config.EditorSavedState.ActiveScene;
				out << YAML::Key << "CameraPos" << YAML::Value << config.EditorSavedState.CameraPos;
				out << YAML::Key << "CameraPitchYaw" << YAML::Value << config.EditorSavedState.CameraPitchYaw;
				out << YAML::Key << "CameraSpeed" << YAML::Value << config.EditorSavedState.CameraSpeed;
				out << YAML::EndMap;

			};
			out << YAML::EndMap << YAML::EndMap;
		}

		std::ofstream fout(path);
		fout << out.c_str();

		return true;
	}

	bool ProjectSerializer::Deserialize(const FilePath & path)
	{
		auto& config = m_Project->GetConfig();

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(path.string());
		}
		catch (YAML::ParserException e)
		{
			ATN_CORE_ERROR("Failed to load project file '{0}'\n     {1}", path, e.what());
			return false;
		}

		auto projectNode = data["Project"];
		if (!projectNode)
			return false;

		auto generalNode = projectNode["General"];
		config.Name = generalNode["Name"].as<std::string>();
		config.StartScene = generalNode["StartScene"].as<FilePath>();

		auto scriptingNode = projectNode["Scripting"];
		config.AthenaSourceDirectory = scriptingNode["AthenaSourceDirectory"].as<FilePath>();
		config.AthenaBinaryDirectory = scriptingNode["AthenaBinaryDirectory"].as<FilePath>();

		auto physicsNode = projectNode["Physics"];
		config.VelocityIterations = physicsNode["VelocityIterations"].as<uint32>();
		config.PositionIterations = physicsNode["PositionIterations"].as<uint32>();
		config.Gravity = physicsNode["Gravity"].as<Vector2>();

		auto editorStateNode = projectNode["EditorState"];
		config.EditorSavedState.SelectedEntity = editorStateNode["SelectedEntity"].as<UUID>();
		config.EditorSavedState.ActiveScene = editorStateNode["ActiveScene"].as<FilePath>();
		config.EditorSavedState.CameraPos = editorStateNode["CameraPos"].as<Vector3>();
		config.EditorSavedState.CameraPitchYaw = editorStateNode["CameraPitchYaw"].as<Vector2>();
		config.EditorSavedState.CameraSpeed = editorStateNode["CameraSpeed"].as<float>();

		return true;
	}
}
