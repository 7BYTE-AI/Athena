#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Math/Quaternion.h"


namespace Athena
{
	struct BonesHierarchyInfo
	{
		String Name;
		std::vector<BonesHierarchyInfo> Children;
	};

	struct Bone
	{
		String Name;
		uint32 ID;

		std::vector<Bone> Children;
	};

	class ATHENA_API Skeleton
	{
	public:
		static Ref<Skeleton> Create(const BonesHierarchyInfo& boneHierarchy);

		void SetBoneOffsetMatrix(const String& name, const Matrix4& transform);
		void SetBoneOffsetMatrix(uint32 id, const Matrix4& transform);

		const Matrix4& GetBoneOffset(uint32 id) const { return m_BoneOffsetMatrices.at(id); }

		uint32 GetBoneID(const String& name) const;
		uint32 GetBoneCount() const { return m_BoneOffsetMatrices.size(); }

		const Bone& GetRootBone() const { return m_RootBone; }

	private:
		Bone m_RootBone;
		std::unordered_map<uint32, Matrix4> m_BoneOffsetMatrices;
	};


	struct TranslationKey
	{
		float TimeStamp;
		Vector3 Value;
	};

	struct RotationKey
	{
		float TimeStamp;
		Quaternion Value;
	};

	struct ScaleKey
	{
		float TimeStamp;
		Vector3 Value;
	};

	struct KeyFramesList
	{
		std::vector<TranslationKey> TranslationKeys;
		std::vector<RotationKey> RotationKeys;
		std::vector<ScaleKey> ScaleKeys;
	};

	struct AnimationCreateInfo
	{
		String Name;
		float Duration = 0;
		uint32 TicksPerSecond = 30;
		std::unordered_map<String, KeyFramesList> BoneNameToKeyFramesMap;
		Ref<Skeleton> Skeleton;
	};

	class ATHENA_API Animation
	{
	public:
		static Ref<Animation> Create(const AnimationCreateInfo& info);

		void GetBoneTransforms(float time, std::vector<Matrix4>& transforms);

		float GetDuration() const { return m_Duration; }
		uint32 GetTicksPerSecond() const { return m_TicksPerSecond; }

		const String& GetName() const { return m_Name; };
		const Ref<Skeleton> GetSkeleton() const { return m_Skeleton; }

	private:
		void ProcessBonesHierarchy(const Bone& bone, const Matrix4& parentTransform, float time, std::vector<Matrix4>& transforms);
		Matrix4 GetInterpolatedLocalTransform(uint32 boneID, float time);

		Vector3 GetInterpolatedTranslation(const std::vector<TranslationKey>& keys, float time);
		Quaternion GetInterpolatedRotation(const std::vector<RotationKey>& keys, float time);
		Vector3 GetInterpolatedScale(const std::vector<ScaleKey>& keys, float time);

	private:
		String m_Name;
		float m_Duration;
		uint32 m_TicksPerSecond;
		std::unordered_map<uint32, KeyFramesList> m_BoneIDToKeyFramesMap;
		Ref<Skeleton> m_Skeleton;
	};


	class ATHENA_API Animator
	{
	public:
		static Ref<Animator> Create(const std::vector<Ref<Animation>>& animations);

		const std::vector<Matrix4>& GetBoneTransforms() const { return m_BoneTransforms; }

		void OnUpdate(Time frameTime);
		bool IsPlaying() const { return m_CurrentAnimation != nullptr; }

		void StopAnimation();
		void PlayAnimation(const Ref<Animation>& animation);

		const std::vector<Ref<Animation>>& GetAllAnimations() const { return m_Animations; }
		const Ref<Animation>& GetCurrentAnimation() const { return m_CurrentAnimation; }

		float GetAnimationTime() const { return m_CurrentTime; }
		void SetAnimationTime(float time) { m_CurrentTime = time; }

	private:
		std::vector<Matrix4> m_BoneTransforms;
		std::vector<Ref<Animation>> m_Animations;
		Ref<Animation> m_CurrentAnimation;
		float m_CurrentTime;
	};
}
