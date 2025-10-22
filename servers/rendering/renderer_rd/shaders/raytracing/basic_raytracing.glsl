#[raygen]
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable 
#extension GL_EXT_samplerless_texture_functions : enable

#VERSION_DEFINES

#GLOBALS

struct hitPayload
{
  vec3 hitValue;
  int  depth;
  vec3 attenuation;
  int  done;
  vec3 rayOrigin;
  vec3 rayDir;
};

struct MaterialData {
	vec4 color;
	uint albedo_texture_index;
	uint dummy;
	uint dummy2;
	uint dummy3;
};

layout(location = 0) rayPayloadEXT hitPayload prd;


// Render target
layout(set = 0, binding = 0, rgba32f) uniform image2D image;
// Acceleration structure
layout(set = 0, binding = 1) uniform accelerationStructureEXT tlas;

layout(push_constant) uniform PushConstants {
    vec4 clear_color;
} push;

struct UBO{
	vec3 cameraPos;
	mat4 inverseViewProj;
};

layout(set = 0, binding = 2) uniform ubo_t{
	UBO data;
}ubo;

layout(set = 0, binding = 6) uniform texture2D normal;

#CODE : RAYTRACE

void main(){
	prd.hitValue = vec3(0.0f);

	const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	const vec2 in_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
	vec2 d = in_uv * 2.0 - 1.0;

	vec4 clip = vec4(d, 0.0, 1.0);
	vec4 worldDir = ubo.data.inverseViewProj * clip;
	worldDir /= worldDir.w;

	//vec4 target = vec4(d.x, d.y, 1.0, 1.0);
	vec4 origin = vec4(ubo.data.cameraPos, 1.0);
	vec4 direction = vec4(normalize(worldDir.xyz - ubo.data.cameraPos), 0);
	float t_min = 0.001;
	float t_max = 10000.0;
	traceRayEXT(tlas,
		gl_RayFlagsOpaqueEXT,
		0xFF,
		0,
		0,
		0,
		origin.xyz,
		t_min,
		direction.xyz,
		t_max,
		0
	);

	ivec2 pixCoords = ivec2(gl_LaunchIDEXT.xy);
	vec4 normalcolor = texelFetch(normal, pixCoords, 0);

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(prd.hitValue.xyz + normalcolor.xyz, 1.0f));
	
	//imageStore(image, ivec2(gl_LaunchIDEXT.xy), material.color);
}

#[closest_hit]
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_descriptor_indexing : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#VERSION_DEFINES

#GLOBALS

struct hitPayload {
  vec3 hitValue;
  int  depth;
  vec3 attenuation;
  int  done;
  vec3 rayOrigin;
  vec3 rayDir;
};

struct MaterialData {
  vec4 color;
  uint albedo_texture_index;
  uint dummy;
  uint dummy2;
  uint dummy3;
};

// ===== Constants for current layout =====
const uint UV_STRIDE = 8u;   // bytes per vertex in the UV buffer
const uint UV_OFFSET = 0u;   // start at 0 (no interleave)

layout(location = 0) rayPayloadInEXT hitPayload prd;

// Treat vertex buffer as raw 32-bit words (4 bytes each)
layout(buffer_reference, scalar) readonly buffer VertexData {
  uint data[];
};

layout(buffer_reference, scalar) readonly buffer IndexData {
  uint16_t indices[];
};

layout(set = 0, binding = 3) buffer MaterialBuffer {
  MaterialData materials[];
} material;

layout(set = 0, binding = 4) uniform sampler2D albedo_texture[2];

layout(set = 0, binding = 5) readonly buffer AddressBuffer {
  uint64_t address[];
} addresses;

hitAttributeEXT vec2 attribs;

// read floats by byte address
float loadFloat(VertexData buf, uint64_t byteAddr) {
  uint idx = uint(byteAddr >> 2);
  return uintBitsToFloat(buf.data[idx]);
}

vec2 loadVec2(VertexData buf, uint64_t baseAddr) {
  return vec2(
    loadFloat(buf, baseAddr + 0),
    loadFloat(buf, baseAddr + 4)
  );
}

vec2 getVertexUV(VertexData vtx, uint vertexIndex) {
  uint64_t base = uint64_t(vertexIndex) * UV_STRIDE + UV_OFFSET;
  return loadVec2(vtx, base);
}

#CODE : RAYTRACE
void main() {

  // Barycentrics from hit attributes
  vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // get addresses
  uint mat_index = gl_InstanceCustomIndexEXT;
  uint64_t vertex_addr = addresses.address[mat_index * 3 + 0];
  uint64_t index_addr  = addresses.address[mat_index * 3 + 1];
  uint64_t uv_addr	   = addresses.address[mat_index * 3 + 2];

  MaterialData mat = material.materials[mat_index];
  vec3 albedo = mat.color.rgb;

  VertexData uvData   = VertexData(uv_addr);
  IndexData  indices  = IndexData(index_addr);

  // Triangle indices
  uint prim = gl_PrimitiveID;
  uint i0 = uint(indices.indices[prim * 3 + 0]);
  uint i1 = uint(indices.indices[prim * 3 + 1]);
  uint i2 = uint(indices.indices[prim * 3 + 2]);

  vec2 uv0 = getVertexUV(uvData, i0);
  vec2 uv1 = getVertexUV(uvData, i1);
  vec2 uv2 = getVertexUV(uvData, i2);

  vec2 uv = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;

  if (mat.albedo_texture_index > 0) { // Check if texture is valid
        vec3 tex_color = texture(albedo_texture[mat.albedo_texture_index], uv).rgb;
        albedo = tex_color; // Usually multiply with base color
  }

	prd.hitValue = albedo;

}

#[miss]
#version 460
#extension GL_EXT_ray_tracing : enable

#VERSION_DEFINES

#GLOBALS

struct hitPayload
{
  vec3 hitValue;
  int  depth;
  vec3 attenuation;
  int  done;
  vec3 rayOrigin;
  vec3 rayDir;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

#CODE : RAYTRACE

void main() {
	prd.hitValue = vec3(1.0, 0.0, 1.0);
}
