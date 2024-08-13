#pragma once

#include <Athena.h>

using namespace Athena;


namespace Sandbox
{
	class FreeCamera: public Script
	{
	public:
		FreeCamera();

	private:
		virtual void OnCreate() override;
		virtual void OnUpdate(Time frameTime) override;
		virtual void GetFieldsDescription(ScriptFieldMap* outFields) override;

	private:
		Vector2 UpdateMousePos();

		Vector3 GetUpDirection() const;
		Vector3 GetRightDirection() const;
		Vector3 GetForwardDirection() const;
		Quaternion GetOrientation() const;

	private:
		float m_MoveSpeed = 5.f;
		float m_RotationSpeed = 2.f;

		float m_Yaw = 0.f;
		float m_Pitch = 0.f;
		float m_InitialRoll = 0.f;
		Vector2 m_InitialMousePos = Vector2(0.f);
	};
}
