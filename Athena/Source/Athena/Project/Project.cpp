#include "Project.h"

#include "Athena/Project/ProjectSerializer.h"


namespace Athena
{
	Ref<Project> Project::New(const String& name, const FilePath& path)
	{
		s_ActiveProject = Ref<Project>::Create();
		s_ActiveProject->m_Config.Name = name;
		s_ActiveProject->m_ProjectDirectory = path;

		return s_ActiveProject;
	}

	Ref<Project> Project::Load(const FilePath& path)
	{
		Ref<Project> project = Ref<Project>::Create();

		ProjectSerializer serializer(project);
		if (serializer.Deserialize(path))
		{
			project->m_ProjectDirectory = path.parent_path();
			s_ActiveProject = project;
			return s_ActiveProject;
		}

		return nullptr;
	}

	bool Project::SaveActive(const FilePath& path)
	{
		ProjectSerializer serializer(s_ActiveProject);
		if (serializer.Serialize(path))
		{
			s_ActiveProject->m_ProjectDirectory = path.parent_path();
			return true;
		}

		return false;
	}

	bool Project::SaveActive()
	{
		ATN_CORE_ASSERT(!s_ActiveProject->m_ProjectDirectory.empty());

		ProjectSerializer serializer(s_ActiveProject);
		if (serializer.Serialize(s_ActiveProject->GetProjectPath()))
			return true;

		return false;
	}

	FilePath Project::GetProjectPath() const 
	{
		FilePath path = m_ProjectDirectory / m_Config.Name;
		path.replace_extension(".atproj");
		return path;
	}
}