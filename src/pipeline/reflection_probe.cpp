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
    width = 512;
    height = 512;

    range = 10.0f;
    reflection_strength = 40.0f; 

    cubemap.createCubemap(width, height, NULL, GL_RGB, GL_UNSIGNED_INT, true);
    capture_fbo.create(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, true);

    sphere_radius = 0.1;
    sphere.createSphere(sphere_radius);
    sphere.uploadToVRAM();
}

ReflectionProbeEntity::~ReflectionProbeEntity() 
{
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
        capture_fbo.setTexture(&cubemap, face);
        renderer->renderFBO(&cam, &capture_fbo, "forward_light_single_pass");
    }

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    cubemap.generateMipmaps();
}

void ReflectionProbeEntity::uploadUniforms(GFX::Shader* shader) const
{
    shader->setUniform("u_reflection_probe", (GFX::Texture*)&cubemap, 10);
    shader->setUniform("u_reflection_strength", reflection_strength); 
    shader->setUniform("u_probe_position", root.model.getTranslation()); //root.model.getTranslation()); //position);
    shader->setUniform("u_probe_range", range);
}

void ReflectionProbeEntity::setPosition(const vec3& pos) 
{
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

    shader->setUniform("u_model", root.model);
    shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
    shader->setUniform("u_camera_position", camera->eye);

    uploadUniforms(shader);

    if (renderer->render_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    sphere.render(GL_TRIANGLES);

    shader->disable();

    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

ReflectionProbeGrid::ReflectionProbeGrid()
{
    name = "reflection_probes";
    render_spheres_mode = true;
    probe_grid_dimensions = vec3(7.0, 2.0, 7.0);
    probe_spacing = 0.8f;
    probe_grid_origin = vec3(0.0, 1.25, 0.0);
    create();
}

ReflectionProbeGrid::~ReflectionProbeGrid()
{
    for (ReflectionProbeEntity* reflection_probe : reflection_probes)
        delete reflection_probe;
    reflection_probes.clear();
}

void ReflectionProbeGrid::renderSpheres(SCN::Renderer* renderer)
{
    if (!render_spheres_mode)
        return;
    for (ReflectionProbeEntity* probe : this->reflection_probes) {
        probe->renderSphere(renderer);
    }
}

void ReflectionProbeGrid::create()
{
    clear();
    vec3 grid_size = probe_grid_dimensions * probe_spacing;
    vec3 half_grid = grid_size * 0.5f;

    for (int x = 0; x <= probe_grid_dimensions.x; ++x) {
        for (int y = 0; y <= probe_grid_dimensions.y; ++y) {
            for (int z = 0; z <= probe_grid_dimensions.z; ++z) {
                vec3 pos = probe_grid_origin - half_grid + vec3(x, y, z) * probe_spacing;
                ReflectionProbeEntity* probe = new ReflectionProbeEntity();
                probe->setPosition(pos);
                probe->range = probe_spacing * 1.5f;
                reflection_probes.push_back(probe);
            }
        }
    }
}

void ReflectionProbeGrid::clear()
{
    for (ReflectionProbeEntity* reflection_probe : reflection_probes)
        delete reflection_probe;
    reflection_probes.clear();
}