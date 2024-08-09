#pragma once

#include <Athena.h>

using namespace Athena;


namespace Sandbox
{
	class FirstPersonCamera : public Script
	{
	public:
		FirstPersonCamera();

	private:
		virtual void OnCreate() override;
		virtual void OnUpdate(Time frameTime) override;
		virtual void GetFieldsDescription(ScriptFieldMap* outFields) override;

	private:
		float m_Speed = 1.f;
	};
}
