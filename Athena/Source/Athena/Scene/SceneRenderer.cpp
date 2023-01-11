#include "SceneRenderer.h"

#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	struct SceneRendererData
	{
		Ref<Shader> PBRShader;
	};

	static SceneRendererData s_Data;

	void SceneRenderer::Init()
	{
		BufferLayout layout = 
		{
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
		};

		s_Data.PBRShader = Shader::Create(layout, "Resources/Shaders/Renderer_PBR");
	}

	void SceneRenderer::Shutdown()
	{

	}

	void SceneRenderer::Render(Scene* scene, const Matrix4& cameraViewProjection)
	{
		Renderer2D::BeginScene(cameraViewProjection);

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


		Renderer::BeginScene(cameraViewProjection);

		auto entities = scene->GetAllEntitiesWith<TransformComponent, MeshComponent>();
		for (auto entity : entities)
		{
			auto [transform, mesh] = entities.get<TransformComponent, MeshComponent>(entity);

			if (!mesh.Hide)
			{
				Renderer::Submit(s_Data.PBRShader, mesh.Mesh->GetVertexBuffer(), transform.AsMatrix());
			}
		}

		Renderer::EndScene();
	}

	void SceneRenderer::RenderEditorScene(Scene* scene, const EditorCamera& camera)
	{
		auto viewProjection = camera.GetViewProjectionMatrix();
		Renderer2D::BeginScene(viewProjection);

		auto quads = scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor, (int)entity);
		}

		auto circles = scene->GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewProjection);

		auto entities = scene->GetAllEntitiesWith<TransformComponent, MeshComponent>();
		for (auto entity : entities)
		{
			auto [transform, mesh] = entities.get<TransformComponent, MeshComponent>(entity);

			if (!mesh.Hide)
			{
				Renderer::Submit(s_Data.PBRShader, mesh.Mesh->GetVertexBuffer(), transform.AsMatrix());
			}
		}

		Renderer::EndScene();
	}
}
