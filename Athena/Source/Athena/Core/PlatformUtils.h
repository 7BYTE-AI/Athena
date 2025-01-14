#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	struct CPUCapabilities
	{
		String Name;
		uint32 Cores = 0;
		uint32 LogicalProcessors = 0;
		uint64 RAM = 0;	// Kb
	};

	class ATHENA_API Platform
	{
	public:
		static void Init();
		static const CPUCapabilities& GetCPUCapabilities();

		static void OpenInBrowser(const std::wstring& url);
		// In milliseconds
		static double GetHighPrecisionTime();
		// In bytes
		static uint64 GetMemoryUsage();
	};

	class ATHENA_API FileDialogs
	{
	public:
		static FilePath OpenFile(std::wstring_view filter);
		static FilePath SaveFile(std::wstring_view filter);

		static void OpenInFileExplorer(const FilePath& path);
	};
}
