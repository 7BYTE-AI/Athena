#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/EnvironmentMap.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/SceneCamera.h"
#include "Athena/Scripting/ScriptEngine.h"


namespace Athena
{
	class ATHENA_API Scene;

	struct IDComponent
	{
		UUID ID;
	};

	struct TagComponent
	{
		String Tag;

		TagComponent(const String& tag)
			: Tag(tag) {}
	};

	struct WorldTransformComponent
	{
		Vector3 Translation = { 0.f, 0.f, 0.f };
		Quaternion Rotation = { 1.f, 0.f, 0.f, 0.f };
		Vector3 Scale = { 1.f, 1.f, 1.f };

		WorldTransformComponent() = default;
		WorldTransformComponent(const Vector3& position)
			: Translation(position) {}

		Matrix4 AsMatrix() const
		{
			return Math::ConstructTransform(Translation, Scale, Rotation);
		}
	};

	struct TransformComponent
	{
		Vector3 Translation = { 0.f, 0.f, 0.f };
		Quaternion Rotation = { 1.f, 0.f, 0.f, 0.f };
		Vector3 Scale = { 1.f, 1.f, 1.f };

		TransformComponent() = default;
		TransformComponent(const Vector3& position)
			: Translation(position) {}

		Matrix4 AsMatrix() const
		{
			return Math::ConstructTransform(Translation, Scale, Rotation);
		}

		TransformComponent& UpdateLocalTransform(const WorldTransformComponent& newWorldTransform, const WorldTransformComponent& oldWorldTransform)
		{
			TransformComponent parentTransform;
			parentTransform.Rotation = oldWorldTransform.Rotation * Rotation.GetInversed();
			parentTransform.Translation = -(parentTransform.Rotation * Translation) + oldWorldTransform.Translation;
			parentTransform.Scale = (Vector3(1.f) / Scale) * oldWorldTransform.Scale;

			Translation = parentTransform.Rotation.GetInversed() * (-parentTransform.Translation + newWorldTransform.Translation);
			Rotation = parentTransform.Rotation.GetInversed() * newWorldTransform.Rotation;
			Scale = (Vector3(1.f) / parentTransform.Scale) * newWorldTransform.Scale;

			return *this;
		}
	};

	struct ChildComponent
	{
		std::vector<Entity> Children;
	};

	struct ParentComponent
	{
		Entity Parent;
	};

	struct SpriteComponent
	{
		Renderer2DSpace Space = Renderer2DSpace::WorldSpace;
		LinearColor Color;
		Texture2DInstance Texture;
		float TilingFactor;

		SpriteComponent(const LinearColor& color = LinearColor::White)
			: Color(color), Texture(TextureGenerator::GetWhiteTexture()), TilingFactor(1.f) {}

		SpriteComponent(const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f)
			: Color(tint), Texture(texture), TilingFactor(tilingFactor) {}
	};

	struct CircleComponent
	{
		Renderer2DSpace Space = Renderer2DSpace::WorldSpace;
		LinearColor Color;
		float Thickness = 1.f;
		float Fade = 0.005f;

		CircleComponent(const LinearColor& color = LinearColor::White)
			: Color(color) {}
	};

	struct TextComponent
	{
		String Text;
		Ref<Font> Font = Font::GetDefault();
		Renderer2DSpace Space = Renderer2DSpace::WorldSpace;
		LinearColor Color = LinearColor::White;
		float MaxWidth = 10.f;
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
		bool Shadowing = false;
		float ShadowDistance = 1.f;
		LinearColor ShadowColor = LinearColor::Black;

		TextComponent() = default;
		TextComponent(TextComponent&& other) = default;
		TextComponent& operator=(TextComponent&& other) noexcept = default;

		TextComponent(const TextComponent& other)
		{
			Font = Font::Create(other.Font->GetFilePath());
			Text = other.Text;
			Space = other.Space;
			Color = other.Color;
			MaxWidth = other.MaxWidth;
			Kerning = other.Kerning;
			LineSpacing = other.LineSpacing;
			Shadowing = other.Shadowing;
			ShadowDistance = other.ShadowDistance;
			ShadowColor = other.ShadowColor;
		}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;
	};

	struct ScriptComponent
	{
		std::string Name;
	};


	struct Rigidbody2DComponent
	{
		enum class BodyType { STATIC = 0, DYNAMIC = 1, KINEMATIC = 2 };
		BodyType Type = BodyType::STATIC;
		bool FixedRotation = false;

		//Storage
		void* RuntimeBody = nullptr;
	};

	struct BoxCollider2DComponent
	{
		Vector2 Offset = { 0.f, 0.f };
		Vector2 Size = { 0.5f, 0.5f };

		float Density = 1.f;
		float Friction = 0.5f;
		float Restitution = 0.f;
		float RestitutionThreshold = 0.5f;

		//Storage
		void* RuntimeFixture = nullptr;
	};

	struct CircleCollider2DComponent
	{
		Vector2 Offset = { 0.f, 0.f };
		float Radius = 0.5f;

		float Density = 1.f;
		float Friction = 0.5f;
		float Restitution = 0.f;
		float RestitutionThreshold = 0.5f;

		//Storage
		void* RuntimeFixture = nullptr;
	};

	struct StaticMeshComponent
	{
		Ref<StaticMesh> Mesh;
		bool Visible = true;

		StaticMeshComponent() = default;
		StaticMeshComponent(StaticMeshComponent&& other) = default;
		StaticMeshComponent& operator=(StaticMeshComponent&& other) noexcept = default;

		StaticMeshComponent(const StaticMeshComponent& other)
		{
			Mesh = StaticMesh::Create(other.Mesh->GetFilePath());
			Visible = other.Visible;
		}
	};

	struct DirectionalLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		bool CastShadows = true;
		float LightSize = 0.4f;
	};

	struct PointLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		float Radius = 10.f;
		float FallOff = 1.f;
	};

	struct SpotLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		float SpotAngle = 30.f;
		float InnerFallOff = 1.f;
		float Range = 10.f;
		float RangeFallOff = 1.f;
	};

	struct SkyLightComponent
	{
		Ref<EnvironmentMap> EnvironmentMap;
		float LOD = 0.f;
		float Intensity = 1.f;

		SkyLightComponent()
		{
			EnvironmentMap = EnvironmentMap::Create(256);
		}

		SkyLightComponent(const SkyLightComponent& other)
		{
			LOD = other.LOD;
			Intensity = other.Intensity;

			const auto& otherEnv = other.EnvironmentMap;

			EnvironmentMap = EnvironmentMap::Create(otherEnv->GetResolution());
			EnvironmentMap->SetType(otherEnv->GetType());
			EnvironmentMap->SetFilePath(otherEnv->GetFilePath());
			float turbidity = otherEnv->GetTurbidity();
			float azimuth = otherEnv->GetAzimuth();
			float inclination = otherEnv->GetInclination();
			EnvironmentMap->SetPreethamParams(turbidity, azimuth, inclination);
		}

		SkyLightComponent(SkyLightComponent&& other) noexcept
		{
			EnvironmentMap = other.EnvironmentMap;
			LOD = other.LOD;
			Intensity = other.Intensity;
			other.EnvironmentMap = nullptr;
		}

		SkyLightComponent& operator=(SkyLightComponent&& other) noexcept
		{
			if (&other != this)
			{
				EnvironmentMap = other.EnvironmentMap;
				LOD = other.LOD;
				Intensity = other.Intensity;
				other.EnvironmentMap = nullptr;
			}

			return *this;
		}
	};


	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents =
		ComponentGroup<WorldTransformComponent, TransformComponent, ParentComponent, ChildComponent,
		SpriteComponent, CircleComponent, TextComponent, 
		ScriptComponent, CameraComponent,
		Rigidbody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent, 
		StaticMeshComponent, 
		DirectionalLightComponent, PointLightComponent, SpotLightComponent, SkyLightComponent>;


	template <typename T>
	inline void Scene::OnComponentAdd(Entity entity, T& component) {}

	template<typename T>
	inline void Scene::OnComponentRemove(Entity entity, T& component) {}

	template <>
	inline void Scene::OnComponentAdd<CameraComponent>(Entity entity, CameraComponent& camera)
	{
		camera.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template <>
	inline void Scene::OnComponentRemove<ScriptComponent>(Entity entity, ScriptComponent& script)
	{
		ScriptEngine::OnEntityScriptRemove(entity);
	}
}
