#pragma once
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/scene.h"

#define NUM_FACES 6

class Scene;

namespace SCN {
    class Renderer;
}

namespace GFX {
    class FBO;
    class Shader;
}

class ReflectionProbeEntity 
{
public:
    GFX::Texture* cubemap;
    int texture_unit;
    float range;
    float reflection_strength;
    vec3 position;
    GFX::FBO capture_FBOs[NUM_FACES];

    ReflectionProbeEntity();
    ~ReflectionProbeEntity();

    void setPosition(const vec3& pos);
    void captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer);
    void uploadUniforms(GFX::Shader* shader) const;

private:
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
};
