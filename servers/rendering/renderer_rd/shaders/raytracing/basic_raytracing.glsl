#[raygen]
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

#CODE : RAYTRACE

void main(){
	const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	const vec2 in_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
	vec2 d = in_uv * 2.0 - 1.0;
	vec4 target = vec4(d.x, d.y, 1.0, 1.0);
	vec4 origin = vec4(ubo.data.cameraPos, 1.0);
	vec4 direction = vec4(normalize(target.xyz), 0);
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

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), push.clear_color);
	//imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(1.0f,0.0f,0.0f,1.0f));
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
	prd.hitValue = vec3(1.0, 0.0, 0.0);
}

#[closest_hit]
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
	prd.hitValue = vec3(0.0, 1.0, 0.0);
}

#[intersection]
#version 460
#extension GL_EXT_ray_tracing : enable

#VERSION_DEFINES

#GLOBALS

#CODE : RAYTRACE


void main(){
}

#[any_hit]
#version 460
#extension GL_EXT_ray_tracing : enable

#VERSION_DEFINES

#GLOBALS

#CODE : RAYTRACE

void main(){
}
