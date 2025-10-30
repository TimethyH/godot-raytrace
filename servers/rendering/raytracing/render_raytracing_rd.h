#pragma once

#include "core/templates/rid_owner.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/pipeline_cache_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/shader_compiler.h"

#include "servers/rendering/renderer_rd/shaders/raytracing/basic_raytracing.glsl.gen.h"

namespace RendererRD {

// Forward declare RendererSceneRenderRD so we can pass it into some of our methods, these classes are pretty tightly bound

//class RenderDataRD;

class RaytraceRD {
public:
	//RaytraceRD();
	void init(const Projection &p_inv_view_proj, const Transform3D &p_cam_pos, RID p_render_buffer, RID p_render_buffer_normal, RID p_render_buffer_specular, RID p_tlas);
	void update_buffer(const Projection &p_inv_view_proj, const Projection &p_inv_view, const Transform3D &cam_pos);
	void setup_uniform_data(RID p_render_target, RID p_normal_render_target, RID p_depth_render_target, RID p_specular_render_target, RID p_tlas);

	void set_material_data(RID p_material, MaterialStorage* p_material_storage, uint32_t& index);
	void upload_material_data();
	void upload_addresses();

	void add_address(const uint64_t& address);

	~RaytraceRD();

	// RenderSceneDataRD &scene_data, const RenderDataRD *p_render_data
	void trace_rays(RID tlas, RID blas, RD::RaytracingListID LID, Size2i viewport_size);

	struct RaySceneState {
		struct UBO {
			float camera_pos[3];
			float align;
			float inv_view_proj[16]; // 64
			float inv_view[16]; // 64
			
			//float z_near; // 4 - 292
			//float z_far; // 4 - 296
		};

		UBO ubo;

		RID uniform_buffer;
		RID uniform_set;

		RID render_target;
	}ray_scene_state;

private:
	// 128 is the max size of a push constant.
	struct RayPushConstant {
		float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 16
	};

	// Ideally this struct holds material data which gets sent to the GPU..
	struct MaterialData {
		float albedo[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // 16
		uint32_t albedo_texture_index = 0;
		uint32_t normal_texture_index = 0;
		uint32_t metallic_texture_index = 0;
		uint32_t roughness_texture_index = 0; // 4x4 = 16
	};

	struct RaytracingShader {
		BasicRaytracingShaderRD shader;
		ShaderCompiler compiler;
		MaterialData material_data;

		RID default_shader;
		RID default_material;
		RID default_shader_rd;
		RID version;
	} raytracing_shader;


	RID raytrace_pipeline;

	LocalVector<MaterialData> materials;
	LocalVector<RID> textures;
	LocalVector<uint64_t> addresses;
	LocalVector<uint64_t> vertices;
	LocalVector<uint64_t> indices;
	HashMap<RID, uint32_t> material_to_index;
	HashMap<RID, uint32_t> texture_to_index;
	uint32_t texture_id = 1;
	RID material_buffer;
	RID address_buffer;

	bool default_texture_set = false;
};
} //namespace RendererRD
