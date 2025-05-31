#include "reflection_probe.h"
#include "../gfx/fbo.h"
#include "../gfx/texture.h"
#include "../gfx/mesh.h"
#include "../gfx/shader.h"
#include "../pipeline/camera.h"
#include "../pipeline/renderer.h"
#include "../pipeline/scene.h"
#include "../utils/utils.h"

using namespace SCN;

ReflectionProbeEntity::ReflectionProbeEntity() 
{
    const int w = 512;
    const int h = 512;

    range = 10.0f;
    reflection_strength = 0.5f; 
    cubemap = new GFX::Texture();
    cubemap->createCubemap(w, h, NULL, GL_RGB, GL_UNSIGNED_INT, true);	

    for (int face = 0; face < NUM_FACES; face++) {
        capture_FBOs[face].create(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, true);
    }
}

ReflectionProbeEntity::~ReflectionProbeEntity() 
{
    if (cubemap) {
        delete cubemap;
        cubemap = nullptr;
    }
}

void ReflectionProbeEntity::configure(cJSON* json)
{
    range = readJSONNumber(json, "range", range);
    reflection_strength = readJSONNumber(json, "reflection_strength", reflection_strength);
}

void ReflectionProbeEntity::serialize(cJSON* json)
{
    writeJSONNumber(json, "range", range);
    writeJSONNumber(json, "reflection_strength", reflection_strength);
}

void ReflectionProbeEntity::captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer)
{
    const vec3 directions[NUM_FACES] = {
        vec3(1, 0, 0),
        vec3(-1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, -1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, -1)
    };

    const vec3 up_vectors[NUM_FACES] = {
        vec3(0, -1, 0),
        vec3(0, -1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, -1),
        vec3(0, -1, 0),
        vec3(0, -1, 0)
    };

    Camera cam;
    cam.setPerspective(90.0f, 1.0f, 0.1f, 1000.0f);
    cam.eye = root.model.getTranslation(); 

    for (int face = 0; face < NUM_FACES; face++) {
        cam.lookAt(cam.eye, cam.eye + directions[face], up_vectors[face]);
        capture_FBOs[face].setTexture(cubemap, face);
        renderer->renderFBO(&cam, &capture_FBOs[face], "forward_light_single_pass");
    }

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    cubemap->generateMipmaps();
}

void ReflectionProbeEntity::uploadUniforms(GFX::Shader* shader) const
{
    shader->setUniform("u_reflection_probe", cubemap, 10);
    shader->setUniform("u_reflection_strength", reflection_strength); 
    shader->setUniform("u_probe_position", root.model.getTranslation());
    shader->setUniform("u_probe_range", range);
}