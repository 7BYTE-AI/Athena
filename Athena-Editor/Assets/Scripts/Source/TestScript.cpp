#include "TestScript.h"


namespace Sandbox
{
	TestScript::TestScript()
		: Script()
	{
		
	}

	void TestScript::OnCreate()
	{

	}

	void TestScript::OnUpdate(Time frameTime)
	{
		GetComponent<TransformComponent>().Translation.x += m_Speed * frameTime.AsSeconds();
		m_Speed += 0.001f;
	}

	void TestScript::GetFieldsDescription(ScriptFieldMap* outFields)
	{
		ADD_FIELD(m_Speed);
		//ADD_FIELD(m_Character);
	}

	EXPORT_SCRIPT(TestScript)
}
