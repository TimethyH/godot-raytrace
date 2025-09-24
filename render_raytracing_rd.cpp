#include "servers/rendering/raytracing/render_raytracing_rd.h"


#if defined(VULKAN_ENABLED)
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif

#include <memory>

namespace RendererRD {
void RaytraceRD::init() {
	RendererRD::TextureStorage *texture_storage = RendererRD::TextureStorage::get_singleton();
	RendererRD::MaterialStorage *material_storage = RendererRD::MaterialStorage::get_singleton();

	//// Maybe set data request_function if needed

	{
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["COLOR"] = "color";
		actions.renames["ALPHA"] = "alpha";
		actions.renames["EYEDIR"] = "cube_normal";
		actions.renames["POSITION"] = "params.position";
		actions.renames["SKY_COORDS"] = "panorama_coords";
		actions.renames["SCREEN_UV"] = "uv";
		actions.renames["FRAGCOORD"] = "gl_FragCoord";
		actions.renames["TIME"] = "params.time";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);
		actions.base_texture_binding_index = 1;
		actions.texture_layout_set = 1;
		actions.base_uniform_string = "material.";
		actions.base_varying_index = 10;

		actions.default_filter = ShaderLanguage::FILTER_LINEAR_MIPMAP;
		actions.default_repeat = ShaderLanguage::REPEAT_ENABLE;
		actions.global_buffer_array_variable = "global_shader_uniforms.data";

		raytracing_shader.compiler.initialize(actions);
	}

	{
		// default material and shader for raytracing shader
		raytracing_shader.default_shader = material_storage->shader_allocate();
		material_storage->shader_initialize(raytracing_shader.default_shader);

		raytracing_shader.default_material = material_storage->material_allocate();
		material_storage->material_initialize(raytracing_shader.default_material);

		raytracing_shader.default_material = material_storage->material_allocate();
		material_storage->material_set_shader(raytracing_shader.default_material, raytracing_shader.default_shader);
	}

	RenderingDevice *rd = RenderingServer::get_singleton()->create_local_rendering_device();
	RenderingContextDriver *rcd = nullptr;
	//default_shader_rd = shader.version_get_shader(md->shader_data->version, SKY_VERSION_BACKGROUND);

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



	// Build RDShaderSource and set stage source
	//Ref<RDShaderSource> src = Ref<RDShaderSource>(memnew(RDShaderSource));
	//// choose the language constant (GLSL)
	//src->set_language(RenderingDevice::SHADER_LANGUAGE_GLSL);
	//src->set_stage_source(RenderingDevice::SHADER_STAGE_CLOSEST_HIT, BasicRaytraceRD::_raygen_code);
	//src->set_stage_source(RenderingDevice::SHADER_STAGE_FRAGMENT, fragment_glsl);

	//// Compile to SPIR-V (returns RDShaderSPIRV resource with possible errors)
	//Ref<RDShaderSPIRV> spirv = rd->shader_compile_spirv_from_source(src, /*allow_cache=*/true);
	//if (!spirv.is_valid() || !spirv->get_compile_error_fragment().is_empty() || !spirv->get_compile_error_vertex().is_empty()) {
	//	ERR_PRINT("SPIR-V compile error: " + spirv->get_compile_error_vertex() + "\n" + spirv->get_compile_error_fragment());
	//	return;
	//}

	//// Create the device-side shader (RID)
	//RID shader_rid = rd->shader_create_from_spirv(spirv);
	//if (!shader_rid.is_valid()) {
	//	ERR_PRINT("Failed to create RD shader from SPIR-V.");
	//	return;
	//}

	//p

	//RID raytrace_pipeline = rd->raytracing_pipeline_create(raytracing_shader);

	// Set uniform bindings
}

void RaytraceRD::_trace_rays() {
	
	// TODO Begin render label

	RenderingDevice *rd = RenderingServer::get_singleton()->create_local_rendering_device();

	rd->raytracing_list_begin();

	rd->acceleration_structure_build(); // blas
	rd->acceleration_structure_build(); // tlas

	rd->raytracing_list_bind_raytracing_pipeline(); // bind list

	// Bind resources
	rd->raytracing_list_bind_uniform_set();
	rd->raytracing_list_set_push_constant();

	rd->raytracing_list_trace_rays(); // width height

	// Pipeline barier function here

	rd->raytracing_list_end();

	// TODO End label
}
} //namespace RendererRD
