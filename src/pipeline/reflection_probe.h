#pragma once
#include "../gfx/texture.h"
#include "../pipeline/scene.h"

class Scene;

namespace SCN {
    class Renderer;
}

class ReflectionProbeEntity{
public:
    GFX::Texture* cubemap = nullptr;
    float range = 10.0f;
    vec3 position;

    ReflectionProbeEntity();
    ~ReflectionProbeEntity();

    void setPosition(const vec3& pos);

    void captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer) const;
};
