#pragma once


#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API FileDialogs
	{
	public:
		static FilePath OpenFile(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir = "");
		static std::vector<FilePath> OpenFiles(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir = "");
		static FilePath OpenDirectory(const String& dialogName, const FilePath& startDir = "");

		static FilePath SaveFile(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir = "");
	};
}
