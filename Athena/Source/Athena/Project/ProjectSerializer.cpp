#include "ProjectSerializer.h"

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
			out << YAML::Key << "Project" << YAML::Value;
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << config.Name;
				out << YAML::Key << "StartScene" << YAML::Value << config.StartScene.string();
				out << YAML::Key << "AthenaSourceDirectory" << YAML::Value << config.AthenaSourceDirectory.string();
				out << YAML::Key << "AthenaBinaryPath" << YAML::Value << config.AthenaBinaryPath.string();
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
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

		config.Name = projectNode["Name"].as<std::string>();
		config.StartScene = projectNode["StartScene"].as<std::string>();
		config.AthenaSourceDirectory = projectNode["AthenaSourceDirectory"].as<std::string>();
		config.AthenaBinaryPath = projectNode["AthenaBinaryPath"].as<std::string>();

		return true;
	}
}
