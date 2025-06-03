#pragma once

#include "camera.h"
#include "prefab.h"
#include "light.h"
#include "draw_command.h"
#include "light_command.h"
#include "shadow_command.h"
#include "deferred_command.h"
#include "reflection_probe.h"

//forward declarations
class Camera;
class Skeleton;

namespace GFX {

	class Shader;
	class Mesh;
	class FBO;
}

namespace SCN {

	class Prefab;
	class Material;
	class ReflectionProbeEntity;

	enum class RenderPipeline {
		FORWARD = 0,
		DEFERRED = 1,
		LIGHT_VOLUME = 2
	};

	enum class Lighting_Type {
		PHONG = 0,
		PBR = 1
	};

	enum class ForwardMode {
		SINGLE_PASS = 0,
		MULTI_PASS = 1
	};

	//this class is in charge of rendering anything in our system.
	//separating the render from anything else makes the code cleaner
	class Renderer
	{
	public:
		bool render_wireframe;
		bool render_boundaries;

		GFX::Texture* skybox_cubemap;
		SCN::Scene* scene; 
		GFX::Mesh sphere;

		//lab 1
		std::vector<SCN::PrefabEntity*> prefab_list; 
		std::vector<DrawCommand> draw_command_opaque_list; 
		std::vector<DrawCommand> draw_command_transparent_list; 

		//lab 2
		std::vector<SCN::LightEntity*> light_list;
		LightCommand light_command; 
		
		//lab 3
		std::vector<Camera*> camera_light_list; 
		ShadowCommand shadow_command; 

		//lab 4
		ForwardMode current_forward_mode;
		GbufferType current_gbuffer;
		RenderPipeline current_pipeline; 
		DeferredCommand deferred_command; 
		Lighting_Type lighting_type;

		//lab 5
		bool is_cubemap_reflections;
		GFX::FBO* lighting_fbo;
		bool update_ref;
		ReflectionProbeGrid* probe_grid;
		ReflectionProbeEntity* closest_probe;


		//updated every frames
		Renderer(const char* shaders_atlas_filename);
		~Renderer();

		//just to be sure we have everything ready for the rendering
		void setupScene();

		//parsers of the elements of the scene
		void parseNode(SCN::Node* node, Camera* camera);
		void parseReflectionProbes(Scene* scene);
		void parsePrefabs(std::vector<SCN::PrefabEntity*> prefab_list, Camera* camera);
		void parseSceneEntities(SCN::Scene* scene, Camera* camera);

		//renderers of the elements of the scene
		void renderSkybox(Camera* camera, GFX::Texture* cubemap);
		void renderShaderSinglePass(Camera* camera, DrawCommand draw_command, const char* shader_name) const;
		void renderShaderMultiPass(Camera* camera, DrawCommand draw_command, const char* shader_name) const;
		void renderFBO(Camera* camera, GFX::FBO* fbo, const char* shader_name);
		void renderShadow(Camera* light_camera, GFX::FBO* shadow_fbo) const;
		void renderForward();
		void renderDeferred();
		void renderScene(SCN::Scene* scene, Camera* camera); 
		

		//to show user interface
		void showUI();
	};
};