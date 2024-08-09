#include "FirstPersonCamera.h"


namespace Sandbox
{
	FirstPersonCamera::FirstPersonCamera()
		: Script()
	{
		
	}

	void FirstPersonCamera::OnCreate()
	{

	}

	void FirstPersonCamera::OnUpdate(Time frameTime)
	{
		GetComponent<TransformComponent>().Translation.x += m_Speed * frameTime.AsSeconds();
		m_Speed += 0.01f;
	}

	void FirstPersonCamera::GetFieldsDescription(ScriptFieldMap* outFields)
	{
		ADD_FIELD(m_Speed);
	}

	EXPORT_SCRIPT(FirstPersonCamera)
}
