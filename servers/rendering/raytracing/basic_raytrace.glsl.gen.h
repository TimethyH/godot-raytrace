#pragma once

#include "servers/rendering/renderer_rd/shader_rd.h"

class BasicRaytraceRD : public ShaderRD {
public:
	BasicRaytraceRD() {
		static const char _raygen_code[] = {
			R"<!>(
#version 450

#VERSION_DEFINES

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Store in a shared shader file
struct hitPayload
{
  vec3 hitValue;
};

struct SceneData {
	mat4 projection_matrix;
	mat4 inv_projection_matrix;
	mat4 inv_view_matrix;
	mat4 view_matrix;

	
	mat4 projection_matrix_view[MAX_VIEWS];
	mat4 inv_projection_matrix_view[MAX_VIEWS];
	vec4 eye_offset[MAX_VIEWS];

	
	mat4 main_cam_inv_view_matrix;

	vec2 viewport_size;
	vec2 screen_pixel_size;
};

// Hit Data
layout(location = 0) rayPayloadEXT hitPayload prd;

layout(set = 1, binding = 0, std140) uniform SceneDataBlock {
	SceneData data;
}

layout(location = 0) out vec2 uv;

void main() {
	const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter/vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

	vec4 origin    = data.inv_view_matrix * vec4(0, 0, 0, 1);
	vec4 target    = data.inv_projection_matrix * vec4(d.x, d.y, 1, 1);
	vec4 direction = data.inv_view_matrix * vec4(normalize(target.xyz), 0);

	uint  rayFlags = gl_RayFlagsOpaqueEXT;
	float tMin     = 0.001;
	float tMax     = 10000.0;

	traceRayEXT(topLevelAS,   // acceleration structure ! Still need to send this via binding
              rayFlags,       // rayFlags
              0xFF,           // cullMask
              0,              // sbtRecordOffset
              0,              // sbtRecordStride
              0,              // missIndex
              origin.xyz,     // ray origin
              tMin,           // ray min range
              direction.xyz,  // ray direction
              tMax,           // ray max range
              0               // payload (location = 0)
  );

  // Store the trace data inside the image
  imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(prd.hitValue, 1.0));
}

)<!>"
		};
		static const char _closest_hit_code[] = {
			R"<!>(
#version 450

#VERSION_DEFINES

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

layout(location = 0) rayPayloadInEXT hitPayload prd;

// Yet to be implemented via code
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; };
layout(buffer_reference, scalar) buffer Materials {MaterialBuffer m[]; };
layout(buffer_reference, scalar) buffer MatIndices {int i[]; };

// Setup PC data in common glsl shader file so it is shared
layout(push_constant) uniform _PushConstantRay
{
  PushConstantRay pcRay;
};

void main() {
	// Object data
    ObjDesc    objResource = objDesc.i[gl_InstanceCustomIndexEXT];
    MatIndices matIndices  = MatIndices(objResource.materialIndexAddress);
    Materials  materials   = Materials(objResource.materialAddress);
    Indices    indices     = Indices(objResource.indexAddress);
    Vertices   vertices    = Vertices(objResource.vertexAddress);

	ivec3 ind = indices.i[gl_PrimitiveID];

	Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

	// Calculate the uvw of the the ray hit
	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

	const vec3 pos      = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));

	const vec3 nrm      = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
	const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));

	vec3  L;
   float lightIntensity = pcRay.lightIntensity;
   float lightDistance  = 100000.0;
   // Point light
   if(pcRay.lightType == 0)
   {
     vec3 lDir      = pcRay.lightPosition - worldPos;
     lightDistance  = length(lDir);
     lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
     L              = normalize(lDir);
   }
   else  // Directional light
   {
     L = normalize(pcRay.lightPosition);
   }
}
}
)<!>"
		};

		static const char _miss_code[] = {
			R"<!>(
#version 450

#VERSION_DEFINES

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(push_constant) uniform _PushConstantRay
{
  PushConstantRay pcRay;
};

void main()
{
  prd.hitValue = pcRay.clearColor.xyz * 0.8;
}

)<!>"
};

static const char *_anyhit_code = nullptr;
static const char *_intersection_code = nullptr;
		

		setup_raytracing(_raygen_code, _closest_hit_code, _miss_code, _anyhit_code, _intersection_code, "BasicRaytraceRD");
	}
};
