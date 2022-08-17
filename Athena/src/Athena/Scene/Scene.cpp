#include "atnpch.h"

#include "Scene.h"
#include "Entity.h"

#include "Components.h"
#include "Athena/Renderer/Renderer2D.h"


namespace Athena
{
	Scene::Scene()
	{

	}

	Scene::~Scene() 
	{

	}

	Entity Scene::CreateEntity(std::string_view name)
	{
		Entity entity(m_Registry.create(), this);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<TagComponent>(name);

		return entity;
	}

	void Scene::OnUpdate(Time frameTime)
	{
		// Render 2D
		Camera* mainCamera = nullptr;
		Matrix4* cameraTransform = nullptr;
		const auto& group = m_Registry.group<CameraComponent>(entt::get<TransformComponent>);
		for (auto entity : group)
		{
			auto& [camera, transformComponent] = group.get<CameraComponent, TransformComponent>(entity);

			if (camera.Primary)
			{
				mainCamera = &camera.Camera;
				cameraTransform = &transformComponent.Transform;
				break;
			}
		}

		if (mainCamera && cameraTransform)
		{
			Renderer2D::BeginScene(*mainCamera, *cameraTransform);

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawQuad(transform, sprite.Color);
			}

			Renderer2D::EndScene();
		}	
	}

	void Scene::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		const auto& view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
			{
				cameraComponent.Camera.SetViewportSize(width, height);
			}
		}
	}
}
