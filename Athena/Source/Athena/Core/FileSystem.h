#pragma once

#include "Athena/Core/Core.h"

#ifdef CreateDirectory
	#undef CreateDirectory;
#endif


namespace Athena
{
	class ATHENA_API FileSystem
	{
	public:
		static String ReadFile(const FilePath& path);
		static std::vector<byte> ReadFileBinary(const FilePath& path);

		static bool WriteFile(const FilePath& path, const char* bytes, uint64 size);
		static bool Remove(const FilePath& path);

		static FilePath GetWorkingDirectory();
		static void SetWorkingDirectory(const FilePath& path);

		static void CreateDirectory(const FilePath& path);
		static bool Exists(const FilePath& path);
	};
}
