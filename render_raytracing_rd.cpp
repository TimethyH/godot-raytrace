#include "render_raytracing_rd.h"

namespace RendererRD {
void RaytraceRD::_trace_rays() {
	// TODO Begin render label

	// Bind pipeline
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

	// Bind Descriptors
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 0,
			(uint32_t)descSets.size(), descSets.data(), 0, nullptr);

	vkCmdPushConstants(cmdBuf, m_rtPipelineLayout,
			VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
			0, sizeof(PushConstantRay), &m_pcRay);

	vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, m_size.width, m_size.height, 1);

	// TODO End label
}
} //namespace RendererRD
