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
        int width;
        int height;
        vec3 position;
        float range;
        float reflection_strength;
        GFX::Texture cubemap;
        GFX::FBO capture_fbo;
        float sphere_radius;
        GFX::Mesh sphere;

        ReflectionProbeEntity();
        ~ReflectionProbeEntity();

        ENTITY_METHODS(ReflectionProbeEntity, REFLECTION_PROBE, 2, 4);

        void configure(cJSON* json);
        void serialize(cJSON* json);

        void captureEnvironment(SCN::Scene* scene, SCN::Renderer* renderer);
        void uploadUniforms(GFX::Shader* shader) const;
        void setPosition(const vec3& pos);
        void renderSphere(SCN::Renderer* renderer);
    };

    class ReflectionProbeGrid : public SCN::BaseEntity
    {
    public:
        vec3 probe_grid_dimensions;
        float probe_spacing;
        vec3 probe_grid_origin;
        std::vector<ReflectionProbeEntity*> reflection_probes;
        SCN::ReflectionProbeEntity* closest_probe;
        bool render_spheres_mode;
        bool update_reflections_button;

        ReflectionProbeGrid();
        ~ReflectionProbeGrid();

        ENTITY_METHODS(ReflectionProbeGrid, REFLECTION_PROBE_GRID, 2, 4);

        void create();
        void clear();
        void updateClosestProbe(GFX::Shader* shader);
        void renderSpheres(SCN::Renderer* render);
        void updatdateReflections(SCN::Renderer* renderer);
    };
}


