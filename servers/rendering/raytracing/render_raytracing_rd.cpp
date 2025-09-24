#include "render_raytracing_rd.h"

#include "servers/rendering/renderer_rd/shaders/raytracing/basic_raytrace.glsl.gen.h"

namespace RendererRD {
void RaytraceRD::_trace_rays() {
	RD::RaytracingListID rayID;
	rayID = RD ::get_singleton()->raytracing_list_begin();


	RenderingDevice *rd = RenderingServer::get_singleton()->create_local_rendering_device();
	//RID raytrace_pipeline = rd->render_pipeline_create(rasterize_shader, rd->framebuffer_get_format(framebuffers[0]), RD::INVALID_FORMAT_ID, RD::RENDER_PRIMITIVE_TRIANGLES, RD::PipelineRasterizationState(), RD::PipelineMultisampleState(), ds, RD::PipelineColorBlendState::create_disabled(3), 0);

	RID raytrace_pipeline = rd->raytracing_pipeline_create();

	Ref<RDShaderFile> raytrace_shader;
	raytrace_shader.instantiate();
	Error err = raytrace_shader->parse_versions_from_text(lm_raster_shader_glsl);
	if (err != OK) {
		raytrace_shader->print_errors("raster_shader");

		FREE_TEXTURES
		FREE_BUFFERS

		memdelete(rd);

		if (rcd != nullptr) {
			memdelete(rcd);
		}
	}

	RID raytracing_shader = rd->shader_create_from_spirv(raytrace_shader->get_spirv_stages());
	RD::get_singleton()->raytracing_list_bind_raytracing_pipeline(rayID, );

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
