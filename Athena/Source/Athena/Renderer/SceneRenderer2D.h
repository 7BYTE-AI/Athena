#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Renderer/Material.h"


namespace Athena
{
	struct QuadVertex
	{
		Vector3 Position;
		LinearColor Color;
		Vector2 TexCoord;
		int32 TexIndex;
		float TilingFactor;
	};

	struct CircleVertex
	{
		Vector3 WorldPosition;
		Vector3 LocalPosition;
		LinearColor Color;
		float Thickness;
		float Fade;
	};

	struct LineVertex
	{
		Vector3 Position;
		LinearColor Color;
	};

	class ATHENA_API SceneRenderer2D : public RefCounted
	{
	public:
		static Ref<SceneRenderer2D> Create(const Ref<RenderPass>& renderPass);
		~SceneRenderer2D();

		void Init(const Ref<RenderPass>& renderPass);
		void Shutdown();

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix);
		void EndScene();
		void Flush();

		void DrawQuad(const Vector2& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		void DrawQuad(const Vector3& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		void DrawQuad(const Vector2& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		void DrawQuad(const Vector3& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const LinearColor& color = LinearColor::White);
		void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const LinearColor& color = LinearColor::White);
		void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawQuad(const Matrix4& transform, const LinearColor& color = LinearColor::White);
		void DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawCircle(const Matrix4& transform, const LinearColor& color = LinearColor::White, float thickness = 1.f, float fade = 0.005f);

		void DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color = LinearColor::White);

		void DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		void DrawRect(const Matrix4& transform, const LinearColor& color = LinearColor::White);

		void SetLineWidth(float width);
		float GetLineWidth();

	private:
		void StartBatch();
		void NextBatch();

	private:
		// Max Geometry per batch
		const uint32 s_MaxQuads = 500;
		const uint32 s_MaxQuadVertices = s_MaxQuads * 4;
		const uint32 s_MaxCircles = 300;
		const uint32 s_MaxCircleVertices = s_MaxCircles * 4;
		const uint32 s_MaxLines = 300;
		const uint32 s_MaxLineVertices = s_MaxLines * 2;
		const uint32 s_MaxIndices = Math::Max(s_MaxQuads, s_MaxCircles) * 6;

		static const uint32 s_MaxTextureSlots = 32;   // TODO: RenderCaps

	private:
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;

		Ref<Pipeline> m_QuadPipeline;
		Ref<Pipeline> m_CirclePipeline;
		Ref<Pipeline> m_LinePipeline;

		Ref<Material> m_QuadMaterial;
		Ref<Material> m_CircleMaterial;
		Ref<Material> m_LineMaterial;

		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<VertexBuffer> m_CircleVertexBuffer;
		Ref<VertexBuffer> m_LineVertexBuffer;

		uint32 m_QuadIndexCount = 0;
		QuadVertex* m_QuadVertexBufferBase = nullptr;
		QuadVertex* m_QuadVertexBufferPointer = nullptr;

		uint32 m_CircleIndexCount = 0;
		CircleVertex* m_CircleVertexBufferBase = nullptr;
		CircleVertex* m_CircleVertexBufferPointer = nullptr;

		uint32 m_LineVertexCount = 0;
		LineVertex* m_LineVertexBufferBase = nullptr;
		LineVertex* m_LineVertexBufferPointer = nullptr;

		std::array<Ref<Texture2D>, s_MaxTextureSlots> m_TextureSlots;
		uint32 m_TextureSlotIndex = 1; // 0 - white texture

		float m_LineWidth = 1.f;
		Vector4 m_QuadVertexPositions[4];
	};
}
