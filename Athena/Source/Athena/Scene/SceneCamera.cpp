#include "SceneCamera.h"

#include "Athena/Math/Projections.h"


namespace Athena
{
	SceneCamera::SceneCamera(ProjectionType type)
		: m_ProjectionType(type)
	{
		RecalculateProjection();
	}

	void SceneCamera::SetViewportSize(uint32 width, uint32 height)
	{
		m_AspectRatio = (float)width / (float)height;
		RecalculateProjection();
	}

	CameraInfo SceneCamera::GetCameraInfo() const
	{
		CameraInfo info;
		info.ProjectionMatrix = m_ProjectionMatrix;

		if (m_ProjectionType == ProjectionType::Orthographic)
		{
			info.NearClip = m_OrthoData.NearClip;
			info.FarClip = m_OrthoData.FarClip;
			info.FOV = 0.f;
		}
		else if (m_ProjectionType == ProjectionType::Perspective)
		{
			info.NearClip = m_PerspectiveData.NearClip;
			info.FarClip = m_PerspectiveData.FarClip;
			info.FOV = m_PerspectiveData.VerticalFOV;
		}

		return info;
	}

	void SceneCamera::RecalculateProjection()
	{
		if (m_ProjectionType == ProjectionType::Orthographic)
		{
			float orthoLeft = -m_OrthoData.Size * m_AspectRatio * 0.5f;
			float orthoRight = m_OrthoData.Size * m_AspectRatio * 0.5f;
			float orthoBottom = -m_OrthoData.Size * 0.5f;
			float orthoTop = m_OrthoData.Size * 0.5f;

			m_ProjectionMatrix = Math::Ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthoData.NearClip, m_OrthoData.FarClip);
		}
		else if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_ProjectionMatrix = Math::PerspectiveReverseZ(m_PerspectiveData.VerticalFOV, m_AspectRatio,
				m_PerspectiveData.NearClip, m_PerspectiveData.FarClip);
		}
	}
}
