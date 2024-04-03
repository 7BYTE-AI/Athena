#include "SceneRenderer2D.h"

#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	Ref<SceneRenderer2D> SceneRenderer2D::Create(const Ref<RenderPass>& renderPass)
	{
		Ref<SceneRenderer2D> result = Ref<SceneRenderer2D>::Create();
		result->Init(renderPass);

		return result;
	}

	SceneRenderer2D::~SceneRenderer2D()
	{
		Shutdown();
	}

	void SceneRenderer2D::Init(const Ref<RenderPass>& renderPass)
	{
		m_IndicesCount.resize(Renderer::GetFramesInFlight());

		IndexBufferCreateInfo indexBufferInfo;
		indexBufferInfo.Name = "Renderer2D_QuadIB";
		indexBufferInfo.Data = nullptr;
		indexBufferInfo.Count = 1 * sizeof(uint32);
		indexBufferInfo.Flags = BufferMemoryFlags::CPU_WRITEABLE;

		m_IndexBuffer = IndexBuffer::Create(indexBufferInfo);

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = "Renderer2D_LineVB";
		vertexBufferInfo.Data = nullptr;
		vertexBufferInfo.Size = 1 * sizeof(LineVertex);
		vertexBufferInfo.IndexBuffer = nullptr;
		vertexBufferInfo.Flags = BufferMemoryFlags::CPU_WRITEABLE;

		m_LineVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		vertexBufferInfo.Name = "Renderer2D_CircleVB";
		vertexBufferInfo.Size = 1 * sizeof(CircleVertex);
		vertexBufferInfo.IndexBuffer = m_IndexBuffer.Get();

		m_CircleVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		vertexBufferInfo.Name = "Renderer2D_QuadVB_0";
		vertexBufferInfo.Size = 1 * sizeof(QuadVertex);
		vertexBufferInfo.IndexBuffer = m_IndexBuffer.Get();

		m_QuadVertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		m_TextureSlots[0] = Renderer::GetWhiteTexture();

		m_QuadVertexPositions[0] = { -0.5f,0.5f, 0.f, 1.f };
		m_QuadVertexPositions[1] = { 0.5f, 0.5f, 0.f, 1.f };
		m_QuadVertexPositions[2] = { 0.5f, -0.5f, 0.f, 1.f };
		m_QuadVertexPositions[3] = { -0.5f, -0.5f, 0.f, 1.f };

		
		PipelineCreateInfo pipelineInfo;
		pipelineInfo.Name = "QuadPipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Quad");
		pipelineInfo.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Float2, "a_TexCoords"},
			{ ShaderDataType::UInt,   "a_TexIndex" }};

		pipelineInfo.Topology = Topology::TRIANGLE_LIST;
		pipelineInfo.CullMode = CullMode::NONE;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_QuadPipeline = Pipeline::Create(pipelineInfo);
		m_QuadPipeline->Bake();

		m_QuadBatchIndex = 0;
		QuadBatch quadBatch;
		quadBatch.IndexCount = 0;
		quadBatch.VertexOffset = 0;
		quadBatch.Material = Material::Create(m_QuadPipeline->GetInfo().Shader, std::format("Renderer2D_Quad_{}", m_QuadBatchIndex));
		quadBatch.Material->Set("u_ViewProjection", m_ViewProjectionCamera);

		m_QuadBatches.push_back(quadBatch);

		pipelineInfo.Name = "CirclePipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Circle");
		pipelineInfo.VertexLayout = {
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          }};

		pipelineInfo.Topology = Topology::TRIANGLE_LIST;
		pipelineInfo.CullMode = CullMode::NONE;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_CirclePipeline = Pipeline::Create(pipelineInfo);
		m_CirclePipeline->Bake();

		m_CircleMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);

		pipelineInfo.Name = "LinePipeline";
		pipelineInfo.RenderPass = renderPass;
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Renderer2D_Line");
		pipelineInfo.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    }};

		pipelineInfo.Topology = Topology::LINE_LIST;
		pipelineInfo.CullMode = CullMode::NONE;
		pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
		pipelineInfo.BlendEnable = true;

		m_LinePipeline = Pipeline::Create(pipelineInfo);
		m_LinePipeline->Bake();

		m_LineMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);

		m_LineWidth = 1.f;
		m_LineBatchIndex = 0;
		LineBatch lineBatch;
		lineBatch.VertexCount = 0;
		lineBatch.VertexOffset = 0;
		lineBatch.LineWidth = m_LineWidth;

		m_LineBatches.push_back(lineBatch);
	}

	void SceneRenderer2D::Shutdown()
	{

	}

	void SceneRenderer2D::OnViewportResize(uint32 width, uint32 height)
	{
		m_QuadPipeline->SetViewport(width, height);
		m_LinePipeline->SetViewport(width, height);
		m_CirclePipeline->SetViewport(width, height);
	}

	void SceneRenderer2D::BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		m_BeginScene = true;
		m_RenderCommandBuffer = Renderer::GetRenderCommandBuffer();

		Matrix4 viewProj = viewMatrix * projectionMatrix;

		m_ViewProjectionCamera = viewProj;
		m_InverseViewCamera = Math::AffineInverse(viewMatrix);

		m_CircleMaterial->Set("u_ViewProjection", viewProj);
		m_LineMaterial->Set("u_ViewProjection", viewProj);
		
		for (auto& quadBatch : m_QuadBatches)
		{
			quadBatch.Material->Set("u_ViewProjection", m_ViewProjectionCamera);
			quadBatch.IndexCount = 0;
		}

		m_QuadBatchIndex = 0;
		m_TextureSlotIndex = 1;

		m_CircleIndexCount = 0;

		for (auto& lineBatch : m_LineBatches)
		{
			lineBatch.VertexCount = 0;
		}

		m_LineBatchIndex = 0;
	}

	void SceneRenderer2D::EndScene()
	{
		ATN_PROFILE_FUNC();

		FlushQuads();
		FlushLines();

		FlushIndexBuffer();

		auto commandBuffer = m_RenderCommandBuffer;

		// QUADS
		{
			if (!m_QuadBatches.empty() && m_QuadBatches[0].IndexCount > 0)
			{
				m_QuadVertexBuffer.Flush();
				m_QuadPipeline->Bind(commandBuffer);
			}

			for (const auto& quadBatch : m_QuadBatches)
			{
				if (quadBatch.IndexCount > 0)
				{
					quadBatch.Material->Bind(commandBuffer);

					Renderer::RenderGeometry(commandBuffer, m_QuadPipeline, m_QuadVertexBuffer.Get(),
						quadBatch.Material, quadBatch.VertexOffset, quadBatch.IndexCount);
				}
			}
		}

		// CIRCLES
		if (m_CircleIndexCount != 0)
		{
			m_CircleVertexBuffer.Flush();
			m_CirclePipeline->Bind(commandBuffer);

			Renderer::RenderGeometry(commandBuffer, m_CirclePipeline, m_CircleVertexBuffer.Get(), m_CircleMaterial, 0, m_CircleIndexCount);
		}

		// LINES
		{
			if (!m_LineBatches.empty() && m_LineBatches[0].VertexCount > 0)
			{
				m_LineVertexBuffer.Flush();
				m_LinePipeline->Bind(commandBuffer);
			}

			for (const auto& lineBatch : m_LineBatches)
			{
				if (lineBatch.VertexCount > 0)
				{
					m_LinePipeline->SetLineWidth(commandBuffer, lineBatch.LineWidth);

					Renderer::RenderGeometry(commandBuffer, m_LinePipeline, m_LineVertexBuffer.Get(),
						m_LineMaterial, lineBatch.VertexOffset, lineBatch.VertexCount);
				}
			}
		}

		m_BeginScene = false;
	}

	void SceneRenderer2D::FlushIndexBuffer()
	{
		uint64 quadIndices = 0;
		for (const auto& draw : m_QuadBatches)
			quadIndices += draw.IndexCount;

		uint64 maxIndices = Math::Max(m_CircleIndexCount, quadIndices);

		if (m_IndicesCount[Renderer::GetCurrentFrameIndex()] < maxIndices)
		{
			uint32* indices = new uint32[maxIndices];
			uint32 offset = 0;
			for (uint32 i = 0; i < maxIndices; i += 6)
			{
				indices[i + 0] = offset + 0;
				indices[i + 1] = offset + 1;
				indices[i + 2] = offset + 2;

				indices[i + 3] = offset + 2;
				indices[i + 4] = offset + 3;
				indices[i + 5] = offset + 0;

				offset += 4;
			}

			m_IndexBuffer.Push(indices, maxIndices * sizeof(uint32));
			m_IndexBuffer.Flush();

			delete[] indices;
		}

		m_IndicesCount[Renderer::GetCurrentFrameIndex()] = maxIndices;
	}

	void SceneRenderer2D::FlushQuads()
	{
		const QuadBatch& batch = m_QuadBatches[m_QuadBatchIndex];
		if (batch.IndexCount == 0)
			return;

		for (uint32 i = 0; i < m_TextureSlotIndex; ++i)
			batch.Material->Set("u_Textures", m_TextureSlots[i], i);

		m_TextureSlotIndex = 1;
	}

	void SceneRenderer2D::NextBatchQuads()
	{
		m_QuadBatchIndex++;

		// Offset for next batch
		const QuadBatch& prevBatch = m_QuadBatches[m_QuadBatchIndex - 1];
		uint32 vertexOffset = prevBatch.VertexOffset + 4 * prevBatch.IndexCount / 6;

		// If batch already exists update it offset
		// If not - create new batch
		if (m_QuadBatches.size() <= m_QuadBatchIndex)
		{
			QuadBatch batch;
			batch.IndexCount = 0;
			batch.VertexOffset = vertexOffset;

			batch.Material = Material::Create(m_QuadPipeline->GetInfo().Shader, std::format("Renderer2D_Quad_{}", m_QuadBatchIndex));
			batch.Material->Set("u_ViewProjection", m_ViewProjectionCamera);

			m_QuadBatches.push_back(batch);
		}
		else
		{
			m_QuadBatches[m_QuadBatchIndex].VertexOffset = vertexOffset;
		}
	}

	void SceneRenderer2D::FlushLines()
	{
		LineBatch& batch = m_LineBatches[m_LineBatchIndex];
		batch.LineWidth = m_LineWidth;
	}

	void SceneRenderer2D::NextBatchLines()
	{
		m_LineBatchIndex++;

		// Offset for next batch
		const LineBatch& prevBatch = m_LineBatches[m_LineBatchIndex - 1];
		uint32 vertexOffset = prevBatch.VertexOffset + prevBatch.VertexCount;

		// If batch already exists update it offset
		// If not - create new batch
		if (m_LineBatches.size() <= m_LineBatchIndex)
		{
			LineBatch batch;
			batch.VertexCount = 0;
			batch.VertexOffset = vertexOffset;

			m_LineBatches.push_back(batch);
		}
		else
		{
			m_LineBatches[m_LineBatchIndex].VertexOffset = vertexOffset;
		}
	}

	void SceneRenderer2D::DrawQuad(Vector2 position, Vector2 size, const LinearColor& color)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, color);
	}

	void SceneRenderer2D::DrawQuad(Vector3 position, Vector2 size, const LinearColor& color)
	{
		Matrix4 transform = Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, color);
	}

	void SceneRenderer2D::DrawQuad(Vector2 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawQuad({ position.x, position.y, 0.f }, size, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawQuad(Vector3 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform = ScaleMatrix(Vector3(size.x, size.y, 1.f)).Translate(position);

		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const LinearColor& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, color);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const LinearColor& color)
	{
		Matrix4 transform =
			Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, color);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.f }, size, rotation, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Matrix4 transform =
			Math::ScaleMatrix(Vector3(size.x, size.y, 1.f)).Rotate(rotation, Vector3(0.f, 0.f, 1.f)).Translate(position);

		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const LinearColor& color)
	{
		const uint32 textureIndex = 0; // White Texture
		const Vector2 textureCoords[4] = { {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} };
		const float tilingFactor = 1.f;

		QuadVertex vertices[4];

		for (uint32 i = 0; i < 4; ++i)
		{
			vertices[i].Position = m_QuadVertexPositions[i] * transform;
			vertices[i].Color = color;
			vertices[i].TexCoords = textureCoords[i] * tilingFactor;
			vertices[i].TexIndex = textureIndex;
		}

		m_QuadVertexBuffer.Push(vertices, sizeof(vertices));
		m_QuadBatches[m_QuadBatchIndex].IndexCount += 6;
	}

	void SceneRenderer2D::DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		const auto& texCoords = texture.GetTexCoords();
		int32 textureIndex = 0;

		for (uint32 i = 1; i < m_TextureSlotIndex; ++i)
		{
			if (m_TextureSlots[i] == texture.GetNativeTexture())
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (m_TextureSlotIndex >= s_MaxTextureSlots)
			{
				FlushQuads();
				NextBatchQuads();
			}
			
			textureIndex = m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture.GetNativeTexture();
			m_TextureSlotIndex++;
		}

		QuadVertex vertices[4];

		for (uint32 i = 0; i < 4; ++i)
		{
			vertices[i].Position = m_QuadVertexPositions[i] * transform;
			vertices[i].Color = tint;
			vertices[i].TexCoords = texCoords[i] * tilingFactor;
			vertices[i].TexIndex = textureIndex;
		}

		m_QuadVertexBuffer.Push(vertices, sizeof(vertices));
		m_QuadBatches[m_QuadBatchIndex].IndexCount += 6;
	}

	void SceneRenderer2D::DrawScreenSpaceQuad(const Vector3& position, Vector2 size, const LinearColor& color)
	{
		Vector3 cameraPos = m_InverseViewCamera[3];
		float distance = Math::Distance(cameraPos, position);
		size *= distance;
		Matrix4 transform = Math::ConstructTransform(position, { size.x, size.y, 1 }, Quaternion(1, 0, 0, 0) * m_InverseViewCamera);
		DrawQuad(transform, color);
	}

	void SceneRenderer2D::DrawScreenSpaceQuad(const Vector3& position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint, float tilingFactor)
	{
		Vector3 cameraPos = m_InverseViewCamera[3];
		float distance = Math::Distance(cameraPos, position);
		size *= distance;
		Matrix4 transform = Math::ConstructTransform(position, { size.x, size.y, 1 }, Quaternion(1, 0, 0, 0) * m_InverseViewCamera);
		DrawQuad(transform, texture, tint, tilingFactor);
	}

	void SceneRenderer2D::DrawCircle(const Matrix4& transform, const LinearColor& color, float thickness, float fade)
	{
		CircleVertex vertices[4];

		for (uint32 i = 0; i < 4; ++i)
		{
			vertices[i].WorldPosition = m_QuadVertexPositions[i] * transform;
			vertices[i].LocalPosition = m_QuadVertexPositions[i] * 2.f;
			vertices[i].Color = color;
			vertices[i].Thickness = thickness;
			vertices[i].Fade = fade;
		}

		m_CircleVertexBuffer.Push(vertices, sizeof(vertices));
		m_CircleIndexCount += 6;
	}

	void SceneRenderer2D::DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color)
	{
		LineVertex vertices[2];

		vertices[0].Position = p0;
		vertices[0].Color = color;

		vertices[1].Position = p1;
		vertices[1].Color = color;

		m_LineVertexBuffer.Push(vertices, sizeof(vertices));
		m_LineBatches[m_LineBatchIndex].VertexCount += 2;
	}

	void SceneRenderer2D::DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color)
	{
		Vector3 p0 = Vector3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p1 = Vector3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		Vector3 p2 = Vector3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		Vector3 p3 = Vector3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color);
		DrawLine(p1, p2, color);
		DrawLine(p2, p3, color);
		DrawLine(p3, p0, color);
	}

	void SceneRenderer2D::DrawRect(const Matrix4& transform, const LinearColor& color)
	{
		Vector3 lineVertices[4];
		for (uint32 i = 0; i < 4; ++i)
			lineVertices[i] = m_QuadVertexPositions[i] * transform;

		DrawLine(lineVertices[0], lineVertices[1], color);
		DrawLine(lineVertices[1], lineVertices[2], color);
		DrawLine(lineVertices[2], lineVertices[3], color);
		DrawLine(lineVertices[3], lineVertices[0], color);
	}

	void SceneRenderer2D::SetLineWidth(float width)
	{
		if (m_BeginScene && m_LineBatches[m_LineBatchIndex].VertexCount != 0)
		{
			FlushLines();
			NextBatchLines();
		}

		m_LineWidth = width;
	}

	float SceneRenderer2D::GetLineWidth()
	{
		return m_LineWidth;
	}
}
