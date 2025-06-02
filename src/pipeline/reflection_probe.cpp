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
    reflection_strength = 40.0f; 
    cubemap = new GFX::Texture();
    cubemap->createCubemap(w, h, NULL, GL_RGB, GL_UNSIGNED_INT, true);
    
    capture_fbo.create(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, true);

    this->sphere.createSphere(1.0f);
    this->sphere.uploadToVRAM();
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
        capture_fbo.setTexture(cubemap, face);
        renderer->renderFBO(&cam, &capture_fbo, "forward_light_single_pass");
    }

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    cubemap->generateMipmaps();
}

void ReflectionProbeEntity::uploadUniforms(GFX::Shader* shader) const
{
    shader->setUniform("u_reflection_probe", cubemap, 10);
    shader->setUniform("u_reflection_strength", reflection_strength); 
    shader->setUniform("u_probe_position", root.model.getTranslation()); //root.model.getTranslation()); //position);
    shader->setUniform("u_probe_range", range);
}

void ReflectionProbeEntity::setPosition(const vec3& pos) {
    this->position = pos;
    this->root.model.setIdentity();
    this->root.model.setTranslation(pos.x, pos.y, pos.z);
}

void ReflectionProbeEntity::renderSphere(SCN::Renderer* renderer)
{
    Camera* camera = Camera::current;

    glEnable(GL_DEPTH_TEST);
    assert(glGetError() == GL_NO_ERROR);

    GFX::Shader* shader = GFX::Shader::Get("sphere");
    if (!shader)
        return;
    shader->enable();

    Matrix44 m;
    m.setTranslation(
        this->root.model.getTranslation().x,
        this->root.model.getTranslation().y,
        this->root.model.getTranslation().z
    );
    m.scale(0.25, 0.25, 0.25);

    shader->setUniform("u_model", m);
    shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
    shader->setUniform("u_camera_position", camera->eye);


    this->uploadUniforms(shader);


    if (renderer->render_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    this->sphere.render(GL_TRIANGLES);

    shader->disable();

    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}