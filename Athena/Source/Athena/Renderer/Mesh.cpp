#include "Mesh.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/Renderer/Renderer.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>


namespace Athena
{
	static Matrix4 ConvertaiMatrix4x4(const aiMatrix4x4& input)
	{
		Matrix4 output;

		for (uint32 i = 0; i < 4; ++i)
		{
			for (uint32 j = 0; j < 4; ++j)
			{
				output[i][j] = input[j][i];
			}
		}

		return output;
	}

	static Quaternion ConvertaiQuaternion(const aiQuaternion& quat)
	{
		return { quat.w, quat.x, quat.y, quat.z };
	}

	static Vector3 ConvertaiVector3D(const aiVector3D& input)
	{
		return { input.x, input.y, input.z };
	}

	static String ConvertaiStringName(const aiString& aiName)
	{
		const uint32 nameMaxLength = 30;

		if (aiName.length >= nameMaxLength)
		{
			String name = aiName.C_Str();
			return name.substr(0, nameMaxLength);
		}

		return aiName.C_Str();
	}

	static Ref<Texture2D> LoadTexture(const aiScene* aiscene, const aiMaterial* aimaterial, uint32 type, bool srgb, const FilePath& path)
	{
		Ref<Texture2D> result = nullptr;

		aiString texFilepath;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(type, 0), texFilepath))
		{
			TextureImportOptions options;
			options.sRGB = srgb;
			options.GenerateMipMaps = true;

			const aiTexture* embeddedTex = aiscene->GetEmbeddedTexture(texFilepath.C_Str());
			if (embeddedTex)
			{
				void* data = embeddedTex->pcData;
				uint32 width = embeddedTex->mWidth;
				uint32 height = embeddedTex->mHeight;

				options.Name = String(texFilepath.C_Str(), texFilepath.length);
				result = TextureImporter::Load(data, width, height, options);
			}
			else
			{
				FilePath path = path;
				path.replace_filename(texFilepath.C_Str());
				if (FileSystem::Exists(path))
				{
					result = TextureImporter::Load(path, options);
				}
				else
				{
					ATN_CORE_WARN_TAG("StaticMesh", "Invalid texture filepath '{}'", path);
				}
			}
		}

		return result;
	}

	static Ref<Material> LoadMaterial(const aiScene* aiscene, uint32 aiMaterialIndex, const FilePath& path, Ref<MaterialTable> table, bool animated)
	{
		Ref<Material> result;
		const aiMaterial* aimaterial = aiscene->mMaterials[aiMaterialIndex];
		String materialName = aimaterial->GetName().C_Str();

		if (table->Exists(materialName))
			return table->Get(materialName);

		if (animated)
			result = Material::CreatePBRAnim(materialName);
		else
			result = Material::CreatePBRStatic(materialName);

		aiColor4D color;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_BASE_COLOR, color))
			result->Set("u_Albedo", Vector4(color.r, color.g, color.b, color.a));
		else if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			result->Set("u_Albedo", Vector4(color.r, color.g, color.b, color.a));

		float roughness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
			result->Set("u_Roughness", roughness);

		float metalness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
			result->Set("u_Metalness", metalness);

		float emission;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emission))
			result->Set("u_Emission", emission);
		
		result->Set("u_UseAlbedoMap", (uint32)true);
		result->Set("u_UseNormalMap", (uint32)false);
		result->Set("u_UseRoughnessMap", (uint32)false);
		result->Set("u_UseMetalnessMap", (uint32)false);

		Ref<Texture2D> texture;

		if (texture = LoadTexture(aiscene, aimaterial, aiTextureType_BASE_COLOR, true, path))
			result->Set("u_AlbedoMap", texture);
		else if (texture = LoadTexture(aiscene, aimaterial, aiTextureType_DIFFUSE, true, path))
			result->Set("u_AlbedoMap", texture);

		result->Set("u_UseAlbedoMap", uint32(texture != nullptr));

		if (texture = LoadTexture(aiscene, aimaterial, aiTextureType_NORMALS, false, path))
			result->Set("u_NormalMap", texture);

		result->Set("u_UseNormalMap", uint32(texture != nullptr));

		if (texture = LoadTexture(aiscene, aimaterial, aiTextureType_DIFFUSE_ROUGHNESS, false, path))
			result->Set("u_RoughnessMap", texture);
		else if (texture = LoadTexture(aiscene, aimaterial, aiTextureType_SHININESS, false, path))
			result->Set("u_RoughnessMap", texture);

		result->Set("u_UseRoughnessMap", uint32(texture != nullptr));

		if (texture = LoadTexture(aiscene, aimaterial, aiTextureType_METALNESS, false, path))
			result->Set("u_MetalnessMap", texture);

		result->Set("u_UseMetalnessMap", uint32(texture != nullptr));
		
		table->Add(result);
		return result;
	}

	static Ref<VertexBuffer> LoadStaticVertexBuffer(const aiMesh* aimesh, const Matrix4& localTransform)
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<StaticVertex> vertices(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			if (aimesh->HasPositions())
			{
				vertices[i].Position = ConvertaiVector3D(aimesh->mVertices[i]) * localTransform;
			}

			// TexCoord
			for (uint32 j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j)
			{
				if (aimesh->HasTextureCoords(j))
				{
					vertices[i].TexCoords.x = aimesh->mTextureCoords[j][i].x;
					vertices[i].TexCoords.y = aimesh->mTextureCoords[j][i].y;
					break;
				}
			}

			// Normal
			if (aimesh->HasNormals())
			{
				vertices[i].Normal = Vector4(ConvertaiVector3D(aimesh->mNormals[i]), 0) * localTransform;
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangent
				vertices[i].Tangent = Vector4(ConvertaiVector3D(aimesh->mTangents[i]), 0) * localTransform;
				// Bitangent
				vertices[i].Bitangent = Vector4(ConvertaiVector3D(aimesh->mBitangents[i]), 0) * localTransform;
			}
		}

		uint32 numFaces = aimesh->mNumFaces;
		aiFace* faces = aimesh->mFaces;

		std::vector<uint32> indices(numFaces * 3);

		uint32 index = 0;
		for (uint32 i = 0; i < numFaces; i++)
		{
			if (faces[i].mNumIndices != 3)
				break;

			indices[index++] = faces[i].mIndices[0];
			indices[index++] = faces[i].mIndices[1];
			indices[index++] = faces[i].mIndices[2];
		}

		Ref<IndexBuffer> indexBuffer = nullptr;
		if (!indices.empty())
		{
			IndexBufferCreateInfo indexBufferInfo;
			indexBufferInfo.Name = std::format("{}_IndexBuffer", ConvertaiStringName(aimesh->mName));
			indexBufferInfo.Data = indices.data();
			indexBufferInfo.Count = indices.size();
			indexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			indexBuffer = IndexBuffer::Create(indexBufferInfo);
		}

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = std::format("{}_VertexBuffer", ConvertaiStringName(aimesh->mName));
		vertexBufferInfo.Data = vertices.data();
		vertexBufferInfo.Size = vertices.size() * sizeof(StaticVertex);
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

		return VertexBuffer::Create(vertexBufferInfo);
	}

	static Ref<VertexBuffer> LoadAnimVertexBuffer(const aiMesh* aimesh, const Matrix4& localTransform, const Ref<Skeleton>& skeleton)
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<AnimVertex> vertices(numVertices);

		if (aimesh->HasBones() && skeleton)
		{
			for (uint32 i = 0; i < aimesh->mNumBones; ++i)
			{
				aiBone* aibone = aimesh->mBones[i];
				uint32 boneID = skeleton->GetBoneIndex(aibone->mName.C_Str());
				skeleton->SetBoneOffsetMatrix(boneID, ConvertaiMatrix4x4(aibone->mOffsetMatrix));

				for (uint32 j = 0; j < aibone->mNumWeights; ++j)
				{
					uint32 vertexID = aibone->mWeights[j].mVertexId;
					float weight = aibone->mWeights[j].mWeight;
					for (uint32 k = 0; k < ShaderDef::MAX_NUM_BONES_PER_VERTEX; ++k)
					{
						if (vertices[vertexID].Weights[k] == 0.f)
						{
							vertices[vertexID].BoneIDs[k] = boneID;
							vertices[vertexID].Weights[k] = weight;
							break;
						}
						else if (k == ShaderDef::MAX_NUM_BONES_PER_VERTEX - 1)
						{
							ATN_CORE_WARN_TAG("StaticMesh", "Vertex has more than four bones/weights affecting it, extra data will be dicarded(BoneID = {}, Weight = {})",
								boneID, weight);
						}
					}
				}
			}
		}

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			if (aimesh->HasPositions())
			{
				vertices[i].Position = ConvertaiVector3D(aimesh->mVertices[i]) * localTransform;
			}

			// TexCoord
			for (uint32 j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j)
			{
				if (aimesh->HasTextureCoords(j))
				{
					vertices[i].TexCoords.x = aimesh->mTextureCoords[j][i].x;
					vertices[i].TexCoords.y = aimesh->mTextureCoords[j][i].y;
					break;
				}
			}

			// Normal
			if (aimesh->HasNormals())
			{
				vertices[i].Normal = Vector4(ConvertaiVector3D(aimesh->mNormals[i]), 0) * localTransform;
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangent
				vertices[i].Tangent = Vector4(ConvertaiVector3D(aimesh->mTangents[i]), 0) * localTransform;
				// Bitangent
				vertices[i].Bitangent = Vector4(ConvertaiVector3D(aimesh->mBitangents[i]), 0) * localTransform;
			}
		}

		uint32 numFaces = aimesh->mNumFaces;
		aiFace* faces = aimesh->mFaces;

		std::vector<uint32> indices(numFaces * 3);

		uint32 index = 0;
		for (uint32 i = 0; i < numFaces; i++)
		{
			if (faces[i].mNumIndices != 3)
				break;

			indices[index++] = faces[i].mIndices[0];
			indices[index++] = faces[i].mIndices[1];
			indices[index++] = faces[i].mIndices[2];
		}

		Ref<IndexBuffer> indexBuffer = nullptr;
		if (!indices.empty())
		{
			IndexBufferCreateInfo indexBufferInfo;
			indexBufferInfo.Name = std::format("{}_IndexBuffer", ConvertaiStringName(aimesh->mName));
			indexBufferInfo.Data = indices.data();
			indexBufferInfo.Count = indices.size();
			indexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			indexBuffer = IndexBuffer::Create(indexBufferInfo);
		}

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = std::format("{}_VertexBuffer", ConvertaiStringName(aimesh->mName));
		vertexBufferInfo.Data = vertices.data();
		vertexBufferInfo.Size = vertices.size() * sizeof(AnimVertex);
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;
	
		return VertexBuffer::Create(vertexBufferInfo);
	}

	static SubMesh LoadSubMesh(const aiScene* aiscene, uint32 aiMeshIndex, const FilePath& path, const Matrix4& localTransform, Ref<MaterialTable> table, Ref<Skeleton> skeleton, AABB& aabb)
	{
		aiMesh* aimesh = aiscene->mMeshes[aiMeshIndex];
		SubMesh subMesh;

		aabb.Extend(AABB(ConvertaiVector3D(aimesh->mAABB.mMin) * localTransform, ConvertaiVector3D(aimesh->mAABB.mMax) * localTransform));

		subMesh.Name = aimesh->mName.C_Str();
		if(skeleton)
			subMesh.VertexBuffer = LoadAnimVertexBuffer(aimesh, localTransform, skeleton);
		else
			subMesh.VertexBuffer = LoadStaticVertexBuffer(aimesh, localTransform);

		const aiMaterial* aimaterial = aiscene->mMaterials[aimesh->mMaterialIndex];
		String materialName = aimaterial->GetName().C_Str();

		subMesh.MaterialName = LoadMaterial(aiscene, aimesh->mMaterialIndex, path, table, skeleton != nullptr)->GetName();

		return subMesh;
	}

	static void LoadBones(const aiNode* aiBone, std::vector<Bone>& bones)
	{
		String nodeName = aiBone->mName.C_Str();
		bool isValid = nodeName.find("AssimpFbx") == String::npos && nodeName.find("RootNode") == String::npos;
		if (aiBone->mNumChildren > 0 && !isValid)
		{
			LoadBones(aiBone->mChildren[0], bones);
			return;
		}

		Bone bone;
		bone.Name = nodeName;
		bone.Index = bones.size();
		bone.Children.resize(aiBone->mNumChildren);
		bone.OffsetMatrix = Matrix4::Identity();
		bones.push_back(bone);

		for (uint32 i = 0; i < aiBone->mNumChildren; ++i)
		{
			bones[bone.Index].Children[i] = bones.size();
			LoadBones(aiBone->mChildren[i], bones);
		}
	}

	static Ref<Skeleton> LoadSkeleton(const aiScene* aiscene)
	{
		if (aiscene->mNumAnimations <= 0)
			return nullptr;

		const aiNode* root = aiscene->mRootNode;
		const aiNode* bonesRoot = nullptr;

		for (uint32 i = 0; i < root->mNumChildren; ++i)
		{
			String name = root->mChildren[i]->mName.C_Str();
			if (name.find("RootNode") != String::npos || name.find("AssimpFbx") != String::npos)
			{
				bonesRoot = root->mChildren[i];
				break;
			}
		}
		if (bonesRoot == nullptr)
			return nullptr;

		std::vector<Bone> bones;
		LoadBones(bonesRoot, bones);

		return Skeleton::Create(bones);
	}

	static Ref<Animation> LoadAnimation(const aiAnimation* aianimation, const Ref<Skeleton>& skeleton)
	{
		AnimationCreateInfo info;
		info.Name = aianimation->mName.C_Str();
		info.Duration = aianimation->mDuration;
		info.TicksPerSecond = aianimation->mTicksPerSecond;
		info.Skeleton = skeleton;

		info.BoneNameToKeyFramesMap.reserve(aianimation->mNumChannels);
		for (uint32 i = 0; i < aianimation->mNumChannels; ++i)
		{
			aiNodeAnim* channel = aianimation->mChannels[i];
			KeyFramesList keyFrames;

			keyFrames.TranslationKeys.resize(channel->mNumPositionKeys);
			for (uint32 j = 0; j < channel->mNumPositionKeys; ++j)
			{
				keyFrames.TranslationKeys[j].TimeStamp = channel->mPositionKeys[j].mTime;
				keyFrames.TranslationKeys[j].Value = ConvertaiVector3D(channel->mPositionKeys[j].mValue);
			}

			keyFrames.RotationKeys.resize(channel->mNumRotationKeys);
			for (uint32 j = 0; j < channel->mNumRotationKeys; ++j)
			{
				keyFrames.RotationKeys[j].TimeStamp = channel->mRotationKeys[j].mTime;
				keyFrames.RotationKeys[j].Value = ConvertaiQuaternion(channel->mRotationKeys[j].mValue);
			}

			keyFrames.ScaleKeys.resize(channel->mNumScalingKeys);
			for (uint32 j = 0; j < channel->mNumScalingKeys; ++j)
			{
				keyFrames.ScaleKeys[j].TimeStamp = channel->mScalingKeys[j].mTime;
				keyFrames.ScaleKeys[j].Value = ConvertaiVector3D(channel->mScalingKeys[j].mValue);
			}

			info.BoneNameToKeyFramesMap[channel->mNodeName.C_Str()] = keyFrames;
		}

		return Animation::Create(info);
	}

	void StaticMesh::ProcessNode(const aiScene* aiscene, const aiNode* ainode, const Matrix4& parentTransform)
	{
		Matrix4 localTransform = parentTransform * ConvertaiMatrix4x4(ainode->mTransformation);

		m_SubMeshes.reserve(ainode->mNumMeshes);
		for (uint32 i = 0; i < ainode->mNumMeshes; ++i)
		{
			SubMesh subMesh = LoadSubMesh(aiscene, ainode->mMeshes[i], m_FilePath, localTransform, m_MaterialTable, m_Skeleton, m_AABB);
			m_SubMeshes.push_back(subMesh);
		}

		for (uint32 i = 0; i < ainode->mNumChildren; ++i)
		{
			ProcessNode(aiscene, ainode->mChildren[i], localTransform);
		}
	}

	Ref<StaticMesh> StaticMesh::Create(const FilePath& path)
	{
		const unsigned int flags =
			aiProcess_GenUVCoords |
			aiProcess_CalcTangentSpace |
			aiProcess_GenSmoothNormals |
			aiProcess_FixInfacingNormals |
			aiProcess_GenBoundingBoxes |
			aiProcess_FindInvalidData | 

			aiProcess_SortByPType |
			aiProcess_FindDegenerates |
			aiProcess_ImproveCacheLocality |
			aiProcess_JoinIdenticalVertices |
			aiProcess_LimitBoneWeights |

			aiProcess_RemoveRedundantMaterials |
			aiProcess_OptimizeGraph |
			aiProcess_OptimizeMeshes |

			aiProcess_Triangulate |
			aiProcess_EmbedTextures |
			aiProcess_FlipUVs;

		const aiScene* aiscene = aiImportFile(path.string().c_str(), flags);

		if (aiscene == nullptr)
		{
			const char* error = aiGetErrorString();
			ATN_CORE_ERROR_TAG("Mesh", "Failed to load mesh from {}", path);
			ATN_CORE_ERROR("	Assimp Error: {}", error);
			return Ref<StaticMesh>::Create();
		}

		Ref<StaticMesh> result = Ref<StaticMesh>::Create();
		result->m_FilePath = path;
		result->m_Name = path.stem().string();
		result->m_MaterialTable = Ref<MaterialTable>::Create();

		//ATN_CORE_TRACE_TAG("StaticMesh", "Create static mesh from '{}'", path);

		result->m_Skeleton = LoadSkeleton(aiscene);
		result->ProcessNode(aiscene, aiscene->mRootNode, Matrix4::Identity());

		if (result->m_Skeleton)
		{
			std::vector<Ref<Animation>> animations(aiscene->mNumAnimations);

			for (uint32 i = 0; i < aiscene->mNumAnimations; ++i)
			{
				animations[i] = LoadAnimation(aiscene->mAnimations[i], result->m_Skeleton);
			}

			result->m_Animator = Animator::Create(animations, result->m_Skeleton);
		}

		aiReleaseImport(aiscene);

		return result;
	}
}
