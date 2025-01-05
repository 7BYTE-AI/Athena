#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	struct CameraInfo
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
		float NearClip;
		float FarClip;
		float FOV;
	};

	class ATHENA_API Camera 
	{
	public:
		Camera() = default;
		virtual ~Camera() = default;
		
		virtual CameraInfo GetCameraInfo() const = 0;
	};
}
