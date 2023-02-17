#include "SceneRenderer.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	void SceneRenderer::Init()
	{

	}

	void SceneRenderer::Shutdown()
	{

	}

	void SceneRenderer::Render(Scene* scene, const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		Renderer2D::BeginScene(viewMatrix, projectionMatrix);

		auto quads = scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor);
		}

		auto circles = scene->GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewMatrix, projectionMatrix, scene->GetEnvironment());

		auto staticMeshes = scene->GetAllEntitiesWith<TransformComponent, StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto [transform, meshComponent] = staticMeshes.get<TransformComponent, StaticMeshComponent>(entity);

			if (!meshComponent.Hide)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::GetMaterial(subMeshes[i].MaterialName);
					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
					
					if (animator != nullptr && animator->IsPlaying())
					{
						Renderer::SubmitWithAnimation(subMeshes[i].VertexBuffer, material, animator->IsPlaying() ? animator : nullptr, transform.AsMatrix(), (int32)entity);
					}
					else
					{
						Renderer::Submit(subMeshes[i].VertexBuffer, material, transform.AsMatrix(), (int32)entity);
					}
				}
			}
		}

		auto dirLights = scene->GetAllEntitiesWith<DirectionalLightComponent>();
		for (auto entity : dirLights)
		{
			auto light = dirLights.get<DirectionalLightComponent>(entity);
			DirectionalLight dirLight = { light.Color, light.Direction, light.Intensity };
			Renderer::SubmitLight(dirLight);
		}

		auto pointLights = scene->GetAllEntitiesWith<TransformComponent, PointLightComponent>();
		for (auto entity : pointLights)
		{
			auto [transform, light] = pointLights.get<TransformComponent, PointLightComponent>(entity);
			PointLight pointLight = { light.Color, transform.Translation, light.Intensity, light.Radius, light.FallOff };
			Renderer::SubmitLight(pointLight);
		}

		Renderer::EndScene();
	}

	void SceneRenderer::RenderEditorScene(Scene* scene, const EditorCamera& camera)
	{
		Matrix4 viewMatrix = camera.GetViewMatrix();
		Matrix4 projectionMatrix = camera.GetProjectionMatrix();

		Renderer2D::BeginScene(viewMatrix, projectionMatrix);

		auto quads = scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor, (int32)entity);
		}

		auto circles = scene->GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade, (int32)entity);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewMatrix, projectionMatrix, scene->GetEnvironment());

		auto staticMeshes = scene->GetAllEntitiesWith<TransformComponent, StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto [transform, meshComponent] = staticMeshes.get<TransformComponent, StaticMeshComponent>(entity);

			if (!meshComponent.Hide)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::GetMaterial(subMeshes[i].MaterialName);
					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();

					if (animator != nullptr && animator->IsPlaying())
					{
						Renderer::SubmitWithAnimation(subMeshes[i].VertexBuffer, material, animator->IsPlaying() ? animator : nullptr, transform.AsMatrix(), (int32)entity);
					}
					else
					{
						Renderer::Submit(subMeshes[i].VertexBuffer, material, transform.AsMatrix(), (int32)entity);
					}
				}
			}
		}

		auto dirLights = scene->GetAllEntitiesWith<DirectionalLightComponent>();
		for(auto entity: dirLights)
		{
			auto light = dirLights.get<DirectionalLightComponent>(entity);
			DirectionalLight dirLight = { light.Color, light.Direction, light.Intensity };
			Renderer::SubmitLight(dirLight);
		}

		auto pointLights = scene->GetAllEntitiesWith<TransformComponent, PointLightComponent>();
		for (auto entity : pointLights)
		{
			auto [transform, light] = pointLights.get<TransformComponent, PointLightComponent>(entity);
			PointLight pointLight = { light.Color, transform.Translation, light.Intensity, light.Radius, light.FallOff };
			Renderer::SubmitLight(pointLight);
		}

		Renderer::EndScene();
	}
}
