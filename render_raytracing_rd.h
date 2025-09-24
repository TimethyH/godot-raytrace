#pragma once

#include "core/templates/rid_owner.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/pipeline_cache_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/shader_compiler.h"

namespace RendererRD {

// Forward declare RendererSceneRenderRD so we can pass it into some of our methods, these classes are pretty tightly bound
class RendererSceneRenderRD;
class RenderSceneBuffersRD;

class RaytraceRD {
public:
	RaytraceRD();
	~RaytraceRD();

private:
};
}
