#pragma once
#include "../gfx/texture.h"
#include "../gfx/fbo.h"
#include "../pipeline/scene.h"

#define NUM_FACES 6

namespace GFX {
    class FBO;
    class Shader;
}

namespace SCN {
    class Scene;
    class BaseEntity;
    class Renderer;

    class ReflectionProbeEntity : public SCN::BaseEntity
    {
    public:
        float range;
        float reflection_strength;
        GFX::Texture* cubemap;
        GFX::FBO capture_FBOs[NUM_FACES];

        ReflectionProbeEntity();
        ~ReflectionProbeEntity();

        ENTITY_METHODS(ReflectionProbeEntity, REFLECTION_PROBE, 2, 4);

        void configure(cJSON* json);
        void serialize(cJSON* json);

        void captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer);
        void uploadUniforms(GFX::Shader* shader) const;
    };
}


