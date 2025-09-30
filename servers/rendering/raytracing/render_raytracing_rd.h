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
class RendererSceneRenderRD;
class RenderSceneBuffersRD;

class RaytraceRD {
public:
	//RaytraceRD();
	void init(RID render_target, RID tlas);

	//~RaytraceRD();

	void trace_rays(RenderSceneDataRD &scene_data, const RenderDataRD *p_render_data);

	struct RaySceneState {
		struct UBO {
			float combined_reprojection[RendererSceneRender::MAX_RENDER_VIEWS][16]; // 2 x 64 - 128
			float view_inv_projections[RendererSceneRender::MAX_RENDER_VIEWS][16]; // 2 x 64 - 256
			float view_eye_offsets[RendererSceneRender::MAX_RENDER_VIEWS][4]; // 2 x 16 - 288

			float z_near; // 4 - 292
			float z_far; // 4 - 296
		};

		UBO ubo;

		RID uniform_buffer;
		RID uniform_set;

		RID render_target;
	}ray_scene_state;

private:
	// 128 is the max size of a push constant.
	struct RayPushConstant {
		float clear_color[3] = { 1.0f, 0.0f, 0.0f }; // 12
	};

	//RayPushConstant ray_pc;

	struct RaytracingShader {
		BasicRaytracingShaderRD shader;
		ShaderCompiler compiler;

		RID default_shader;
		RID default_material;
		RID default_shader_rd;
	} raytracing_shader;
};
} //namespace RendererRD
