#include "ScriptEngine.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Project/Project.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scripting/Script.h"
#include "Athena/Utils/StringUtils.h"

// Name collision with WinApi CreateDirectory
#include <filewatch/FileWatch.h>
#include "Athena/Core/FileSystem.h"


namespace Athena
{
	struct ScriptEngineData
	{
		ScriptConfig Config;
		FilePath ScriptsBinaryPath;
		Scope<Library> ScriptsLibrary;

		std::vector<String> ScriptNames;
		std::unordered_map<String, ScriptClass> ScriptClasses;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;
		std::unordered_map<UUID, String> EntityScriptNames;
		std::unordered_map<UUID, ScriptInstance> EntityInstances;

		Scope<filewatch::FileWatch<String>> ScriptsAssemblyFileWatcher;
		bool ReloadPending = false;

		bool IsRuntime = false;
		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data = nullptr;

	static void OnScriptsAssemblyFileSystemEvent(const String& path, const filewatch::Event changeType)
	{
		if (!s_Data->ReloadPending && !s_Data->IsRuntime && changeType == filewatch::Event::modified)
		{
			s_Data->ReloadPending = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));	// HACK

			Application::Get().SubmitToMainThread([]()
			{
				s_Data->ScriptsAssemblyFileWatcher.Release();
				ScriptEngine::ReloadScripts();
			});
		}
	}	

	template<typename FuncT, typename... Args>
	static bool InvokeScriptFunc(const String& scriptName, Entity entity, FuncT func, Args&&... args)
	{
		if (!s_Data->Config.EnableDebug)
		{
			func(std::forward<Args>(args)...);
			return true;
		}

		try
		{
			func(std::forward<Args>(args)...);
		}
		catch (std::exception& exception)
		{
			if (entity == Entity{})
			{
				ATN_CORE_ERROR_TAG("SriptEngine", "Script '{}' threw exception: \n{}!", scriptName, exception.what());
			}
			else
			{
				const String& entityName = entity.GetComponent<TagComponent>().Tag;
				UUID id = entity.GetComponent<IDComponent>().ID;

				ATN_CORE_ERROR_TAG("SriptEngine", "Script '{}' (Entity name - {}, id - {}) threw exception: \n{}!", 
					scriptName, entityName, id, exception.what());
			}
			return false;
		}

		return true;
	}

	ScriptClass::ScriptClass(const String& className)
	{
		m_InstantiateMethod = (ScriptFunc_Instantiate)s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_Instantiate", className));
		m_OnCreateMethod = (ScriptFunc_OnCreate)s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_OnCreate", className));
		m_OnUpdateMethod = (ScriptFunc_OnUpdate)s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_OnUpdate", className));
		m_GetFieldsDescriptionMethod = (ScriptFunc_GetFieldsDescription)
			s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_GetFieldsDescription", className));

		m_IsLoaded = m_InstantiateMethod && m_OnCreateMethod && m_OnUpdateMethod && m_GetFieldsDescriptionMethod;
		m_Name = className;

		if (!m_IsLoaded)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to load script with name '{}'!", className);
			return;
		}

		// Get Fields
		Script* script = nullptr;
		if (InvokeScriptFunc(m_Name, Entity{}, m_InstantiateMethod, &script))
		{
			if (InvokeScriptFunc(m_Name, Entity{}, m_GetFieldsDescriptionMethod, script, &m_FieldsDescription))
			{
				for (auto& [name, storage] : m_FieldsDescription)
				{
					storage.GetInitialValue();
					storage.RemoveFieldReference();
				}

				delete script;
				return;
			}
		}

		ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to get fields description of script with name '{}'", className);
		delete script;
	}

	Script* ScriptClass::Instantiate(Entity entity) const
	{
		Script* script = nullptr;

		InvokeScriptFunc(m_Name, entity, m_InstantiateMethod, &script);
		script->Initialize(s_Data->SceneContext, entity);

		return script;
	}


	ScriptInstance::ScriptInstance(ScriptClass* scriptClass, Entity entity)
	{
		m_ScriptClass = scriptClass;
		m_Instance = m_ScriptClass->Instantiate(entity);

		if (s_Data->EntityScriptFields.contains(entity.GetID()))
			UpdateFieldMap(entity, true);
	}

	ScriptInstance::~ScriptInstance()
	{
		delete m_Instance;
	}

	void ScriptInstance::UpdateFieldMap(Entity entity, bool write)
	{
		m_FieldMap = s_Data->EntityScriptFields.at(entity.GetID());

		ScriptFieldMap fieldRefs;
		InvokeScriptFunc(m_ScriptClass->GetName(), m_Instance->GetEntity(), m_ScriptClass->GetGetFieldsDescriptionMethod(),
			m_Instance, &fieldRefs);

		ATN_CORE_ASSERT(m_FieldMap.size() == fieldRefs.size());
		for (auto& [name, field] : m_FieldMap)
		{
			field.SetFieldReference(fieldRefs.at(name).GetFieldReference(), write);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		InvokeScriptFunc(m_ScriptClass->GetName(), m_Instance->GetEntity(), m_ScriptClass->GetOnCreateMethod(),
			m_Instance);
	}

	void ScriptInstance::InvokeOnUpdate(Time frameTime)
	{
		InvokeScriptFunc(m_ScriptClass->GetName(), m_Instance->GetEntity(), m_ScriptClass->GetOnUpdateMethod(),
			m_Instance, (float)frameTime.AsMilliseconds());
	}

	Script* ScriptInstance::GetInternalInstance()
	{
		return m_Instance;
	}


	void ScriptEngine::Init(const ScriptConfig& config)
	{
		s_Data = new ScriptEngineData();
		s_Data->Config = config;
	}

	void ScriptEngine::Shutdown()
	{
		delete s_Data;
	}

	void ScriptEngine::InitProject()
	{
#ifdef ATN_DEBUG // This will search for scripting binary with MDd
		FilePath debugBinaryPath = Project::GetScriptsBinaryPath();
		debugBinaryPath = debugBinaryPath.parent_path() / FilePath(debugBinaryPath.stem().string() + "-d" + debugBinaryPath.extension().string());
		s_Data->ScriptsBinaryPath = debugBinaryPath;
#else
		s_Data->ScriptsBinaryPath = Project::GetScriptsBinaryPath();
#endif

		GenerateCMakeConfig();
		ReloadScripts();
	}

	void ScriptEngine::LoadAssembly()
	{
#ifdef ATN_DIST
		const FilePath& libPath = s_Data->Config.ScriptsBinaryPath;

		if (!FileSystem::Exists(libPath))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Scripts binary does not exists {}!", libPath);
			return;
		}

		s_Data->ScriptsLibrary = Scope<Library>::Create(libPath);
#else
		s_Data->ReloadPending = true;

		// Create copy of dll and pdb to read from it, original dll and pdb for writing
		const FilePath& libPath = s_Data->ScriptsBinaryPath;
		FilePath pdbPath = s_Data->ScriptsBinaryPath;
		pdbPath.replace_extension("pdb");

		FilePath activeFolder = libPath.parent_path() / "Active";

		if (!FileSystem::Exists(activeFolder))
			FileSystem::CreateDirectory(activeFolder);

		if (!FileSystem::Exists(libPath))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Scripts binary does not exists {}!", libPath);
			return;
		}
		
		FilePath activeLibPath = activeFolder / libPath.filename();

		if (!FileSystem::Copy(libPath, activeLibPath))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to load scripting library!");
			return;
		}

		FilePath activePdbPath = activeFolder / pdbPath.filename();

		if (FileSystem::Exists(pdbPath))
		{
			if (!FileSystem::Copy(pdbPath, activePdbPath))
			{
				ATN_CORE_WARN_TAG("ScriptEngine", "Failed to load debug info!");
			}
		}

		s_Data->ScriptsLibrary = Scope<Library>::Create(activeLibPath);
		s_Data->ScriptsAssemblyFileWatcher = Scope<filewatch::FileWatch<String>>::Create(libPath.string(), OnScriptsAssemblyFileSystemEvent);
		s_Data->ReloadPending = false;

#endif
	}

	void ScriptEngine::InitScriptClasses()
	{
		if (!s_Data->ScriptsLibrary || !s_Data->ScriptsLibrary->IsLoaded())
		{
			s_Data->EntityScriptFields.clear();
			s_Data->EntityScriptNames.clear();
			return;
		}

		FilePath sourceDir = Project::GetScriptsDirectory() / "Source";

		if (!FileSystem::Exists(sourceDir))
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "Source directory does not exists {}!", sourceDir);
			return;
		}

		FindScripts(sourceDir, s_Data->ScriptNames);

		for (const auto& scriptName : s_Data->ScriptNames)
		{
			ScriptClass scriptClass = ScriptClass(scriptName);

			if (scriptClass.IsLoaded())
				s_Data->ScriptClasses[scriptName] = scriptClass;
		}

		if (s_Data->ScriptNames.empty())
			ATN_CORE_WARN_TAG("ScriptEngine", "Failed to find any scripts in scripts binary!");

		// Restore old edited field values
		std::vector<UUID> entitiesToRemove;
		for (const auto& [entityID, fieldMap] : s_Data->EntityScriptFields)
		{
			const String& className = s_Data->EntityScriptNames.at(entityID);
			if (!ScriptEngine::IsScriptExists(className))
				entitiesToRemove.push_back(entityID);

			const ScriptClass& scClass = s_Data->ScriptClasses.at(className);
			const ScriptFieldMap& fieldMapDesc = scClass.GetFieldsDescription();
			
			ScriptFieldMap newFieldMap = fieldMapDesc;
			for (auto& [name, field] : fieldMapDesc)
			{
				if (fieldMap.contains(name) && (fieldMap.at(name).GetType() == newFieldMap.at(name).GetType()))
					newFieldMap.at(name) = fieldMap.at(name);
			}

			s_Data->EntityScriptFields.at(entityID) = newFieldMap;
		}

		for (const auto& entityID : entitiesToRemove)
		{
			s_Data->EntityScriptNames.erase(entityID);
			s_Data->EntityScriptFields.erase(entityID);
		}
	}

	void ScriptEngine::ReloadScripts()
	{
		s_Data->ScriptsLibrary.Release();
		s_Data->ScriptClasses.clear();
		s_Data->ScriptNames.clear();

		LoadAssembly();
		InitScriptClasses();
	}

	bool ScriptEngine::IsScriptExists(const String& name)
	{
		return s_Data->ScriptClasses.contains(name);
	}

	const std::vector<String>& ScriptEngine::GetAvailableScripts()
	{
		return s_Data->ScriptNames;
	}

	ScriptFieldMap* ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		UUID entityID = entity.GetID();

		if (s_Data->IsRuntime)
		{
			if (s_Data->EntityInstances.contains(entityID) && s_Data->EntityInstances.at(entityID).IsFieldMapInitialized())
				return &s_Data->EntityInstances.at(entityID).GetFieldMap();
		}
		else
		{
			if (s_Data->EntityScriptFields.contains(entityID))
				return &s_Data->EntityScriptFields.at(entityID);
		}

		const auto& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			ScriptClass scClass = s_Data->ScriptClasses.at(scriptName);
			if (!scClass.GetFieldsDescription().empty())
			{
				s_Data->EntityScriptFields[entityID] = scClass.GetFieldsDescription();
				s_Data->EntityScriptNames[entityID] = scriptName;

				// Update field refs if initialize fieldMap at runtime
				if (s_Data->EntityInstances.contains(entityID))
				{
					s_Data->EntityInstances.at(entityID).UpdateFieldMap(entity, false);
					return &s_Data->EntityInstances.at(entityID).GetFieldMap();
				}

				return &s_Data->EntityScriptFields.at(entityID);
			}
		}

		return nullptr;
	}

	void ScriptEngine::OnEntityScriptRemove(Entity entity)
	{
		UUID entityID = entity.GetID();

		if(s_Data->EntityScriptFields.contains(entityID))
			s_Data->EntityScriptFields.erase(entityID);

		if (s_Data->EntityScriptNames.contains(entityID))
			s_Data->EntityScriptNames.erase(entityID);

		if (s_Data->IsRuntime && s_Data->EntityInstances.contains(entityID))
			s_Data->EntityInstances.erase(entityID);
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->IsRuntime = true;
		s_Data->SceneContext = scene;
	}

	void ScriptEngine::OnRuntimeStop()
	{
		s_Data->IsRuntime = false;
		s_Data->SceneContext = nullptr;
		s_Data->EntityInstances.clear();
	}

	void ScriptEngine::InstantiateEntity(Entity entity)
	{
		ATN_PROFILE_FUNC();

		const String& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			ScriptClass& scriptClass = s_Data->ScriptClasses.at(scriptName);

			// Need to construct ScriptInstance in place
			s_Data->EntityInstances.emplace(std::piecewise_construct, 
				std::forward_as_tuple(entity.GetID()), 
				std::forward_as_tuple(&scriptClass, entity));
		}
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		ATN_PROFILE_FUNC();

		const String& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			s_Data->EntityInstances.at(entity.GetID()).InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Time frameTime)
	{
		ATN_PROFILE_FUNC();

		const String& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			// Initialize entity script at runtime
			if (!s_Data->EntityInstances.contains(entity.GetID()))
			{
				InstantiateEntity(entity);
				OnCreateEntity(entity);
			}

			s_Data->EntityInstances.at(entity.GetID()).InvokeOnUpdate(frameTime);
		}
	}

	void ScriptEngine::GenerateCMakeConfig()
	{
		FilePath scResources = Application::Get().GetConfig().EngineResourcesPath / "Scripting";
		FilePath configTemplatePath = scResources / "Config-Template.cmake";

		if (!FileSystem::Exists(configTemplatePath))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to find config template!");
			return;
		}

		// TODO: Projects
		String configSource = FileSystem::ReadFile(configTemplatePath);
		Utils::ReplaceAll(configSource, "<REPLACE_PROJECT_NAME>", "Sandbox");
		Utils::ReplaceAll(configSource, "<REPLACE_ATHENA_SOURCE_DIR>", "${CMAKE_SOURCE_DIR}/../../..");
		Utils::ReplaceAll(configSource, "<REPLACE_ATHENA_BINARY_DIR>", "${CMAKE_SOURCE_DIR}/../../../Build/Binaries/Athena");

#ifdef ATN_DEBUG
		Utils::ReplaceAll(configSource, "<REPLACE_USE_DEBUG_RUNTIME_LIBS>", "ON");
#else
		Utils::ReplaceAll(configSource, "<REPLACE_USE_DEBUG_RUNTIME_LIBS>", "OFF");
#endif

		FilePath configPath = Project::GetScriptsDirectory() / "Config.cmake";
		bool generate = !FileSystem::Exists(configPath);

		if (!generate)
		{
			String configOldSource = FileSystem::ReadFile(configPath);
			generate = configOldSource != configSource;
		}

		if (generate)
		{
			if (!FileSystem::WriteFile(configPath, configSource.c_str(), configSource.size()))
				ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to generate cmake config {}", configPath);
		}
	}

	void ScriptEngine::CreateNewScript(const String& name)
	{
		bool invalidName = name.empty() ||
			std::find_if(name.begin(), name.end(), [](char c) { return !std::isalpha(c); }) != name.end();

		if (invalidName)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Invalid name for script class - '{}'", name);
			return;
		}

		FilePath scResources = Application::Get().GetConfig().EngineResourcesPath / "Scripting";

		FilePath cppTemplate = scResources / "Script-Template.cpp";
		FilePath headerTemplate = scResources / "Script-Template.h";

		if (!FileSystem::Exists(cppTemplate) || !FileSystem::Exists(headerTemplate))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to find script templates!");
			return;
		}

		FilePath srcPath = Project::GetScriptsDirectory() / "Source";

		{
			String headerSource = FileSystem::ReadFile(headerTemplate);
			Utils::ReplaceAll(headerSource, "ClassName", name);
			Utils::ReplaceAll(headerSource, "NamespaceName", "Sandbox");	// TODO: Projects
			FilePath headerPath = srcPath / fmt::format("{}.h", name);

			if (!FileSystem::WriteFile(headerPath, headerSource.c_str(), headerSource.size()))
				ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to create new script file {}", headerPath);
		}

		{
			String cppSource = FileSystem::ReadFile(cppTemplate);
			Utils::ReplaceAll(cppSource, "ClassName", name);
			Utils::ReplaceAll(cppSource, "NamespaceName", "Sandbox");	// TODO: Projects
			FilePath cppPath = srcPath / fmt::format("{}.cpp", name);

			if(!FileSystem::WriteFile(cppPath, cppSource.c_str(), cppSource.size()))
				ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to create new script file {}", cppPath);
		}

		GenCMakeProjects();
	}

	void ScriptEngine::OpenInVisualStudio()
	{
		const String projectName = "Sandbox"; // Temporary

		FilePath solutionName = fmt::format("{}.sln", projectName);
		FilePath solutionPath = Project::GetScriptsDirectory() / "Build/Projects" / solutionName;

		if (!FileSystem::Exists(solutionPath))
			GenCMakeProjects();

		if (FileSystem::Exists(solutionPath))
			Platform::RunFile(solutionName, Project::GetScriptsDirectory() / "Build/Projects");
		else
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to open solution file!");
	}

	void ScriptEngine::GenCMakeProjects()
	{
		Platform::RunFile("VS2022-GenProjects.bat", Project::GetScriptsDirectory());
	}

	void ScriptEngine::FindScripts(const FilePath& dir, std::vector<String>& scriptsNames)
	{
		// TODO: in the future at runtime we will be using AssetManager to find necessary scripts
		// For now iterate through .cpp files and check if corresponding script exists 
		// in library and then load this script

		for (const auto& dirEntry : std::filesystem::directory_iterator(dir))
		{
			const FilePath& path = dirEntry.path();

			if (dirEntry.is_directory())
				FindScripts(path, scriptsNames);

			if (path.extension() == ".cpp")
			{
				String name = path.stem().string();

				String instantiateFuncName = fmt::format("_{}_Instantiate", name);
				void* func = s_Data->ScriptsLibrary->LoadFunction(instantiateFuncName);

				if (func)
					scriptsNames.push_back(name);
			}
		}
	}
}
