#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Core/Application.h"

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <commdlg.h>
#include <intrin.h>
#include <psapi.h>


namespace Athena
{
	namespace WindowsUtils
	{
		static bool CheckLastError()
		{
			DWORD errorMessageID = GetLastError();
			if (errorMessageID == ERROR_SUCCESS)
				return true;

			LPSTR messageBuffer = nullptr;

			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			String message(messageBuffer, size);
			ATN_CORE_ERROR_TAG("Platform", "WinAPI Error: {}", message);

			LocalFree(messageBuffer);
			return false;
		}

		static DWORD CountSetBits(ULONG_PTR bitMask)
		{
			DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
			DWORD bitSetCount = 0;
			ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
			DWORD i;

			for (i = 0; i <= LSHIFT; ++i)
			{
				bitSetCount += ((bitMask & bitTest) ? 1 : 0);
				bitTest /= 2;
			}

			return bitSetCount;
		}
	}

#ifdef ATN_ENABLE_ASSERTS
	#define WINAPI_CHECK_LASTERROR() ATN_CORE_ASSERT(WindowsUtils::CheckLastError())
	#define WINAPI_SUPPRESS_LAST_ERROR() SetLastError(ERROR_SUCCESS)
#else
	#define WINAPI_CHECK_LASTERROR()
	#define WINAPI_SUPPRESS_LAST_ERROR()
#endif


	struct WindowsPlatformData
	{
		CPUCapabilities CPUCaps;
		HWND WindowHandle = nullptr;
		DWORD ProcessID = 0;
		double PerformanceFrequency = 0;
	};

	static WindowsPlatformData s_Data;


	void Platform::Init()
	{
		s_Data.WindowHandle = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());

		// CPU name
		{
			int cpuInfo[4] = { -1 };
			char cpuString[0x41] = { 0 };

			memset(cpuString, 0, sizeof(cpuString));

			__cpuid(cpuInfo, 0x80000002);
			memcpy(cpuString, cpuInfo, sizeof(cpuInfo));

			__cpuid(cpuInfo, 0x80000003);
			memcpy(cpuString + 16, cpuInfo, sizeof(cpuInfo));

			__cpuid(cpuInfo, 0x80000004);
			memcpy(cpuString + 32, cpuInfo, sizeof(cpuInfo));

			s_Data.CPUCaps.Name = cpuString;
		}

		WINAPI_SUPPRESS_LAST_ERROR();

		// Memory info
		{
			MEMORYSTATUSEX statex;
			statex.dwLength = sizeof(statex);
			GlobalMemoryStatusEx(&statex);
			WINAPI_CHECK_LASTERROR();

			s_Data.CPUCaps.RAM = statex.ullTotalPhys / 1024;
		}

		// Logical processor info
		{
			typedef BOOL(WINAPI* LPFN_GetLogicalProcessorInformation)(
				PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
				PDWORD);

			auto GetLogicalProcessorInformation = (LPFN_GetLogicalProcessorInformation)GetProcAddress(
				GetModuleHandle(TEXT("kernel32")),
				"GetLogicalProcessorInformation");

			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
			DWORD bufferSize = 0;
			DWORD logicalProcessorCount = 0;
			DWORD processorCoreCount = 0;
			DWORD byteOffset = 0;

			while (true)
			{
				DWORD rc = GetLogicalProcessorInformation(buffer, &bufferSize);
				if (rc != FALSE)
					break;

				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					free(buffer);

					buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(bufferSize);
					WINAPI_SUPPRESS_LAST_ERROR();
				}
				else
				{
					WINAPI_CHECK_LASTERROR();
				}
			}

			ptr = buffer;

			while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= bufferSize)
			{
				if (ptr->Relationship == RelationProcessorCore)
				{
					processorCoreCount++;

					// A hyper threaded core supplies more than one logical processor.
					logicalProcessorCount += WindowsUtils::CountSetBits(ptr->ProcessorMask);
				}
				byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
				ptr++;
			}

			free(buffer);

			s_Data.CPUCaps.Cores = processorCoreCount;
			s_Data.CPUCaps.LogicalProcessors = logicalProcessorCount;
		}

		// Process ID
		{
			DWORD processID;
			GetWindowThreadProcessId(s_Data.WindowHandle, &processID);
			WINAPI_CHECK_LASTERROR();
			s_Data.ProcessID = processID;
		}

		ATN_CORE_INFO_TAG("Platform", "Initalize Windows platform");
	}

	const CPUCapabilities& Platform::GetCPUCapabilities()
	{
		return s_Data.CPUCaps;
	}

	void Platform::OpenInBrowser(const std::wstring& url)
	{
		WINAPI_SUPPRESS_LAST_ERROR();
		ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWDEFAULT);
		WINAPI_CHECK_LASTERROR();
	}

	void Platform::RunFile(const FilePath& path, const FilePath& workingDir)
	{
		WINAPI_SUPPRESS_LAST_ERROR();
		ShellExecute(NULL, L"open", path.c_str(), NULL, workingDir.c_str(), SW_SHOWDEFAULT);
		WINAPI_CHECK_LASTERROR();
	}

	double Platform::GetHighPrecisionTime()
	{
		// We need timer before platform initialization
		if (s_Data.PerformanceFrequency == 0)
		{
			LARGE_INTEGER li;
			QueryPerformanceFrequency(&li);
			WINAPI_CHECK_LASTERROR();
			s_Data.PerformanceFrequency = (double)li.QuadPart / 1000.0;
		}

		LARGE_INTEGER result;
		QueryPerformanceCounter(&result);

		return double(result.QuadPart) / s_Data.PerformanceFrequency;
	}

	uint64 Platform::GetMemoryUsage()
	{
		WINAPI_SUPPRESS_LAST_ERROR();	// Error from somewhere else

		HANDLE hProcess;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, s_Data.ProcessID);

		WINAPI_CHECK_LASTERROR();

		if (NULL == hProcess)
			return 0;

		uint64 memUsage = 0;
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			memUsage = pmc.WorkingSetSize;
		}

		WINAPI_CHECK_LASTERROR();

		CloseHandle(hProcess);
		WINAPI_CHECK_LASTERROR();
		return memUsage;
	}


	FilePath FileDialogs::OpenFile(std::wstring_view filter)
	{
		OPENFILENAME ofn;
		WCHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = s_Data.WindowHandle;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileName(&ofn) == TRUE)
			return ofn.lpstrFile;

		WINAPI_CHECK_LASTERROR();
		return {};
	}

	FilePath FileDialogs::SaveFile(std::wstring_view filter)
	{
		OPENFILENAME ofn;
		WCHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = s_Data.WindowHandle;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (GetSaveFileName(&ofn) == TRUE)
			return ofn.lpstrFile;

		WINAPI_CHECK_LASTERROR();
		return {};
	}

	void FileDialogs::OpenInFileExplorer(const FilePath& path)
	{
		ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}


	Library::Library(const FilePath& path)
		: m_Path(path)
	{
		WINAPI_SUPPRESS_LAST_ERROR();
		m_Handle = LoadLibrary(m_Path.c_str());
		WINAPI_CHECK_LASTERROR();

		if (IsLoaded())
			ATN_CORE_INFO_TAG("Platform", "Successfully loaded library from {}", m_Path);
	}

	Library::~Library()
	{
		FreeLibrary((HMODULE)m_Handle);
	}

	bool Library::IsLoaded()
	{
		return m_Handle != nullptr;
	}

	void* Library::LoadFunction(const String& name)
	{
		return GetProcAddress((HMODULE)m_Handle, name.c_str());
	}
}
