#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scripting/ScriptFields.h"


namespace Athena
{
	class Script
	{
	public:
		Script() = default;
		virtual ~Script() = default;

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			return m_Entity.AddOrReplaceComponent<T>(std::forward<Args>(args)...);
		}

		template <typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		template <typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();;
		}

		template <typename T>
		const T& GetComponent() const
		{
			return m_Entity.GetComponent<T>();;
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_Entity.HasComponent<T>();;
		}

	public:
		Scene* GetScene() const { return m_Scene; }
		Entity GetEntity() const { return m_Entity; }

	public:
		virtual void OnCreate() {};
		virtual void OnUpdate(Time frameTime) {};
		virtual void GetFieldsDescription(ScriptFieldMap* outFieldsDesc) {};

	protected:
		Entity Instantiate() const { return m_Scene->CreateEntity(); }
		Entity FindEntityByName(const String& name) const { return m_Scene->FindEntityByName(name); }
		Entity FindEntityByUUID(UUID id) const { return m_Scene->GetEntityByUUID(id); }

	private:
		friend class ATHENA_API ScriptEngine;
		friend class ATHENA_API ScriptClass;

		void Initialize(Scene* scene, Entity entity)
		{
			m_Scene = scene;
			m_Entity = entity;
		}

	private:
		Scene* m_Scene;
		Entity m_Entity;
	};


	using ScriptFunc_Instantiate = void (*)(Script** outScript);
	using ScriptFunc_OnCreate = void (*)(Script* _this);
	using ScriptFunc_OnUpdate = void (*)(Script* _this, float frameTime);
	using ScriptFunc_GetFieldsDescription = void (*)(Script* _this, ScriptFieldMap* outFields);


#ifdef ATN_PLATFORM_WINDOWS
#define SCRIPT_API __declspec(dllexport)
#else
#define SCRIPT_API
#endif // End of linking detection


#define EXPORT_SCRIPT(ClassName) 	extern "C"																\
	{																										\
		SCRIPT_API void _##ClassName##_Instantiate(::Athena::Script** outScript)							\
		{																								    \
			*outScript = new ClassName();																	\
		}																									\
		SCRIPT_API void _##ClassName##_OnCreate(::Athena::Script* _this)									\
		{																								    \
			_this->OnCreate();																				\
		}																									\
		SCRIPT_API void _##ClassName##_OnUpdate(::Athena::Script* _this, float frameTime)					\
		{																									\
			_this->OnUpdate(::Athena::Time::Milliseconds(frameTime));										\
		}																									\
		SCRIPT_API void _##ClassName##_GetFieldsDescription(::Athena::Script * _this,						\
			::Athena::ScriptFieldMap * outFields)															\
		{																									\
			_this->GetFieldsDescription(outFields);															\
		}																									\
	}	


#define ADD_FIELD(FieldName) outFields->insert({ ATN_STRINGIFY_MACRO(FieldName), ::Athena::ScriptFieldStorage(&(FieldName))});
}
