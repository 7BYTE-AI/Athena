#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	struct PipelineStatistics
	{
		uint64 InputAssemblyVertices;
		uint64 InputAssemblyPrimitives;
		uint64 VertexShaderInvocations;
		uint64 GeometryShaderInvocations;
		uint64 GeometryShaderPrimitives;
		uint64 ClippingInvocations;
		uint64 ClippingPrimitives;
		uint64 FragmentShaderInvocations;
		uint64 ComputeShaderInvocations;
	};

	struct GPUProfilerCreateInfo
	{
		String Name;
		Ref<RenderCommandBuffer> RenderCommandBuffer;
		uint32 MaxTimestampsCount = 0;
		uint32 MaxPipelineQueriesCount = 0;
	};


	// Returns query results with FramesInFlight delay
	class ATHENA_API GPUProfiler
	{
	public:
		static Ref<GPUProfiler> Create(const GPUProfilerCreateInfo& info);
		virtual ~GPUProfiler() = default;

		virtual void Reset() = 0;

		virtual void BeginTimeQuery() = 0;
		virtual void EndTimeQuery(Time* time) = 0;

		virtual void BeginPipelineStatsQuery() = 0;
		virtual const PipelineStatistics& EndPipelineStatsQuery() = 0;

	protected:
		GPUProfilerCreateInfo m_Info;
	};
}
