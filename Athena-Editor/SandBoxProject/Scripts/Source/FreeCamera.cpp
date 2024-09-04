#include "FreeCamera.h"


namespace Sandbox
{
	FreeCamera::FreeCamera()
		: Script()
	{
		
	}

	void FreeCamera::OnCreate()
	{
		Vector3 startRot = GetComponent<TransformComponent>().Rotation.AsEulerAngles();
		m_Pitch = -startRot.x;
		m_Yaw = -startRot.y;
		m_InitialRoll = startRot.z;
	}

	void FreeCamera::OnUpdate(Time frameTime)
	{
		Vector2 mouseDelta = UpdateMousePos() * m_RotationSpeed / 10.f * frameTime.AsSeconds();
		Vector3 moveDir = Vector3(0.0);

		if (Input::IsMouseButtonPressed(Mouse::Left) || Input::IsMouseButtonPressed(Mouse::Right))
		{
			float yawSign = Math::Sign(GetUpDirection().y);
			m_Yaw += yawSign * mouseDelta.x;
			m_Pitch += mouseDelta.y;

			if (Input::IsKeyPressed(Keyboard::W))
				moveDir += GetForwardDirection();
			else if (Input::IsKeyPressed(Keyboard::S))
				moveDir += -GetForwardDirection();

			if (Input::IsKeyPressed(Keyboard::D))
				moveDir += GetRightDirection();
			else if (Input::IsKeyPressed(Keyboard::A))
				moveDir += -GetRightDirection();

			if (Input::IsKeyPressed(Keyboard::E))
				moveDir += Vector3::Up();
			else if (Input::IsKeyPressed(Keyboard::Q))
				moveDir += Vector3::Down();

			moveDir.Normalize();

			TransformComponent& transform = GetComponent<TransformComponent>();
			transform.Translation += moveDir * m_MoveSpeed * 10.f * frameTime.AsSeconds();
			transform.Rotation = GetOrientation();
		}
	}

	Vector2 FreeCamera::UpdateMousePos()
	{
		Vector2 mousePos = Input::GetMousePosition();
		Vector2 delta = mousePos - m_InitialMousePos;
		m_InitialMousePos = mousePos;
		return delta;
	}

	Vector3 FreeCamera::GetUpDirection() const
	{
		return GetOrientation() * Vector3::Up();
	}

	Vector3 FreeCamera::GetRightDirection() const
	{
		return GetOrientation() * Vector3::Right();
	}

	Vector3 FreeCamera::GetForwardDirection() const
	{
		return GetOrientation() * Vector3::Forward();
	}

	Quaternion FreeCamera::GetOrientation() const
	{
		return Quaternion(Vector3(-m_Pitch, -m_Yaw, m_InitialRoll));
	}


	void FreeCamera::GetFieldsDescription(ScriptFieldMap* outFields)
	{
		ADD_FIELD(m_MoveSpeed);
		ADD_FIELD(m_RotationSpeed);
	}

	EXPORT_SCRIPT(FreeCamera)
}
