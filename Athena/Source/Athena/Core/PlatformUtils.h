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
		static void OpenInFileExplorer(const FilePath& path);

		static void RunFile(const FilePath& path, const FilePath& workingDir);

		// In milliseconds
		static double GetHighPrecisionTime();
		// In bytes
		static uint64 GetMemoryUsage();
	};

	class ATHENA_API Library
	{
	public:
		Library() = default;
		Library(const FilePath& path);
		~Library();

		Library(const Library& other) = delete;
		Library& operator=(const Library& other) = delete;

		bool IsLoaded();
		void* LoadFunction(const String& name);

	private:
		void* m_Handle = nullptr;
		FilePath m_Path;
	};
}
