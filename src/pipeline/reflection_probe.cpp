#include "reflection_probe.h"
#include "../pipeline/scene.h"
#include "../gfx/fbo.h"
#include "../gfx/texture.h"
#include "../pipeline/camera.h"
#include "../gfx/mesh.h"
#include "../pipeline/renderer.h"

ReflectionProbeEntity::ReflectionProbeEntity() {
    cubemap = new GFX::Texture();
    cubemap->createCubemap(512, 512);
}

ReflectionProbeEntity::~ReflectionProbeEntity() {
    if (cubemap) {
        delete cubemap;
        cubemap = nullptr;
    }
}


void ReflectionProbeEntity::captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer) const{
    static const vec3 directions[6] = {
        vec3(1, 0, 0), vec3(-1, 0, 0),
        vec3(0, 1, 0), vec3(0, -1, 0),
        vec3(0, 0, 1), vec3(0, 0, -1)
    };
    static const vec3 up_vectors[6] = {
        vec3(0, -1, 0), vec3(0, -1, 0),
        vec3(0, 0, 1), vec3(0, 0, -1),
        vec3(0, -1, 0), vec3(0, -1, 0)
    };

    GFX::FBO capture_fbo;
    capture_fbo.create(cubemap->width, cubemap->height, 1, GL_RGB, GL_UNSIGNED_BYTE, true); //setup with color + depth

    Camera cam;
    cam.setPerspective(90.0f, 1.0f, 0.1f, 1000.0f);
    cam.eye = this->position;

    for (int face = 0; face < 6; ++face) {
        cam.lookAt(cam.eye, cam.eye + directions[face], up_vectors[face]);

        capture_fbo.setTexture(this->cubemap, face);
        capture_fbo.bind();
        capture_fbo.enableSingleBuffer(0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer->renderScene(scene, &cam);

        capture_fbo.unbind();
    }
}

void ReflectionProbeEntity::setPosition(const vec3& pos) {
    this->position = pos;
}


