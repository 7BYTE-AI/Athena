#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Camera.h"

#include "Athena/Math/Trigonometric.h"


namespace Athena
{
	class ATHENA_API SceneCamera : public Camera
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };

		struct OrthographicData
		{
			float Size = 10.f;
			float NearClip = -1.f;
			float FarClip = 1.f;
		};

		struct PerspectiveData
		{
			float VerticalFOV = Math::Radians(45.f);
			float NearClip = 0.1f;
			float FarClip = 1000.f;
		};

	public:
		SceneCamera(ProjectionType type = ProjectionType::Orthographic);

		void SetViewportSize(uint32 width, uint32 height);

		virtual CameraInfo GetCameraInfo() const override;

		float GetOrthographicSize() const { return m_OrthoData.Size; }
		void SetOrthographicSize(float size) { m_OrthoData.Size = size; RecalculateProjection(); }

		const OrthographicData& GetOrthographicData() const { return m_OrthoData; }
		const PerspectiveData& GetPerspectiveData() const { return m_PerspectiveData; }

		void SetOrthographicData(const OrthographicData& data) { m_OrthoData = data; RecalculateProjection(); }
		void SetPerspectiveData(const PerspectiveData& data) { m_PerspectiveData = data; RecalculateProjection(); }

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

		const Matrix4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		float GetAspectRatio() const { return m_AspectRatio; }

	private:
		void RecalculateProjection();

	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		OrthographicData m_OrthoData;
		PerspectiveData m_PerspectiveData;

		float m_AspectRatio = 1.f;
		Matrix4 m_ProjectionMatrix;
	};
}
