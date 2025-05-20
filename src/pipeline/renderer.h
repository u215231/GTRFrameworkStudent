#pragma once

#include "camera.h"
#include "prefab.h"
#include "light.h"
#include "draw_command.h"
#include "light_command.h"
#include "shadow_command.h"
#include "deferred_command.h"

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

	enum class RenderPipeline {
		FORWARD = 0,
		DEFERRED = 1,
		LIGHT_VOLUME = 2
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

		std::vector<SCN::PrefabEntity*> prefab_list; //lab1
		std::vector<DrawCommand> draw_command_opaque_list; //lab1
		std::vector<DrawCommand> draw_command_transparent_list; //lab1

		std::vector<SCN::LightEntity*> light_list; //lab2
		LightCommand light_command; // lab2
		ForwardMode current_forward_mode;

		std::vector<GFX::FBO*> shadow_FBOs; //lab3
		std::vector<Camera*> camera_light_list; //lab3
		ShadowCommand shadow_command; // lab3

		RenderPipeline current_pipeline; //lab4
		DeferredCommand deferred_command; //lab4
		DeferredCommand lighting;
		std::vector<GFX::Mesh> spheres; //lab4

		//updated every frames
		Renderer(const char* shaders_atlas_filename);
		~Renderer();

		//just to be sure we have everything ready for the rendering
		void setupScene();

		//parsers of the elements of the scene
		void parseNode(SCN::Node* node, Camera* cam);
		void parsePrefabs(std::vector<SCN::PrefabEntity*> prefab_list, Camera* camera);
		void parseCameraLights(std::vector<SCN::LightEntity*> light_list);
		void parseLightVolumes(std::vector<SCN::LightEntity*> light_list);
		void parseSceneEntities(SCN::Scene* scene, Camera* camera);

		//renderers of the elements of the scene
		void renderSkybox(GFX::Texture* cubemap);
		void renderMeshWithMaterial(DrawCommand draw_command) const;
		void renderShadow(Camera* light_camera, GFX::FBO* shadow_fbo) const;
		void renderShaderMultiPass(Camera* camera, DrawCommand draw_command) const;
		void renderShader(Camera* camera, DrawCommand draw_command, const char* shader_name) const;
		void renderForward();
		void renderDeferred();
		void renderDeferredLightingPass() const;
		void renderLightVolumes();
		void renderScene(SCN::Scene* scene, Camera* camera);

		//to show user interface
		void showUI();
	};
};