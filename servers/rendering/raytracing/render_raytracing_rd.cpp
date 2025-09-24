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

	RenderingDevice *rd = RenderingServer::get_singleton()->create_local_rendering_device();
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
	}

	Vector<String> variants;
	variants.push_back("");
	raytracing_shader.shader.initialize(variants);

	RID version = raytracing_shader.shader.version_create(true);
	RID shader = raytracing_shader.shader.version_get_shader(version, 0);

	RID raytrace_pipeline = rd->raytracing_pipeline_create(shader);

	// Set uniform bindings
	ray_scene_state.uniform_buffer = RD::get_singleton()->uniform_buffer_create(sizeof(RaySceneState::UBO));

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
		u.append_id(ray_scene_state.uniform_buffer);
		uniforms.push_back(u);
	}

	ray_scene_state.uniform_set = RD::get_singleton()->uniform_set_create(uniforms, shader, 0); // TODO remove magic number set 0
}

void RaytraceRD::trace_rays(RenderSceneDataRD &scene_data) {
	
	// TODO Begin render label

	//RenderingDevice *rd = RenderingServer::get_singleton()->create_local_rendering_device();

	//rd->raytracing_list_begin();

	//rd->acceleration_structure_build(); // blas
	//rd->acceleration_structure_build(); // tlas

	//rd->raytracing_list_bind_raytracing_pipeline(); // bind list

	//// Bind resources
	//rd->raytracing_list_bind_uniform_set();
	//rd->raytracing_list_set_push_constant();

	//rd->raytracing_list_trace_rays(); // width height

	//// Pipeline barier function here

	//rd->raytracing_list_end();

	// TODO End label
}
} //namespace RendererRD
