#include "servers/rendering/raytracing/render_raytracing_rd.h"

//#include "servers/rendering/storage/render_scene_data.h"

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
	update_buffer(p_inv_view_proj, p_inv_view_proj, p_cam_pos);

	setup_uniform_data(p_render_buffer, p_render_buffer, p_render_buffer, p_render_buffer, p_tlas);

	raytrace_pipeline = RD::get_singleton()->raytracing_pipeline_create(raytracing_shader.default_shader_rd);
}

void RaytraceRD::update_buffer(const Projection &p_inv_view_proj, const Projection &p_inv_view, const Transform3D &cam_pos) {
	//ubo set cam
	ray_scene_state.ubo.camera_pos[0] = cam_pos.get_origin().x;
	ray_scene_state.ubo.camera_pos[1] = cam_pos.get_origin().y;
	ray_scene_state.ubo.camera_pos[2] = cam_pos.get_origin().z;
	RendererRD::MaterialStorage::store_camera(p_inv_view_proj, ray_scene_state.ubo.inv_view_proj);
	RendererRD::MaterialStorage::store_camera(p_inv_view, ray_scene_state.ubo.inv_view);

	RD::get_singleton()->buffer_update(ray_scene_state.uniform_buffer, 0, sizeof(RaySceneState::UBO), &ray_scene_state.ubo);
}

void RaytraceRD::setup_uniform_data(RID p_render_target, RID p_normal_render_target, RID p_depth_render_target, RID p_specular_render_target, RID p_tlas) {
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

	//{
	//	RD::Uniform u;
	//	u.binding = 4;
	//	u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
	//	RID render_texture = p_specular_render_target;
	//	u.append_id(render_texture);
	//	uniforms.push_back(u);
	//}

	{
		RD::Uniform u;
		u.binding = 3;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.append_id(material_buffer);
		uniforms.push_back(u);
	}

	{
		MaterialStorage *material_storage = MaterialStorage::get_singleton();
		RID sampler = material_storage->sampler_rd_get_default(
				RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR,
				RS::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED);

		// albedo texture
		RD::Uniform u;
		u.binding = 4;
		u.uniform_type = RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
		for (int i = 0; i < static_cast<int>(textures.size()); i++) {
			if (!RD::get_singleton()->texture_is_valid(textures[i])) {
				print_error(vformat("Texture %d is invalid RD texture", i));
			}
			if (!textures[i].is_valid()) {
				print_error("Invalid texture!");
				u.append_id(sampler);
				u.append_id(textures[0]); // default white
			} else {
				u.append_id(sampler);
				u.append_id(textures[i]);
			}
		}
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 5;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.append_id(address_buffer);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 6;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		RID render_texture = p_normal_render_target;
		u.append_id(render_texture);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 7;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		RID render_texture = p_depth_render_target;
		u.append_id(render_texture);
		uniforms.push_back(u);
	}

	{
		RD::Uniform u;
		u.binding = 8;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		RID render_texture = p_specular_render_target;
		u.append_id(render_texture);
		uniforms.push_back(u);
	}

	const RendererRD::MaterialStorage::Samplers samplers = RendererRD::MaterialStorage::get_singleton()->samplers_rd_get_default();

	{
		RD::Uniform u;
		u.binding = 9;
		u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
		RID sampler = samplers.get_sampler(RS::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);

		u.append_id(sampler);
		uniforms.push_back(u);
	}

	ray_scene_state.uniform_set = RD::get_singleton()->uniform_set_create(uniforms, raytracing_shader.default_shader_rd, 0); // TODO remove magic number set 0
}

void RaytraceRD::set_material_data(RID p_material, MaterialStorage *p_material_storage, uint32_t &p_index) {
	if (!material_to_index.has(p_material)) {
		MaterialData mat_data;

		Color albedo = p_material_storage->material_get_param(p_material, "albedo");
		mat_data.albedo[0] = albedo.r;
		mat_data.albedo[1] = albedo.g;
		mat_data.albedo[2] = albedo.b;
		mat_data.albedo[3] = albedo.a;

		TextureStorage *texture_storage = TextureStorage::get_singleton();
		if (default_texture_set == false) {
			RID default_white = texture_storage->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
			textures.push_back(default_white); // p_index 0 is used for default.
			default_texture_set = true;
		}
		// Albedo
		RID texture = p_material_storage->material_get_param(p_material, "texture_albedo");
		if (texture.is_valid()) {
			RID rd_texture = texture_storage->texture_get_rd_texture(texture);

			if (rd_texture.is_valid()) {
				if (!texture_to_index.has(rd_texture)) {
					textures.push_back(rd_texture);
					texture_to_index[rd_texture] = texture_id++;
				}
				mat_data.albedo_texture_index = texture_to_index[rd_texture];
			} else {
				mat_data.albedo_texture_index = 0;
			}
		} else {
			mat_data.albedo_texture_index = 0;
		}

		RID normal_texture = p_material_storage->material_get_param(p_material, "texture_normal");
		if (normal_texture.is_valid()) {
			RID rd_texture = texture_storage->texture_get_rd_texture(normal_texture);

			if (rd_texture.is_valid()) {
				if (!texture_to_index.has(rd_texture)) {
					textures.push_back(rd_texture);
					texture_to_index[rd_texture] = texture_id++;
				}
				mat_data.normal_texture_index = texture_to_index[rd_texture];
			}
		}

		RID metallic_texture = p_material_storage->material_get_param(p_material, "texture_metallic");
		if (metallic_texture.is_valid()) {
			RID rd_texture = texture_storage->texture_get_rd_texture(metallic_texture);

			if (rd_texture.is_valid()) {
				if (!texture_to_index.has(rd_texture)) {
					textures.push_back(rd_texture);
					texture_to_index[rd_texture] = texture_id++;
				}
				mat_data.metallic_texture_index = texture_to_index[rd_texture];
			}
		}

		RID roughness_texture = p_material_storage->material_get_param(p_material, "texture_roughness");
		if (roughness_texture.is_valid()) {
			RID rd_texture = texture_storage->texture_get_rd_texture(roughness_texture);

			if (rd_texture.is_valid()) {
				if (!texture_to_index.has(rd_texture)) {
					textures.push_back(rd_texture);
					texture_to_index[rd_texture] = texture_id++;
				}
				mat_data.roughness_texture_index = texture_to_index[rd_texture];
			}
		}

		materials.push_back(mat_data);
		material_to_index[p_material] = p_index++;
	}
}

void RaytraceRD::upload_material_data() {
	if (materials.size() == 0) {
		materials.push_back(MaterialData());
	}

	material_buffer = RD::get_singleton()->storage_buffer_create(
			materials.size() * sizeof(MaterialData));

	RD::get_singleton()->buffer_update(material_buffer, 0, materials.size() * sizeof(MaterialData), materials.ptr());
}

void RaytraceRD::upload_addresses() {
	address_buffer = RD::get_singleton()->storage_buffer_create(
			addresses.size() * sizeof(uint64_t)); // vertices and indices together

	RD::get_singleton()->buffer_update(address_buffer, 0, addresses.size() * sizeof(uint64_t), addresses.ptr());
}

void RaytraceRD::add_address(const uint64_t &address) {
	addresses.push_back(address);
}

RaytraceRD::~RaytraceRD() {
	raytracing_shader.shader.version_free(raytracing_shader.version);
	RD::get_singleton()->free(ray_scene_state.uniform_buffer);
}

// RenderSceneDataRD & scene_data, const RenderDataRD *p_render_data
void RaytraceRD::trace_rays(RID tlas, RID blas, RD::RaytracingListID LID, Size2i viewport_size) {
	RayPushConstant ray_push_constant;
	ray_push_constant.clear_color[0] = { 1.0f };
	ray_push_constant.clear_color[1] = { 0.0f };
	ray_push_constant.clear_color[2] = { 1.0f };
	ray_push_constant.clear_color[3] = { 1.0f };

	Vector<uint8_t> push_constant_bytes;
	push_constant_bytes.resize(sizeof(RayPushConstant));
	memcpy(push_constant_bytes.ptrw(), &ray_push_constant, sizeof(RayPushConstant));

	//RD::get_singleton()->acceleration_structure_build(blas);
	//RD::get_singleton()->acceleration_structure_build(tlas);

	RD::get_singleton()->raytracing_list_bind_raytracing_pipeline(LID, raytrace_pipeline); // bind list

	 if(!ray_scene_state.uniform_set.is_valid()) {
		print_error(vformat("Uniform set is not valid"));
	}

	// Bind resources
	RD::get_singleton()->raytracing_list_bind_uniform_set(LID, ray_scene_state.uniform_set, 0);
	RD::get_singleton()->raytracing_list_set_push_constant(LID, push_constant_bytes.ptr(), sizeof(RayPushConstant));

	RD::get_singleton()->raytracing_list_trace_rays(LID, (uint32_t)viewport_size.width, (uint32_t)viewport_size.height); // width height
}
} //namespace RendererRD
