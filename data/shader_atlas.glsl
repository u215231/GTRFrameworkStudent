//example of some shaders compiled
compute test.cs
flat basic.vs flat.fs
texture basic.vs texture.fs
skybox basic.vs skybox.fs
depth quad.vs depth.fs
multi basic.vs multi.fs
plain basic.vs plain.fs
gbuffer_fill basic.vs gbuffer_fill.fs
deferred_light_pass quad.vs deferred_light_pass.fs
forward_light_single_pass basic.vs forward_light_single_pass.fs
forward_light_multi_pass basic.vs forward_light_multi_pass.fs

////////////////////////////////////////////////////////////////////////////////////////////////////

\my_functions

#define NO_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2
#define DIRECTIONAL_LIGHT 3
#define MAX_NUM_LIGHTS 10
#define MAX_NUM_SHADOWS 10

vec3 correct_light_position(vec3 light_position, vec3 world_position, int type);
float compute_phong(vec3 light_position, vec3 camera_position, vec3 world_position, vec3 normal, float shininess);
vec3 compute_intensified_color(vec3 light_color, float light_intensity);
float quadratic_attenuation(vec3 light_position);
float angular_attenuation(vec3 light_position, vec3 light_direction, vec2 light_cos_angle);
float compute_attenuation(vec3 light_position, vec3 light_direction, vec2 light_cos_angle, int type);
float compute_shadow(sampler2D shadow_map, mat4 shadow_viewprojection, float shadow_bias, vec3 world_position, int type); 

vec3 correct_light_position(vec3 light_position, vec3 world_position, int type)
{
	 return light_position - world_position * float(type != DIRECTIONAL_LIGHT);
}

float compute_phong(vec3 light_position, vec3 camera_position, vec3 world_position, vec3 normal, float shininess)
{
	vec3 L = normalize(light_position);
	vec3 V = normalize(camera_position - world_position);
	vec3 N = normalize(normal);
	vec3 R = normalize(reflect(-L, N));
	float diffuse_component = clamp(dot(N, L), 0.0, 1.0);
	float specular_component = pow(clamp(dot(R, V), 0.0, 1.0), shininess);
	return diffuse_component + specular_component;
}

vec3 compute_intensified_color(vec3 light_color, float light_intensity)
{
	return light_color * light_intensity;
}

float quadratic_attenuation(vec3 light_position)
{
	float distance = length(light_position);
	return 1.0 / max(pow(distance, 2), 0.00001);
}

float angular_attenuation(vec3 light_position, vec3 light_direction, vec2 light_cos_angle)
{
	vec3 L = normalize(light_position);
	vec3 D = normalize(light_direction); 

	float cos_min = light_cos_angle.x;
	float cos_max = light_cos_angle.y;

	float attenuation = clamp((dot(L, D) - cos_max) / max(cos_min - cos_max, 0.00001), 0.0, 1.0);
	attenuation *= float(dot(L, D) >= cos_max);
	attenuation *= quadratic_attenuation(light_position);
	return attenuation;
}

float compute_attenuation(vec3 light_position, vec3 light_direction, vec2 light_cos_angle, int type)
{
	if (type == POINT_LIGHT)	
		return quadratic_attenuation(light_position);
	if (type == SPOT_LIGHT)
		return angular_attenuation(light_position, light_direction, light_cos_angle);
	return 1.0;
}

float compute_shadow(sampler2D shadow_map, mat4 shadow_viewprojection, float shadow_bias, vec3 world_position, int type) 
{              
	if (type == POINT_LIGHT) 
		return 1.0;

    vec4 light_space_pos = shadow_viewprojection * vec4(world_position, 1.0);
    vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
    proj_coords = (proj_coords + 1) / 2;

	bool outside_shadow = proj_coords.x < 0.0 || proj_coords.x > 1.0 || 
							proj_coords.y < 0.0 || proj_coords.y > 1.0 ||
							proj_coords.z < 0.0 || proj_coords.z > 1.0;
    if (outside_shadow)
        return 0.0;

    float shadow_closest_depth = texture(shadow_map, proj_coords.xy).x;
    float current_depth = proj_coords.z;
    return shadow_closest_depth < (current_depth - shadow_bias) ? 0.0 : 1.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\PBR_functions

const float PI = 3.14159265359;

vec3 comp_F(vec3 F0, float H_dot_V){
	return F0 + (vec3(1.0) - F0)*pow((1.0 - H_dot_V),5.0);
}
float comp_D(float alpha_sq, float N_dot_H){
	return alpha_sq / (PI * pow(pow(N_dot_H,2.0) * (alpha_sq - 1.0) + 1.0, 2.0));
}

float comp_G(float N_dot_V, float alpha){
	return N_dot_V / (N_dot_V * (1.0 - alpha/2.0) + alpha/2.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\test.cs
#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() 
{
	vec4 i = vec4(0.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\basic.vs

#version 330 core

in vec3 a_vertex;
in vec3 a_normal;
in vec2 a_coord;
in vec4 a_color;

uniform vec3 u_camera_pos;

uniform mat4 u_model;
uniform mat4 u_viewprojection;

//this will store the color for the pixel shader
out vec3 v_position;
out vec3 v_world_position;
out vec3 v_normal;
out vec2 v_uv;
out vec4 v_color;

uniform float u_time;

void main()
{	
	//calcule the normal in camera space (the NormalMatrix is like ViewMatrix but without traslation)
	v_normal = (u_model * vec4( a_normal, 0.0) ).xyz;
	
	//calcule the vertex in object space
	v_position = a_vertex;
	v_world_position = (u_model * vec4( v_position, 1.0) ).xyz;
	
	//store the color in the varying var to use it from the pixel shader
	v_color = a_color;

	//store the texture coordinates
	v_uv = a_coord;

	//calcule the position of the vertex using the matrices
	gl_Position = u_viewprojection * vec4( v_world_position, 1.0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\instanced.vs

#version 330 core

in vec3 a_vertex;
in vec3 a_normal;
in vec2 a_coord;

in mat4 u_model;

uniform vec3 u_camera_pos;

uniform mat4 u_viewprojection;

//this will store the color for the pixel shader
out vec3 v_position;
out vec3 v_world_position;
out vec3 v_normal;
out vec2 v_uv;

void main()
{	
	//calcule the normal in camera space (the NormalMatrix is like ViewMatrix but without traslation)
	v_normal = (u_model * vec4( a_normal, 0.0) ).xyz;
	
	//calcule the vertex in object space
	v_position = a_vertex;
	v_world_position = (u_model * vec4( a_vertex, 1.0) ).xyz;
	
	//store the texture coordinates
	v_uv = a_coord;

	//calcule the position of the vertex using the matrices
	gl_Position = u_viewprojection * vec4( v_world_position, 1.0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\quad.vs

#version 330 core

in vec3 a_vertex;
in vec2 a_coord;
out vec2 v_uv;

void main()
{	
	v_uv = a_coord;
	gl_Position = vec4( a_vertex, 1.0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\flat.fs

#version 330 core

uniform vec4 u_color;

out vec4 FragColor;

void main()
{
	FragColor = u_color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\texture.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;
uniform float u_alpha_cutoff;

out vec4 FragColor;

void main()
{
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_texture, v_uv );

	if(color.a < u_alpha_cutoff)
		discard;

	FragColor = color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\skybox.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;

uniform samplerCube u_texture;
uniform vec3 u_camera_position;
out vec4 FragColor;

layout(location = 0) out vec4 gbuffer_albedo_map;
layout(location = 1) out vec4 gbuffer_normal_map;

void main()
{
	vec3 view_position = v_world_position - u_camera_position;
	vec4 color = texture( u_texture, view_position );
	FragColor = color;

	gbuffer_albedo_map = color;
	gbuffer_normal_map = vec4( vec3(0.0), 1.0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\multi.fs

#version 330 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;
uniform float u_alpha_cutoff;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 NormalColor;

void main()
{
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_texture, uv );

	if(color.a < u_alpha_cutoff)
		discard;

	vec3 N = normalize(v_normal);

	FragColor = color;
	NormalColor = vec4(N,1.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\depth.fs

#version 330 core

uniform vec2 u_camera_nearfar;
uniform sampler2D u_texture; //depth map
in vec2 v_uv;
out vec4 FragColor;

void main()
{
	float n = u_camera_nearfar.x;
	float f = u_camera_nearfar.y;
	float z = texture2D(u_texture,v_uv).x;
	if( n == 0.0 && f == 1.0 )
		FragColor = vec4(z);
	else
		FragColor = vec4( n * (z + 1.0) / (f + n - z * (f - n)) );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\plain.fs

#version 330 core

out vec4 FragColor;

void main()
{
	//Some alpha testing would be good here
	FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\gbuffer_fill.fs

#version 330 core

#include "my_functions"

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_albedo_texture;
uniform sampler2D u_metallic_roughness_texture;
uniform vec4 u_color;
//uniform sampler2D u_texture;

uniform mat4 u_model;
uniform mat4 u_viewprojection;
uniform vec3 u_camera_position;

layout(location = 0) out vec4 gbuffer_albedo_roughness;
layout(location = 1) out vec4 gbuffer_normal_metalness;
layout(location = 2) out vec4 gbuffer_position_map;
layout(location = 3) out vec4 gbuffer_shadow_map;

// Shading
uniform int u_num_shadows;
uniform sampler2D u_shadow_maps[MAX_NUM_SHADOWS];
uniform mat4 u_shadow_vps[MAX_NUM_SHADOWS];
uniform float u_shadow_biases[MAX_NUM_SHADOWS];

float merge_shadow_maps(int num_shadows, vec3 world_position) 
{                    
	float merged_shadow = 0.0;
	for (int i = 0; i < MAX_NUM_SHADOWS && i < num_shadows; i++)
	{
		float shadow = compute_shadow(u_shadow_maps[i], u_shadow_vps[i], u_shadow_biases[i], world_position, 2); 
		merged_shadow += shadow * pow(2, i);
	}
	merged_shadow /= pow(2, num_shadows) - 1.0;
	return merged_shadow;
}


void main()
{
	vec3 albedo_srgb = texture(u_albedo_texture, v_uv).rgb;
	vec3 albedo = pow(albedo_srgb, vec3(2.2)); // convert sRGB to linear

	vec3 orm = texture(u_metallic_roughness_texture, v_uv).rgb;
	float roughness = orm.g;
	float metalness = orm.b;

	vec3 normal = normalize(v_normal);
	vec3 encoded_normal = normal * 0.5 + 0.5;

	gbuffer_albedo_roughness = vec4(albedo, roughness);
	gbuffer_normal_metalness = vec4(encoded_normal, metalness);
	gbuffer_position_map     = vec4(v_world_position, 1.0);
	gbuffer_shadow_map       = vec4(vec3(merge_shadow_maps(u_num_shadows, v_world_position)), 1.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

\deferred_light_pass.fs

#version 330 core

#include "my_functions"
#include "PBR_functions"

in vec2 v_uv;

// G-buffers
uniform sampler2D u_gbuffer_color;
uniform sampler2D u_gbuffer_normal;
uniform sampler2D u_gbuffer_position;
uniform sampler2D u_gbuffer_shadow;
uniform sampler2D u_gbuffer_depth;

// View
uniform mat4 u_inv_vp_mat;
uniform vec3 u_camera_position;
uniform vec2 u_res_inv;

// Material
const float shininess = 20.0;


// Lighting
uniform int u_num_lights;
uniform vec3 u_light_ambient;
uniform vec3 u_light_positions[MAX_NUM_LIGHTS];
uniform vec3 u_light_colors[MAX_NUM_LIGHTS];
uniform vec3 u_light_directions[MAX_NUM_LIGHTS];
uniform float u_light_intensities[MAX_NUM_LIGHTS];
uniform int u_light_types[MAX_NUM_LIGHTS]; 
uniform vec2 u_light_cos_angles[MAX_NUM_LIGHTS];
uniform int u_lighting_type;

// Shading
uniform int u_num_shadows;

out vec4 FragColor;

float unmerge_shadow_map(int num_shadows, int shadow_num, float merged_shadow, int type)
{
	float shadow = 1.0;
	if (type == POINT_LIGHT)
		return shadow;
	merged_shadow *= pow(2, num_shadows) - 1.0;
	shadow = mod(int(merged_shadow) / int(pow(2, shadow_num)), 2);
	return shadow;
}

void main() 
{
    vec2 uv = gl_FragCoord.xy * u_res_inv;
	vec2 uv_clip = uv * 2.0 - 1.0;

	vec4 albedo_rough = texture(u_gbuffer_color, uv);
	vec3 albedo = albedo_rough.rgb;
	float roughness = albedo_rough.a;

	vec4 normal_metal = texture(u_gbuffer_normal, uv);
	float metalness = normal_metal.a;
	vec3 normal = normalize(normal_metal.rgb * 2.0 - 1.0);

	vec3 position = texture(u_gbuffer_position, uv).xyz;
	float shadow = texture(u_gbuffer_shadow, uv).r;
    float depth = texture(u_gbuffer_depth, uv).r;

	vec4 color = vec4(albedo, 1.0);

	if (normal == vec3(0.0))
	{
		FragColor = color;
		return;
	}

	float depth_clip = depth * 2.0 - 1.0;
	vec4 clip_coords = vec4( uv_clip.x, uv_clip.y, depth_clip, 1.0);
	vec4 not_norm_world_pos = u_inv_vp_mat * clip_coords;
	vec3 world_position = not_norm_world_pos.xyz / not_norm_world_pos.w;

	int shadow_num = 0;
	shadow *= pow(2, u_num_shadows) - 1.0;

	vec3 ambient_component = u_light_ambient;
	vec3 diffuse_component = vec3(0.0);
	vec3 specular_component = vec3(0.0);

	if (u_lighting_type == 0)
	{
		vec3 normal = texture(u_gbuffer_normal, uv).xyz;

		for (int i = 0; i < u_num_lights && i < MAX_NUM_LIGHTS; ++i) 
		{
			int type = u_light_types[i];
			vec3 light_position = u_light_positions[i];
			vec3 light_direction = u_light_directions[i];
			vec2 cos_angles = u_light_cos_angles[i];
			float attenuation = 1.0;
			float correct_shadow = 1.0;

			if (type == NO_LIGHT)
			{	
				continue;
			}

			if (type == POINT_LIGHT) 
			{ 
				light_position -= world_position;
				attenuation = quadratic_attenuation(light_position);
			}
		
			if (type == SPOT_LIGHT) 
			{ 
				light_position -= world_position;
				attenuation = angular_attenuation(light_position, light_direction, cos_angles);	

				correct_shadow = mod(int(shadow) / int(pow(2, shadow_num)), 2);
				shadow_num++;
			}
		
			if (type == DIRECTIONAL_LIGHT)
			{
				correct_shadow = mod(int(shadow) / int(pow(2, shadow_num)), 2);
				shadow_num++;
			}

			vec3 L = normalize(light_position);
			vec3 V = normalize(u_camera_position - world_position);
			vec3 N = normalize(normal * 2.0 - 1.0);
			vec3 R = normalize(reflect(-L, N));
		
			float L_dot_N = clamp(dot(N, L), 0.0, 1.0);
			float R_dot_V = clamp(dot(R, V), 0.0, 1.0);


			vec3 light_color = attenuation * u_light_colors[i] * u_light_intensities[i];
			diffuse_component += correct_shadow * light_color * L_dot_N;
			specular_component += correct_shadow * light_color * pow(R_dot_V, shininess);
		}
		
		color.xyz *= ambient_component + diffuse_component + specular_component;
		FragColor = color;
	}
	else if (u_lighting_type == 1)
	{
		vec3 directLighting = vec3(0.0);

		vec3 F0 = mix(vec3(0.04), albedo, metalness);

		for (int i = 0; i < u_num_lights; i++) 
		{
			int type = u_light_types[i];
			vec3 light_position = u_light_positions[i];
			vec3 light_direction = u_light_directions[i];
			vec2 cos_angles = u_light_cos_angles[i];
			float attenuation = 1.0;
			float correct_shadow = 1.0;

			if (type == NO_LIGHT)
				continue;

			if (type == POINT_LIGHT)
			{	
				light_position -= world_position;
				attenuation = quadratic_attenuation(light_position);
			} 

			if (type == SPOT_LIGHT)
			{  
				light_position -= world_position;
				attenuation = angular_attenuation(light_position, light_direction, cos_angles);	

				correct_shadow = mod(int(shadow) / int(pow(2, shadow_num)), 2);
				shadow_num++; 
			}

			if (type == DIRECTIONAL_LIGHT)
			{	 
				correct_shadow = mod(int(shadow) / int(pow(2, shadow_num)), 2);
				shadow_num++;
			}

			vec3 L = normalize(light_position);
			vec3 V = normalize(u_camera_position - world_position);
			vec3 N = normal;
			vec3 R = normalize(reflect(-L, N));
			vec3 H = normalize(V+L);

			float rp = clamp(roughness, 0.0, 1.0);
			float alpha = rp*rp;
			float alpha_sq = pow(alpha,2.0);

			float N_dot_L = clamp(dot(N, L), 0.0, 1.0);
			float N_dot_V = clamp(dot(N,V), 0.0, 1.0);
			float H_dot_V = clamp(dot(H,V), 0.0, 1.0);
			float N_dot_H = clamp(dot(N,H), 0.0, 1.0);

			vec3 fresnel  = comp_F(F0, H_dot_V);
			float distribution  = comp_D(alpha_sq, N_dot_H);
			float geometry = comp_G(N_dot_V, alpha);

			vec3 light_color =  attenuation * u_light_colors[i];
			diffuse_component = correct_shadow * light_color * N_dot_L;
			float denom = max((4.0 * N_dot_L * N_dot_V), 0.0001);
			specular_component = correct_shadow*((fresnel * distribution * geometry) / denom);

			directLighting += (diffuse_component + specular_component) * u_light_intensities[i] * N_dot_L;
		}

		color.xyz *=  directLighting;    //diffuse_component + specular_component;
		FragColor = color;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////

\forward_light_single_pass.fs

#version 330 core

#include "PBR_functions"
#include "my_functions"

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec4 u_color;

uniform sampler2D u_albedo_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_metallic_roughness_texture;

uniform float u_time;
uniform float u_alpha_cutoff;

// View
uniform vec3 u_camera_position;				

// Material
uniform float u_shininess;			

// Lighting
uniform int u_num_lights;
uniform vec3 u_light_ambient;
uniform vec3 u_light_positions[MAX_NUM_LIGHTS];
uniform vec3 u_light_colors[MAX_NUM_LIGHTS];
uniform vec3 u_light_directions[MAX_NUM_LIGHTS];
uniform float u_light_intensities[MAX_NUM_LIGHTS];
uniform int u_light_types[MAX_NUM_LIGHTS]; 
uniform vec2 u_light_cos_angles[MAX_NUM_LIGHTS];
uniform int u_lighting_type;

// Shading
uniform int u_num_shadows;
uniform sampler2D u_shadow_maps[MAX_NUM_SHADOWS];
uniform mat4 u_shadow_vps[MAX_NUM_SHADOWS];
uniform float u_shadow_biases[MAX_NUM_SHADOWS];

out vec4 FragColor;

void main()
{
	vec3 ao_rough_metal = texture(u_metallic_roughness_texture, v_uv).rgb;
	//float ao = ao_rough_metal.r;
	float roughness = ao_rough_metal.g;
	float metalness = ao_rough_metal.b;
	vec3 albedo_srgb = texture(u_albedo_texture, v_uv).rgb;
	vec3 albedo = pow(albedo_srgb, vec3(2.2)); //convert sRGB to linear

	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_albedo_texture, v_uv );


	vec3 diffuse_component = vec3(0.0);
	vec3 specular_component = vec3(0.0);
	int shadow_num = 0;

	if (u_lighting_type == 0)
	{
		vec3 ambient_component = u_light_ambient;

		int shadow_num = 0; 

		for (int i = 0; i < u_num_lights; i++) 
		{
			int type = u_light_types[i];
			vec3 light_position = u_light_positions[i];
			vec3 light_direction = u_light_directions[i];
			vec2 cos_angles = u_light_cos_angles[i];
			float attenuation = 1.0;
			float shadow = 1.0;

			if (type == NO_LIGHT)
				continue;

			if (type == POINT_LIGHT)
			{	
				light_position -= v_world_position;
				attenuation = quadratic_attenuation(light_position);
			} 

			if (type == SPOT_LIGHT)
			{  
				light_position -= v_world_position;
				attenuation = angular_attenuation(light_position, light_direction, cos_angles);	
				shadow = compute_shadow(u_shadow_maps[shadow_num], u_shadow_vps[shadow_num], u_shadow_biases[shadow_num], v_world_position, type);
				shadow_num++;
			}

			if (type == DIRECTIONAL_LIGHT)
			{	 
				shadow = compute_shadow(u_shadow_maps[shadow_num], u_shadow_vps[shadow_num], u_shadow_biases[shadow_num], v_world_position, type);
				shadow_num++;
			}

			vec3 L = normalize(light_position);
			vec3 V = normalize(u_camera_position - v_world_position);
			vec3 N = normalize(v_normal);
			vec3 R = normalize(reflect(-L, N));
		
			float L_dot_N = clamp(dot(N, L), 0.0, 1.0);
			float R_dot_V = clamp(dot(R, V), 0.0, 1.0);

			vec3 light_color =  attenuation * u_light_colors[i] * u_light_intensities[i];

			diffuse_component += shadow * light_color * L_dot_N;
			specular_component += shadow * light_color * pow(R_dot_V, u_shininess);
		}

		if(color.a < u_alpha_cutoff)
			discard;

		color.xyz *= ambient_component + diffuse_component + specular_component;
		FragColor = color;
	}
	else if (u_lighting_type == 1)
	{
		if(color.a < u_alpha_cutoff)
			discard;

		vec3 directLighting = vec3(0.0);

		vec3 F0 = mix(vec3(0.04), albedo, metalness);

		for (int i = 0; i < u_num_lights; i++) 
		{
			int type = u_light_types[i];
			vec3 light_position = u_light_positions[i];
			vec3 light_direction = u_light_directions[i];
			vec2 cos_angles = u_light_cos_angles[i];
			float attenuation = 1.0;
			float shadow = 1.0;

			if (type == NO_LIGHT)
				continue;

			if (type == POINT_LIGHT)
			{	
				light_position -= v_world_position;
				attenuation = quadratic_attenuation(light_position);
			} 

			if (type == SPOT_LIGHT)
			{  
				light_position -= v_world_position;
				attenuation = angular_attenuation(light_position, light_direction, cos_angles);	
				shadow = compute_shadow(u_shadow_maps[shadow_num], u_shadow_vps[shadow_num], u_shadow_biases[shadow_num], v_world_position, type);
				shadow_num++;
			}

			if (type == DIRECTIONAL_LIGHT)
			{	 
				shadow = compute_shadow(u_shadow_maps[shadow_num], u_shadow_vps[shadow_num], u_shadow_biases[shadow_num], v_world_position, type);
				shadow_num++;
			}

			vec3 L = normalize(light_position);
			vec3 V = normalize(u_camera_position - v_world_position);
			vec3 N = normalize(v_normal);
			vec3 R = normalize(reflect(-L, N));
			vec3 H = normalize(V+L);

			float rp = clamp(roughness, 0.0, 1.0);
			float alpha = rp*rp;
			float alpha_sq = pow(alpha,2.0);

			float N_dot_L = clamp(dot(N, L), 0.0, 1.0);
			float N_dot_V = clamp(dot(N,V), 0.0, 1.0);
			float H_dot_V = clamp(dot(H,V), 0.0, 1.0);
			float N_dot_H = clamp(dot(N,H), 0.0, 1.0);

			vec3 fresnel  = comp_F(F0, H_dot_V);
			float distribution  = comp_D(alpha_sq, N_dot_H);
			float geometry = comp_G(N_dot_V, alpha);

			vec3 light_color =  attenuation * u_light_colors[i];
			diffuse_component = shadow * light_color * N_dot_L;
			float denom = max((4.0 * N_dot_L * N_dot_V), 0.0001);
			specular_component = shadow*((fresnel * distribution * geometry) / denom);

			directLighting += (diffuse_component + specular_component) * u_light_intensities[i] * N_dot_L;
		}

		color.xyz *=  directLighting;//diffuse_component + specular_component;
		FragColor = color;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////

\forward_light_multi_pass.fs

#version 330 core

#include "my_functions"

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_time;

// View
uniform vec3 u_camera_position;				

// Material
uniform float u_shininess;			

// Lighting
uniform vec3 u_light_ambient;
uniform vec3 u_light_position;
uniform vec3 u_light_color;
uniform vec3 u_light_direction;
uniform float u_light_intensity;
uniform int u_light_type; 
uniform vec2 u_light_cos_angle;

out vec4 FragColor;

void main()
{
	vec2 uv = v_uv;
	vec4 color = u_color;
	color *= texture( u_texture, v_uv );
	
	vec3 light_position = correct_light_position(u_light_position, v_world_position, u_light_type);

	vec3 accumulation = vec3(1.0);

	accumulation *= compute_phong(light_position, u_camera_position, v_world_position, v_normal, u_shininess);
	accumulation *= compute_attenuation(light_position, u_light_direction, u_light_cos_angle, u_light_type);
	accumulation *= compute_intensified_color(u_light_color, u_light_intensity);
	// accumulation *= compute_shadow(u_shadow_map, u_shadow_vp, u_shadow_bias, v_world_position, u_light_type);

	color.xyz *= accumulation;
	FragColor = color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////