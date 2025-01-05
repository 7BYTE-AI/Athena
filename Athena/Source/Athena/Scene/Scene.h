#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/UUID.h"

#include "Athena/Renderer/EditorCamera.h"
#include "Athena/Renderer/SceneRenderer.h"

#include "Athena/Scene/SceneCamera.h"

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <entt/entt.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#include <unordered_map>


class b2World;

namespace Athena
{
	class ATHENA_API Entity;

	class TransformComponent;
	class WorldTransformComponent;


	class ATHENA_API Scene
	{
	public:
		friend class ATHENA_API Entity;
		friend class ATHENA_API SceneSerializer;

	public:
		Scene(const String& name = "UnNamed");
		~Scene();

		static Ref<Scene> Copy(Ref<Scene> scene);

		Entity CreateEntity(const String& name, UUID id);
		Entity CreateEntity(const String& name, UUID id, Entity parent);
		Entity CreateEntity(const String& name = "UnNamed");
		void DestroyEntity(Entity entity);
		Entity DuplicateEntity(Entity entity);
		void MakeRelationship(Entity parent, Entity child);
		void MakeOrphan(Entity child);

		Entity GetEntityByUUID(UUID uuid);
		Entity FindEntityByName(const String& name);

		void OnUpdateEditor(Time frameTime); 
		void OnUpdateRuntime(Time frameTime);
		void OnUpdateSimulation(Time frameTime);

		void OnRuntimeStart();
		void OnSimulationStart();

		void OnRuntimeStop();

		void OnRender(const Ref<SceneRenderer>& renderer, const EditorCamera& camera);
		void OnRender(const Ref<SceneRenderer>& renderer);

		void OnRender2D(const Ref<SceneRenderer2D>& renderer2D);

		void OnViewportResize(uint32 width, uint32 height);
		Vector2u GetViewportSize() const { return { m_ViewportWidth, m_ViewportHeight }; }

		void SetSceneName(const String& name) { m_Name = name; }
		const String& GetSceneName() const { return m_Name; };

		Entity GetPrimaryCameraEntity();
		uint64 GetEntitiesCount() { return m_EntityMap.size(); }

		template <typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		void UpdateWorldTransforms();
		void UpdateWorldTransform(Entity entity, const WorldTransformComponent& parentTransform);

		void OnPhysics2DStart();
		void UpdatePhysics(Time frameTime);

		void RenderScene(const Ref<SceneRenderer>& renderer, const CameraInfo& cameraInfo);

		template <typename T>
		void OnComponentAdd(Entity entity, T& component);

		template <typename T>
		void OnComponentRemove(Entity entity, T& component);

	private:
		String m_Name;

		entt::registry m_Registry;
		std::unordered_map<UUID, entt::entity> m_EntityMap;

		std::unique_ptr<b2World> m_PhysicsWorld;

		uint32 m_ViewportWidth = 0, m_ViewportHeight = 0;
	};
}
