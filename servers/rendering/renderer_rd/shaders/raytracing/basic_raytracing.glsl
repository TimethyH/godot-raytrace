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
  float attenuation;
  float metallic;
  int  done;
  vec4 rayOrigin;
  vec4 rayDir;
};

struct MaterialData {
  vec4 color;
  uint albedo_texture_index;
  uint normal_texture_index;
  uint metallic_texture_index;
  uint roughness_texture_index;
  float metallicData;
  float roughnessData;
  float pad1;
  float pad2;
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
	mat4 inverseView;
};

layout(set = 0, binding = 2) uniform ubo_t{
	UBO data;
}ubo;

layout(set = 0, binding = 6) uniform texture2D normal;
layout(set = 0, binding = 7) uniform texture2D depth;
layout(set = 0, binding = 8) uniform texture2D specular;

layout(set = 0, binding = 9) uniform sampler point_sampler;

#CODE : RAYTRACE

vec3 reconstruct_position_from_depth(vec2 screen_uv, float raw_depth) {
    vec3 ndc = vec3(screen_uv * 2.0 - 1.0, raw_depth);
    vec4 world_pos = ubo.data.inverseViewProj * vec4(ndc, 1.0);
	world_pos /= world_pos.w;
    return world_pos.xyz;
}

void main(){
	ivec2 pixCoords = ivec2(gl_LaunchIDEXT.xy);

	// Initialize prd values
	prd.hitValue = imageLoad(image, pixCoords).xyz;
	prd.attenuation = 1.0f;

	const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	const vec2 in_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
	vec2 d = in_uv * 2.0 - 1.0;

	vec4 clip = vec4(d, 0.0, 1.0);
	vec4 worldDir = ubo.data.inverseViewProj * clip;
	worldDir /= worldDir.w;

	vec4 normal_roughness =  texelFetch(normal, pixCoords, 0); //texture(sampler2D(normal, point_sampler), pixCoords);
	vec3 encoded_normal = normal_roughness.xyz;
	float roughness = normal_roughness.w;

	vec3 decoded_normals = vec3(0.0f);
	decoded_normals = encoded_normal.xyz * 2.0f - 1.0f;
	decoded_normals = normalize(mat3(ubo.data.inverseView) * decoded_normals);

	prd.metallic = texelFetch(specular, pixCoords, 0).w;

	float depth_color = texelFetch(depth, pixCoords, 0).r;

	vec3 world_pos = reconstruct_position_from_depth(in_uv, depth_color);

	float t_min = 0.001;
	float t_max = 10000.0;

	int depth = 0;

	//vec3 view = normalize(ubo.data.cameraPos - world_pos);

	vec3 V = normalize(ubo.data.cameraPos - world_pos);
	vec3 R = reflect(-V, decoded_normals);

	vec3 accumulated_color = prd.hitValue;
	prd.attenuation = 1.0f * prd.metallic;

	prd.rayOrigin = vec4(world_pos + decoded_normals * 1e-4, 1.0);
	prd.rayDir = vec4(R, 0);

	if(prd.metallic > 0.001){
		// Iterative loop for the reflections
		while(depth < 4) // TODO remove hardcoded value with vulkan recursion limit
		{
			float previous_weight = 1.0f;

			traceRayEXT(tlas,
			gl_RayFlagsOpaqueEXT,
			0xFF,
			0,
			0,
			0,
			prd.rayOrigin.xyz,
			t_min,
			prd.rayDir.xyz,
			t_max,
			0
			);

			accumulated_color += prd.hitValue * prd.attenuation;
			prd.attenuation *= prd.metallic; // reduce attenuation depending on the metallic value of the material
			depth++;
		}
	}

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(accumulated_color, 1.0f));
	
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
  float attenuation;
  float metallic;
  int  done;
  vec4 rayOrigin;
  vec4 rayDir;
};

struct MaterialData {
  vec4 color;
  uint albedo_texture_index;
  uint normal_texture_index;
  uint metallic_texture_index;
  uint roughness_texture_index;
  float metallicData;
  float roughnessData;
  float pad1;
  float pad2;
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

layout(set = 0, binding = 4) uniform sampler2D albedo_texture[512]; // 512 is a hardcoded flag which tells the program we intend to use bindless descriptors

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

// Functions from Godot scene.glsl
#define M_PI 3.14159265358979323846

float SchlickFresnel(float u) {
    float m = 1.0 - u;
    float m2 = m * m;
    return m2 * m2 * m;
}

float D_GGX(float NdotH, float alpha) {
    float a = NdotH * alpha;
    float k = alpha / (1.0 - NdotH * NdotH + a * a);
    return k * k * (1.0 / M_PI);
}

float V_GGX(float NdotL, float NdotV, float alpha) {
    return 0.5 / mix(2.0 * NdotL * NdotV, NdotL + NdotV, alpha);
}

float DiffuseBurley(float NdotL, float NdotV, float LdotH, float roughness) {
    float FD90_minus_1 = 2.0 * LdotH * LdotH * roughness - 0.5;
    float FdV = 1.0 + FD90_minus_1 * SchlickFresnel(NdotV);
    float FdL = 1.0 + FD90_minus_1 * SchlickFresnel(NdotL);
    return (1.0 / M_PI) * FdV * FdL * NdotL;
}

void main() {

  // Barycentrics from hit attributes
  vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

  // get addresses
  uint mat_index = gl_InstanceCustomIndexEXT;
  uint64_t vertex_addr = addresses.address[mat_index * 3 + 0];
  uint64_t index_addr  = addresses.address[mat_index * 3 + 1];
  uint64_t uv_addr	   = addresses.address[mat_index * 3 + 2];

  MaterialData mat = material.materials[mat_index];
  vec3 albedo = mat.color.rgb;
  vec3 normal = vec3(0);
  float metallic = mat.metallicData;
  float roughness = mat.roughnessData;

  VertexData vtxData = VertexData(vertex_addr);
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

  vec3 p0 = vec3(loadFloat(vtxData, i0*12+0), loadFloat(vtxData, i0*12+4), loadFloat(vtxData, i0*12+8));
  vec3 p1 = vec3(loadFloat(vtxData, i1*12+0), loadFloat(vtxData, i1*12+4), loadFloat(vtxData, i1*12+8));
  vec3 p2 = vec3(loadFloat(vtxData, i2*12+0), loadFloat(vtxData, i2*12+4), loadFloat(vtxData, i2*12+8));

  normal = normalize(cross(p1 - p0, p2 - p0));
  if (dot(normal, gl_WorldRayDirectionEXT) > 0.0)
    normal = -normal;

  if (mat.albedo_texture_index > 0) { // Check if texture is valid
        vec3 tex_color = texture(albedo_texture[nonuniformEXT(mat.albedo_texture_index)], uv).rgb;
		albedo = tex_color;
  }

  if (mat.normal_texture_index > 0) { // Check if texture is valid
        vec3 tex_color = texture(albedo_texture[nonuniformEXT(mat.normal_texture_index)], uv).rgb;
		normal = tex_color;
  }

  if (mat.metallic_texture_index > 0) { // Check if texture is valid
        float tex_color = texture(albedo_texture[nonuniformEXT(mat.metallic_texture_index)], uv).b;
		metallic = tex_color;
  }

  if (mat.roughness_texture_index > 0) { // Check if texture is valid
        float tex_color = texture(albedo_texture[nonuniformEXT(mat.roughness_texture_index)], uv).g;
		roughness = tex_color;
  }

  prd.metallic = metallic;

  // Pbr equation
  vec3 N = normal;
  vec3 V = normalize(-gl_WorldRayDirectionEXT);
  vec3 R = reflect(gl_WorldRayDirectionEXT, normal);
  vec3 L = normalize(vec3(0.2,1.0,0.2) - hitPos);
  vec3 H = normalize(V + L);
  
  vec3 F0 = vec3(0.04f, 0.04f, 0.04f);
  F0 = mix(F0, albedo.rgb, metallic);

  float ndotl = max(dot(N, L), 0.0f);
  float ndotv = max(dot(N, V), 0.0f);
  float hdotv = max(dot(H, V), 0.0f);
  float ndoth = max(dot(N, H), 0.0f);
  float ldoth = max(dot(L, H), 0.0f);

  float NDF = D_GGX(ndoth, roughness);
  float G = V_GGX(ndotl, ndotv, roughness);
  vec3 F = F0 + (1.0 - F0) * SchlickFresnel(hdotv);

  vec3 numerator = NDF * G * F;
  float denominator = max(4.0f * ndotv * ndotl, 0.0001f);
  vec3 specular = max(numerator / denominator, vec3(0.0f, 0.0f, 0.0f));

  vec3 kS = F;
  vec3 kD = 1.0f - kS;
  kD *= (1.0f - metallic);

  float diffuse = DiffuseBurley(ndotl, ndotv, ldoth, roughness);

  vec3 Lo = kD * vec3(diffuse) + specular * ndotl;

  prd.rayOrigin = vec4(hitPos + normal * 0.001, 1.0f);
  prd.rayDir = vec4(R, 0.0f);
  prd.hitValue = Lo;

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
  float attenuation;
  float metallic;
  int  done;
  vec4 rayOrigin;
  vec4 rayDir;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

#CODE : RAYTRACE

void main() {
	prd.hitValue = vec3(0.0, 0.0, 0.0);
}
