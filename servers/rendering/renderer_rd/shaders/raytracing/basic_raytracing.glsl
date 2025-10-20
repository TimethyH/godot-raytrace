#[raygen]
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable 

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

layout(set = 0, binding = 3) buffer MaterialBuffer{
	MaterialData materials[];
}material;

layout(set = 0, binding = 4) uniform sampler2D albedo_texture[2];

#CODE : RAYTRACE

void main(){
	prd.hitValue = vec3(0.0f); //push.clear_color.xyz;

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

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(prd.hitValue, 1.0f));
	
	//imageStore(image, ivec2(gl_LaunchIDEXT.xy), material.color);
}


#[closest_hit]
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_descriptor_indexing : enable

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

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(set = 0, binding = 3) buffer MaterialBuffer{
	MaterialData materials[];
}material;

layout(set = 0, binding = 4) uniform sampler2D albedo_texture[2];


#CODE : RAYTRACE

void main() {
	uint mat_index = gl_InstanceCustomIndexEXT;
    MaterialData mat = material.materials[mat_index];
    
    vec3 albedo = mat.color.rgb;
    
    // Check if material has albedo texture
    if (mat.albedo_texture_index > 0) {
        vec2 uv = vec2(0.1,0.1);
        
        // Sample texture and multiply with base color
        vec3 tex_color = texture(albedo_texture[1], uv).rgb;
        albedo = tex_color;
    }
    
    prd.hitValue = albedo;
    //prd.hitValue = vec3(1.0f,0.0f,0.0f);
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
