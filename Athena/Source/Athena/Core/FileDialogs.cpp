#include "FileDialogs.h"

#include <portable-file-dialogs/portable-file-dialogs.h>


namespace Athena
{
	FilePath FileDialogs::OpenFile(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir)
	{
		std::vector<String> selection = pfd::open_file(dialogName, defaultDir.string(), filters, false).result();

		if (!selection.empty())
			return selection[0];

		return "";
	}

	std::vector<FilePath> FileDialogs::OpenFiles(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir)
	{
		std::vector<String> selection = pfd::open_file(dialogName, defaultDir.string(), filters, true).result();

		if (!selection.empty())
		{
			std::vector<FilePath> result;
			result.reserve(selection.size());

			for (const auto& item : selection)
				result.push_back(item);

			return result;
		}

		return {};
	}

	FilePath FileDialogs::OpenDirectory(const String& dialogName, const FilePath& startDir)
	{
		String selection = pfd::select_folder(dialogName, startDir.string(), pfd::opt::none).result();
		return selection;
	}

	FilePath FileDialogs::SaveFile(const String& dialogName, const std::vector<String>& filters, const FilePath& defaultDir)
	{
		String selection = pfd::save_file(dialogName, defaultDir.string(), filters, true).result();
		return selection;
	}
}
