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
	void init(const Projection &p_inv_view_proj, const Transform3D &p_cam_pos, RID p_render_buffer, RID p_tlas);
	void update_buffer(const Projection& p_inv_view_proj, const Transform3D& cam_pos);
	void setup_uniform_data(RID render_targe, RID tlas);

	void update_material_data(RID buffer);

	~RaytraceRD();

	// RenderSceneDataRD &scene_data, const RenderDataRD *p_render_data
	void trace_rays(RID tlas, RID blas, RD::RaytracingListID LID, Size2i viewport_size);

	struct RaySceneState {
		struct UBO {
			float camera_pos[3];
			float align;
			float view_proj[16]; // 64
			
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
		RID uniform_buffer;
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
};
} //namespace RendererRD
