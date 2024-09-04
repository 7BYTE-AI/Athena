#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scripting/Script.h"


namespace Athena
{
	class Script;

	class ScriptClass
	{
	public:
		ScriptClass() = default;
		ScriptClass(const String& className);

		Script* Instantiate(Entity entity) const;

		ScriptFunc_OnCreate GetOnCreateMethod() { return m_OnCreateMethod; }
		ScriptFunc_OnUpdate GetOnUpdateMethod() { return m_OnUpdateMethod; }
		ScriptFunc_GetFieldsDescription GetGetFieldsDescriptionMethod() { return m_GetFieldsDescriptionMethod; }

		bool IsLoaded() const { return m_IsLoaded; }
		const String& GetName() const { return m_Name; }

		const ScriptFieldMap& GetFieldsDescription() const { return m_FieldsDescription; }

	private:
		ScriptFunc_Instantiate m_InstantiateMethod;
		ScriptFunc_OnCreate m_OnCreateMethod;
		ScriptFunc_OnUpdate m_OnUpdateMethod;
		ScriptFunc_GetFieldsDescription m_GetFieldsDescriptionMethod;

		ScriptFieldMap m_FieldsDescription;
		bool m_IsLoaded = false;
		String m_Name;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance() = default;
		ScriptInstance(ScriptClass* scriptClass, Entity entity);
		~ScriptInstance();

		ScriptInstance(const ScriptInstance&) = delete;
		ScriptInstance& operator=(const ScriptInstance&) = delete;

		ScriptInstance(ScriptInstance&& other) = default;
		ScriptInstance& operator=(ScriptInstance&& other) = default;

		ScriptFieldMap& GetFieldMap() { return m_FieldMap; }
		bool IsFieldMapInitialized() const { return !m_FieldMap.empty(); }
		void UpdateFieldMap(Entity entity, bool write = true);

		void InvokeOnCreate();
		void InvokeOnUpdate(Time frameTime);

		Script* GetInternalInstance();

	private:
		ScriptClass* m_ScriptClass;
		Script* m_Instance = nullptr;
		ScriptFieldMap m_FieldMap;
	};


	struct ScriptConfig
	{
		bool EnableDebug;
	};

	class ATHENA_API ScriptEngine
	{
	public:
		static void Init(const ScriptConfig& config);
		static void Shutdown();

		static void InitProject();
		static void ReloadScripts();
		static bool IsScriptExists(const String& name);
		static const std::vector<String>& GetAvailableScripts();

		static ScriptFieldMap* GetScriptFieldMap(Entity entity);

		static void OnEntityScriptRemove(Entity entity);
		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static void InstantiateEntity(Entity entity);
		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, Time frameTime);

		static void GenerateCMakeConfig();
		static void CreateNewScript(const String& name);
		static void OpenInVisualStudio();

	private:
		static void LoadAssembly();
		static void InitScriptClasses();

		static void GenCMakeProjects();
		static void FindScripts(const FilePath& dir, std::vector<String>& scriptsNames);
	};
}
