#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Project/Project.h"


namespace Athena
{
	class ProjectSerializer
	{
	public:
		ProjectSerializer(const Ref<Project>& project);

		bool Serialize(const FilePath& path);
		bool Deserialize(const FilePath& path);

	private:
		Ref<Project> m_Project;
	};
}
