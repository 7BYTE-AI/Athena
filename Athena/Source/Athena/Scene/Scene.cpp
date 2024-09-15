#include "Scene.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Light.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Components.h"
#include "Athena/Project/Project.h"
#include "Athena/Scripting/ScriptEngine.h"

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif


namespace Athena
{
	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
			{
				auto view = src.view<Component>();
				for (auto srcEntity : view)
				{
					UUID id = src.get<IDComponent>(srcEntity).ID;
					entt::entity dstEntity = enttMap.at(id);

					auto& srcComponent = src.get<Component>(srcEntity);
					dst.emplace_or_replace<Component>(dstEntity, srcComponent);
				}
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
			{
				if (src.HasComponent<Component>())
					dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}


	static b2BodyType AthenaRigidBody2DTypeToBox2D(Rigidbody2DComponent::BodyType type)
	{
		switch (type)
		{
		case Rigidbody2DComponent::BodyType::STATIC: return b2BodyType::b2_staticBody;
		case Rigidbody2DComponent::BodyType::DYNAMIC: return b2BodyType::b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::KINEMATIC: return b2BodyType::b2_kinematicBody;
		}
		ATN_CORE_ASSERT(false, "Undefined Rigidbody2DComponent BodyType!");
		return (b2BodyType)0;
	}

	static void DeleteFromChildren(std::vector<Entity>& children, Entity parent, Entity child)
	{
		auto iter = std::find(children.begin(), children.end(), child);
		if (iter != children.end())
		{
			children.erase(iter);
			if (children.size() == 0)
				parent.RemoveComponent<ChildComponent>();

		}
		else
		{
			const String& childTag = child.GetComponent<TagComponent>().Tag;
			const String& parentTag = parent.GetComponent<TagComponent>().Tag;
			ATN_CORE_WARN("Attempt to delete entity '{}' from children of entity '{}'", childTag, parentTag);
		}
	}

	Scene::Scene(const String& name)
		: m_Name(name)
	{
		
	}

	Scene::~Scene()
	{

	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = Ref<Scene>::Create();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;

		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto entity : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(entity).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(entity).Tag;
			Entity newEntity = newScene->CreateEntity(name, uuid);
		}

		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, newScene->m_EntityMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const String& name, UUID id)
	{
		Entity entity(m_Registry.create(), this);
		entity.AddComponent<IDComponent>(id);
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<WorldTransformComponent>();

		m_EntityMap[id] = entity;

		return entity;
	}

	Entity Scene::CreateEntity(const String& name, UUID id, Entity parent)
	{
		Entity entity = CreateEntity(name, id);
		MakeRelationship(parent, entity);

		return entity;
	}

	Entity Scene::CreateEntity(const String& name)
	{
		return CreateEntity(name, UUID());
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity.HasComponent<ChildComponent>())
		{
			// Need to copy array here, because it will be modified in recursive calls
			auto children = entity.GetComponent<ChildComponent>().Children;
			for (Entity e : children)
			{
				DestroyEntity(e);
			}
		}

		if (entity.HasComponent<ParentComponent>())
		{
			Entity parent = entity.GetComponent<ParentComponent>().Parent;
			auto& parentChildren = parent.GetComponent<ChildComponent>().Children;

			DeleteFromChildren(parentChildren, parent, entity);
		}

		if (entity.HasComponent<ScriptComponent>())
			ScriptEngine::OnEntityScriptRemove(entity);

		m_EntityMap.erase(entity.GetID());
		m_Registry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		String name = entity.GetName();
		Entity newEntity;

		if (entity.HasComponent<ParentComponent>())
		{
			Entity entityParent = entity.GetComponent<ParentComponent>().Parent;
			newEntity = CreateEntity(name, UUID(), entityParent);
		}
		else
		{
			newEntity = CreateEntity(name, UUID());
		}

		CopyComponentIfExists(AllComponents{}, newEntity, entity);

		if (entity.HasComponent<ChildComponent>())
		{
			// Copy because next calls of DuplicateEntity in for-loop can reallocate this array
			std::vector<Entity> children = entity.GetComponent<ChildComponent>().Children;

			// Clear new children because, ParentComponent copied in CopyComponentIfExists
			std::vector<Entity>& newChildren = newEntity.GetComponent<ChildComponent>().Children;
			newChildren.clear();

			for (Entity child : children)
			{
				Entity newChild = DuplicateEntity(child);
				MakeRelationship(newEntity, newChild);
			}
		}

		return newEntity;
	}

	void Scene::MakeRelationship(Entity parent, Entity child)
	{
		if (child.HasComponent<ParentComponent>())
		{
			Entity& oldParent = child.GetComponent<ParentComponent>().Parent;
			auto& oldParentChildren = oldParent.GetComponent<ChildComponent>().Children;

			DeleteFromChildren(oldParentChildren, oldParent, child);
			oldParent = parent;
		}
		else
		{
			child.AddComponent<ParentComponent>().Parent = parent;
		}

		if (!parent.HasComponent<ChildComponent>())
			parent.AddComponent<ChildComponent>();

		auto& children = parent.GetComponent<ChildComponent>().Children;
		children.push_back(child);
	}

	void Scene::MakeOrphan(Entity child)
	{
		if (child.HasComponent<ParentComponent>())
		{
			Entity parent = child.GetComponent<ParentComponent>().Parent;
			auto& parentChildren = parent.GetComponent<ChildComponent>().Children;

			DeleteFromChildren(parentChildren, parent, child);
			child.RemoveComponent<ParentComponent>();
		}
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		if(m_EntityMap.contains(uuid))
			return { m_EntityMap.at(uuid), this };

		return Entity{};
	}

	Entity Scene::FindEntityByName(const String& name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			if (view.get<TagComponent>(entity).Tag == name)
				return Entity{ entity, this };
		}

		return {};
	}

	void Scene::OnUpdateEditor(Time frameTime)
	{
		ATN_PROFILE_FUNC();

		UpdateWorldTransforms();

		// Update Animations
		{
			ATN_PROFILE_SCOPE("Scene::UpdateAnimations");
			auto view = m_Registry.view<StaticMeshComponent>();
			for (auto entity : view)
			{
				auto& meshComponent = view.get<StaticMeshComponent>(entity);
				if (meshComponent.Mesh->HasAnimations())
				{
					meshComponent.Mesh->GetAnimator()->OnUpdate(frameTime);
				}
			}
		}
	}

	void Scene::OnUpdateRuntime(Time frameTime)
	{
		ATN_PROFILE_FUNC();

		UpdateWorldTransforms();

		// Update scripts
		{
			ATN_PROFILE_SCOPE("ScriptEngine::OnUpdate");
			auto view = m_Registry.view<ScriptComponent>();
			for (auto id : view)
			{
				Entity entity = { id, this };
				ScriptEngine::OnUpdateEntity(entity, frameTime);
			}
		}

		// Update Animations
		{
			ATN_PROFILE_SCOPE("Scene::UpdateAnimations");
			auto view = m_Registry.view<StaticMeshComponent>();
			for (auto entity : view)
			{
				auto& meshComponent = view.get<StaticMeshComponent>(entity);
				if (meshComponent.Mesh->HasAnimations())
				{
					meshComponent.Mesh->GetAnimator()->OnUpdate(frameTime);
				}
			}
		}

		// Physics
		UpdatePhysics(frameTime);
	}

	void Scene::OnUpdateSimulation(Time frameTime)
	{
		ATN_PROFILE_FUNC();

		UpdateWorldTransforms();

		// Update Animations
		{
			ATN_PROFILE_SCOPE("Scene::UpdateAnimations");
				auto view = m_Registry.view<StaticMeshComponent>();
			for (auto entity : view)
			{
				auto& meshComponent = view.get<StaticMeshComponent>(entity);
				if (meshComponent.Mesh->HasAnimations())
				{
					meshComponent.Mesh->GetAnimator()->OnUpdate(frameTime);
				}
			}
		}

		UpdatePhysics(frameTime);
	}

	void Scene::OnRuntimeStart()
	{
		ATN_PROFILE_FUNC();

		UpdateWorldTransforms();
		OnPhysics2DStart();

		// Scripting
		{
			ATN_PROFILE_SCOPE("ScriptEngine::OnRuntimeStart");

			ScriptEngine::OnRuntimeStart(this);

			// Instantiate all script entities
			auto view = m_Registry.view<ScriptComponent>();
			for (auto id: view)
			{
				Entity entity = { id, this };
				ScriptEngine::InstantiateEntity(entity);
			}

			for (auto id: view)
			{
				Entity entity = { id, this };
				ScriptEngine::OnCreateEntity(entity);
			}
		}
	}

	void Scene::OnSimulationStart()
	{
		UpdateWorldTransforms();
		OnPhysics2DStart();
	}

	void Scene::OnRuntimeStop()
	{
		ScriptEngine::OnRuntimeStop();
	}

	void Scene::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
			{
				cameraComponent.Camera.SetViewportSize(width, height);
			}
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (cameraComponent.Primary)
				return Entity(entity, this);
		}

		return Entity{};
	}

	void Scene::UpdateWorldTransforms()
	{
		ATN_PROFILE_FUNC();

		auto baseEntities = m_Registry.view<WorldTransformComponent, TransformComponent>(entt::exclude<ParentComponent>);
		for (auto entt : baseEntities)
		{
			const TransformComponent& transform = baseEntities.get<TransformComponent>(entt);
			WorldTransformComponent& worldTransform = baseEntities.get<WorldTransformComponent>(entt);

			worldTransform.Translation = transform.Translation;
			worldTransform.Rotation = transform.Rotation;
			worldTransform.Scale = transform.Scale;

			Entity entity = { entt, this };
			if (entity.HasComponent<ChildComponent>())
			{
				const std::vector<Entity>& children = entity.GetComponent<ChildComponent>().Children;

				for(auto& child: children)
					UpdateWorldTransform(child, worldTransform);
			}
		}
	}

	void Scene::UpdateWorldTransform(Entity entity, const WorldTransformComponent& parentTransform)
	{
		const TransformComponent& localTransform = entity.GetComponent<TransformComponent>();
		WorldTransformComponent& worldTransform = entity.GetComponent<WorldTransformComponent>();

		worldTransform.Translation = parentTransform.Translation + parentTransform.Rotation * localTransform.Translation;
		worldTransform.Rotation = parentTransform.Rotation * localTransform.Rotation;
		worldTransform.Scale = parentTransform.Scale * localTransform.Scale;

		if (entity.HasComponent<ChildComponent>())
		{
			const std::vector<Entity>& children = entity.GetComponent<ChildComponent>().Children;

			for (auto& child : children)
				UpdateWorldTransform(child, worldTransform);
		}
	}

	void Scene::OnPhysics2DStart()
	{
		ATN_PROFILE_FUNC();

		const Vector2 gravity = Project::GetActive()->GetConfig().Gravity;

		m_PhysicsWorld = std::make_unique<b2World>(b2Vec2(gravity.x, gravity.y));
		m_Registry.view<Rigidbody2DComponent>().each([this](auto entityID, auto& rb2d)
		{
			Entity entity = Entity(entityID, this);
			const WorldTransformComponent& transform = entity.GetComponent<WorldTransformComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = AthenaRigidBody2DTypeToBox2D(rb2d.Type);
			bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
			bodyDef.angle = transform.Rotation.AsEulerAngles().z;

			b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);
			rb2d.RuntimeBody = reinterpret_cast<void*>(body);

			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y, b2Vec2(bc2d.Offset.y, bc2d.Offset.x), 0.f);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
				circleShape.m_radius = cc2d.Radius * transform.Scale.x;

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}
		});
	}

	void Scene::UpdatePhysics(Time frameTime)
	{
		ATN_PROFILE_FUNC();

		const uint32 velocityIterations = Project::GetActive()->GetConfig().VelocityIterations;
		const uint32 positionIterations = Project::GetActive()->GetConfig().PositionIterations;
		m_PhysicsWorld->Step(frameTime.AsSeconds(), velocityIterations, positionIterations);

		auto rigidBodies2D = GetAllEntitiesWith<Rigidbody2DComponent, WorldTransformComponent, TransformComponent>();
		for (auto entity : rigidBodies2D)
		{
			WorldTransformComponent oldWorldTransform = rigidBodies2D.get<WorldTransformComponent>(entity);
			WorldTransformComponent& newWorldTransform = rigidBodies2D.get<WorldTransformComponent>(entity);

			const auto& rb2D = rigidBodies2D.get<Rigidbody2DComponent>(entity);

			b2Body* body = reinterpret_cast<b2Body*>(rb2D.RuntimeBody);
			const auto& position = body->GetPosition();
			newWorldTransform.Translation.x = position.x;
			newWorldTransform.Translation.y = position.y;

			Vector3 eulerAngles = oldWorldTransform.Rotation.AsEulerAngles();
			eulerAngles.z = body->GetAngle();
			newWorldTransform.Rotation = Quaternion(eulerAngles);

			rigidBodies2D.get<TransformComponent>(entity).UpdateLocalTransform(newWorldTransform, oldWorldTransform);
		}
	}

	void Scene::OnRender(const Ref<SceneRenderer>& renderer, const EditorCamera& camera)
	{
		RenderScene(renderer, camera.GetCameraInfo());
	}

	void Scene::OnRender(const Ref<SceneRenderer>& renderer)
	{
		// Choose camera
		SceneCamera* mainCamera = nullptr;
		WorldTransformComponent cameraTransform;
		{
			auto cameras = m_Registry.view<CameraComponent, WorldTransformComponent>();
			for (auto entity : cameras)
			{
				auto& camera = cameras.get<CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = cameras.get<WorldTransformComponent>(entity);
					break;
				}
			}
		}

		// Render 
		if (mainCamera)
		{
			CameraInfo info = mainCamera->GetCameraInfo();
			info.ViewMatrix = Math::AffineInverse(cameraTransform.AsMatrix());

			RenderScene(renderer, info);
		}
	}

	void Scene::OnRender2D(const Ref<SceneRenderer2D>& renderer2D)
	{
		ATN_PROFILE_FUNC();

		auto quads = GetAllEntitiesWith<SpriteComponent, WorldTransformComponent>();
		for (auto entity : quads)
		{
			const auto& transform = quads.get<WorldTransformComponent>(entity);
			const auto& sprite = quads.get<SpriteComponent>(entity);

			renderer2D->DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Space, sprite.Color, sprite.TilingFactor);
		}

		auto circles = GetAllEntitiesWith<CircleComponent, WorldTransformComponent>();
		for (auto entity : circles)
		{
			const auto& transform = circles.get<WorldTransformComponent>(entity);
			const auto& circle = circles.get<CircleComponent>(entity);

			renderer2D->DrawCircle(transform.AsMatrix(), circle.Space, circle.Color, circle.Thickness, circle.Fade);
		}

		auto textEntities = GetAllEntitiesWith<TextComponent, WorldTransformComponent>();
		for (auto entity : textEntities)
		{
			const auto& transform = textEntities.get<WorldTransformComponent>(entity);
			const auto& text = textEntities.get<TextComponent>(entity);

			TextParams params;
			params.Color = text.Color;
			params.MaxWidth = text.MaxWidth;
			params.Kerning = text.Kerning;
			params.LineSpacing = text.LineSpacing;
			params.Shadowing = text.Shadowing;
			params.ShadowDistance = text.ShadowDistance;
			params.ShadowColor = text.ShadowColor;

			renderer2D->DrawText(text.Text, text.Font, transform.AsMatrix(), text.Space, params);
		}
	}

	void Scene::RenderScene(const Ref<SceneRenderer>& renderer, const CameraInfo& cameraInfo)
	{
		ATN_PROFILE_FUNC();

		renderer->BeginScene(cameraInfo);

		auto staticMeshes = GetAllEntitiesWith<StaticMeshComponent, WorldTransformComponent>();
		for (auto entity : staticMeshes)
		{
			const auto& transform = staticMeshes.get<WorldTransformComponent>(entity);
			const auto& meshComponent = staticMeshes.get<StaticMeshComponent>(entity);

			if (meshComponent.Visible)
			{
				renderer->Submit(meshComponent.Mesh, transform.AsMatrix());
			}
		}

		LightEnvironment lightEnv;

		auto dirLights = GetAllEntitiesWith<DirectionalLightComponent, WorldTransformComponent>();
		for (auto entity : dirLights)
		{
			const auto& transform = dirLights.get<WorldTransformComponent>(entity);
			const auto& light = dirLights.get<DirectionalLightComponent>(entity);

			DirectionalLight dirLight;
			dirLight.Color = light.Color;
			dirLight.Intensity = light.Intensity;
			dirLight.Direction = transform.Rotation * Vector3::Forward();
			dirLight.CastShadows = light.CastShadows;
			dirLight.LightSize = light.LightSize;

			lightEnv.DirectionalLights.push_back(dirLight);
		}

		auto pointLights = GetAllEntitiesWith<PointLightComponent, WorldTransformComponent>();
		for (auto entity : pointLights)
		{
			const auto& transform = pointLights.get<WorldTransformComponent>(entity);
			const auto& light = pointLights.get<PointLightComponent>(entity);

			PointLight pointLight;
			pointLight.Color = light.Color;
			pointLight.Position = transform.Translation;
			pointLight.Intensity = light.Intensity;
			pointLight.Radius = light.Radius;
			pointLight.FallOff = light.FallOff;

			lightEnv.PointLights.push_back(pointLight);
		}

		auto spotLights = GetAllEntitiesWith<SpotLightComponent, WorldTransformComponent>();
		for (auto entity : spotLights)
		{
			const auto& transform = spotLights.get<WorldTransformComponent>(entity);
			const auto& light = spotLights.get<SpotLightComponent>(entity);

			SpotLight spotLight;
			spotLight.Color = light.Color;
			spotLight.Position = transform.Translation;
			spotLight.SpotAngle = light.SpotAngle;
			spotLight.Intensity = light.Intensity;
			spotLight.Direction = transform.Rotation * Vector3::Forward();
			spotLight.Range = light.Range;
			spotLight.RangeFallOff = light.RangeFallOff;
			spotLight.InnerFallOff = light.InnerFallOff;

			lightEnv.SpotLights.push_back(spotLight);
		}

		auto skyLights = GetAllEntitiesWith<SkyLightComponent>();
		if (skyLights.size() > 1)
			ATN_CORE_WARN_TAG("Scene", "Attempt to submit more than 1 SkyLight in the scene!");

		if (!skyLights.empty())
		{
			auto entity = skyLights[0];
			const auto& light = skyLights.get<SkyLightComponent>(entity);

			lightEnv.EnvironmentMap = light.EnvironmentMap;
			lightEnv.EnvironmentMapLOD = light.LOD;
			lightEnv.EnvironmentMapIntensity = light.Intensity;
		}


		renderer->SubmitLightEnvironment(lightEnv);

		renderer->EndScene();
	}
}
