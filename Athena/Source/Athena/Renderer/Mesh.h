#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/AABB.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"

#include <vector>


class aiScene;
class aiNode;


namespace Athena
{
	struct StaticVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;

		static VertexMemoryLayout GetLayout()
		{
			return {
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoords" },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" } };
		}
	};

	struct AnimVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
		int BoneIDs[ShaderDef::MAX_NUM_BONES_PER_VERTEX];
		float Weights[ShaderDef::MAX_NUM_BONES_PER_VERTEX];

		static VertexMemoryLayout GetLayout()
		{
			return {
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoords" },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" },
				{ ShaderDataType::Int4,   "a_Tangent"   },
				{ ShaderDataType::Float4, "a_Bitangent" } };
		}
	};

	struct SubMesh
	{
		String Name;
		String MaterialName;
		Ref<VertexBuffer> VertexBuffer;
	};

	class ATHENA_API StaticMesh
	{
	public:
		static Ref<StaticMesh> Create(const FilePath& path);

		const std::vector<SubMesh>& GetAllSubMeshes() const { return m_SubMeshes; }

		const String& GetName() const { return m_Name; }
		const FilePath& GetFilePath() const { return m_FilePath; }
		const AABB& GetBoundingBox() const { return m_AABB; }
		const Ref<MaterialTable>& GetMaterialTable() const { return m_MaterialTable; }
		
		const Ref<Animator>& GetAnimator() { return m_Animator; }

		bool HasAnimations() const { return m_Animator != nullptr; }

	private:
		void ProcessNode(const aiScene* aiscene, const aiNode* ainode, const Matrix4& parentTransform);

	private:
		FilePath m_FilePath;
		String m_Name;
		AABB m_AABB;
		std::vector<SubMesh> m_SubMeshes;
		Ref<MaterialTable> m_MaterialTable;

		Ref<Skeleton> m_Skeleton;
		Ref<Animator> m_Animator;
	};
}
