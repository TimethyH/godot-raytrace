#include "servers/rendering/raytracing/render_raytracing_rd.h"

//#include "servers/rendering/storage/render_scene_data.h"

#include "basic_raytrace.glsl.gen.h"

#if defined(VULKAN_ENABLED)
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif

#include <memory>

namespace RendererRD {
void RaytraceRD::init() {

	/*RenderingDevice *rd = RenderingServer::get_singleton()->create_local_rendering_device();
	RenderingContextDriver *rcd = nullptr;

	Error err;

	if (rd == nullptr) {
#if defined(RD_ENABLED)
#if defined(METAL_ENABLED)
		rcd = memnew(RenderingContextDriverMetal);
		rd = memnew(RenderingDevice);
#endif
#if defined(VULKAN_ENABLED)
		if (rcd == nullptr) {
			rcd = memnew(RenderingContextDriverVulkan);
			rd = memnew(RenderingDevice);
		}
#endif
#endif
		if (rcd != nullptr && rd != nullptr) {
			err = rcd->initialize();
			if (err == OK) {
				err = rd->initialize(rcd);
			}

			if (err != OK) {
				memdelete(rd);
				memdelete(rcd);
				rd = nullptr;
				rcd = nullptr;
			}
		}
	}*/

	Vector<String> variants;
	variants.push_back(" ");
	raytracing_shader.shader.initialize(variants);

	raytracing_shader.version = raytracing_shader.shader.version_create(true);
	raytracing_shader.default_shader_rd = raytracing_shader.shader.version_get_shader(raytracing_shader.version, 0);

	update_buffer(Projection(), Transform3D());
	setup_uniform_data(RID(), RID());

	// Specify the shader stages to get the compiled spriv source code
	/*Vector<String> stage_names;
	stage_names.push_back("raygen");
	stage_names.push_back("closest_hit");
	stage_names.push_back("miss");

	Vector<RD::ShaderStageSPIRVData> shader_stage = raytracing_shader.shader.compile_stages(stage_names);*/

	raytrace_pipeline = RD::get_singleton()->raytracing_pipeline_create(raytracing_shader.default_shader_rd);

	// Set uniform bindings
	ray_scene_state.uniform_buffer = RD::get_singleton()->uniform_buffer_create(sizeof(RaySceneState::UBO));
}

void RaytraceRD::update_buffer(Projection view_proj, Transform3D cam_pos) {
	//ubo set cam
	ray_scene_state.ubo.camera_pos[0] = cam_pos.get_origin().x;
	ray_scene_state.ubo.camera_pos[1] = cam_pos.get_origin().y;
	ray_scene_state.ubo.camera_pos[2] = cam_pos.get_origin().z;
	RendererRD::MaterialStorage::store_camera(view_proj, ray_scene_state.ubo.view_proj);

	RD::get_singleton()->buffer_update(ray_scene_state.uniform_buffer, 0, sizeof(RaySceneState::UBO), &ray_scene_state.ubo);
}

void RaytraceRD::setup_uniform_data(RID render_target, RID tlas) {
	
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
		RID render_texture = render_target;  //RendererRD::TextureStorage::get_singleton()->render_target_get_rd_texture(render_target);
		u.append_id(render_texture);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 1;
		u.uniform_type = RD::UNIFORM_TYPE_ACCELERATION_STRUCTURE;
		u.append_id(tlas);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 2;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.append_id(ray_scene_state.uniform_buffer);
		uniforms.push_back(u);
	}

	ray_scene_state.uniform_set = RD::get_singleton()->uniform_set_create(uniforms, raytracing_shader.default_shader_rd, 0); // TODO remove magic number set 0
}

RaytraceRD::~RaytraceRD() {
	raytracing_shader.shader.version_free(raytracing_shader.version);
	RD::get_singleton()->free(ray_scene_state.uniform_buffer);
}

// RenderSceneDataRD & scene_data, const RenderDataRD *p_render_data
void RaytraceRD::trace_rays(RID tlas, RID blas, RD::RaytracingListID LID, Size2i viewport_size) {
	//RayPushConstant ray_push_constant;

	//memset(&ray_push_constant, 0, sizeof(RayPushConstant));

	RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();

	//RD::RaytracingListID LID = rd->raytracing_list_begin();

	// Update the acceleration structures preferably refit
	//rd->acceleration_structure_build(blas); // blas
	//rd->acceleration_structure_build(tlas); // tlas

	rd->raytracing_list_bind_raytracing_pipeline(LID, raytrace_pipeline); // bind list

	// Bind resources
	rd->raytracing_list_bind_uniform_set(LID, ray_scene_state.uniform_set, 0);
	//rd->raytracing_list_set_push_constant(LID, &ray_push_constant, sizeof(RayPushConstant));

	rd->raytracing_list_trace_rays(LID, (uint32_t)viewport_size.width, (uint32_t)viewport_size.height); // width height

	// Pipeline barier function here

	//rd->raytracing_list_end();
}
} //namespace RendererRD
