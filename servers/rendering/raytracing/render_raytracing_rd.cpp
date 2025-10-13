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
void RaytraceRD::init(const Projection &p_inv_view_proj, const Transform3D &p_cam_pos, RID p_render_buffer, RID p_render_buffer_normal, RID p_render_buffer_specular, RID p_tlas) {

	Vector<String> variants;
	variants.push_back(" ");
	raytracing_shader.shader.initialize(variants);

	raytracing_shader.version = raytracing_shader.shader.version_create(true);
	raytracing_shader.default_shader_rd = raytracing_shader.shader.version_get_shader(raytracing_shader.version, 0);

	ray_scene_state.uniform_buffer = RD::get_singleton()->uniform_buffer_create(sizeof(RaySceneState::UBO));
	update_buffer(p_inv_view_proj, p_cam_pos);

	setup_uniform_data(p_render_buffer, p_render_buffer, p_render_buffer, p_tlas);

	raytrace_pipeline = RD::get_singleton()->raytracing_pipeline_create(raytracing_shader.default_shader_rd);
}

void RaytraceRD::update_buffer(const Projection &p_inv_view_proj, const Transform3D &cam_pos) {
	//ubo set cam
	ray_scene_state.ubo.camera_pos[0] = cam_pos.get_origin().x;
	ray_scene_state.ubo.camera_pos[1] = cam_pos.get_origin().y;
	ray_scene_state.ubo.camera_pos[2] = cam_pos.get_origin().z;
	RendererRD::MaterialStorage::store_camera(p_inv_view_proj, ray_scene_state.ubo.view_proj);

	RD::get_singleton()->buffer_update(ray_scene_state.uniform_buffer, 0, sizeof(RaySceneState::UBO), &ray_scene_state.ubo);
}

void RaytraceRD::setup_uniform_data(RID p_render_target, RID p_normal_render_target, RID p_specular_render_target, RID p_tlas) {
	
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
		RID render_texture = p_render_target; //RendererRD::TextureStorage::get_singleton()->render_target_get_rd_texture(p_render_target);
		u.append_id(render_texture);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 1;
		u.uniform_type = RD::UNIFORM_TYPE_ACCELERATION_STRUCTURE;
		u.append_id(p_tlas);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 2;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.append_id(ray_scene_state.uniform_buffer);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 3;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		RID render_texture = p_normal_render_target;
		u.append_id(render_texture);
		uniforms.push_back(u);
	}

	//{
	//	RD::Uniform u;
	//	u.binding = 4;
	//	u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
	//	RID render_texture = p_specular_render_target;
	//	u.append_id(render_texture);
	//	uniforms.push_back(u);
	//}

	ray_scene_state.uniform_set = RD::get_singleton()->uniform_set_create(uniforms, raytracing_shader.default_shader_rd, 0); // TODO remove magic number set 0
}

RaytraceRD::~RaytraceRD() {
	raytracing_shader.shader.version_free(raytracing_shader.version);
	RD::get_singleton()->free(ray_scene_state.uniform_buffer);
}

// RenderSceneDataRD & scene_data, const RenderDataRD *p_render_data
void RaytraceRD::trace_rays(RID tlas, RID blas, RD::RaytracingListID LID, Size2i viewport_size) {
	RayPushConstant ray_push_constant;
	ray_push_constant.clear_color[0] = { 1.0f};
	ray_push_constant.clear_color[1] = { 0.0f};
	ray_push_constant.clear_color[2] = { 1.0f};
	ray_push_constant.clear_color[3] = { 1.0f};

	Vector<uint8_t> push_constant_bytes;
	push_constant_bytes.resize(sizeof(RayPushConstant));
	memcpy(push_constant_bytes.ptrw(), &ray_push_constant, sizeof(RayPushConstant));

	RD::get_singleton()->raytracing_list_bind_raytracing_pipeline(LID, raytrace_pipeline); // bind list

	// Bind resources
	RD::get_singleton()->raytracing_list_bind_uniform_set(LID, ray_scene_state.uniform_set, 0);
	RD::get_singleton()->raytracing_list_set_push_constant(LID, push_constant_bytes.ptr(), sizeof(RayPushConstant));

	RD::get_singleton()->raytracing_list_trace_rays(LID, (uint32_t)viewport_size.width, (uint32_t)viewport_size.height); // width height
}
} //namespace RendererRD
