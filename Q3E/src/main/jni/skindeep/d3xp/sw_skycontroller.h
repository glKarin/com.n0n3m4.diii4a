#ifndef __SKYCONTROLLER_H__
#define __SKYCONTROLLER_H__


#include "Entity.h"
#include "Misc.h"
#include "Light.h"

class idDynamicSkyBrush : public idStaticEntity
{
    public:
        CLASS_PROTOTYPE(idDynamicSkyBrush);

        void SetSkyMaterial(const idMaterial* sky);
};

class idSkyController : public idEntity
{
    public:
        CLASS_PROTOTYPE(idSkyController);

        idSkyController();

        void Init(void);
        idList<const idMaterial*> GetSkyList();
        const idMaterial* GetCurrentSky(void);
        void SetCurrentSky(int skyIndex); // Uses index in the list
        void NextSky(void);
        void ReloadSky(void);

    private:
        void ParseSkies(const idDict* spawnArgs);
        void FindSkyBrushes(void);
        void FindLinkedLights(void);
        void SetCurrentSky(const idMaterial* sky); // Fails if the sky is not in the list
        void TryAddSky(idStr name);

        idList<idLight*> linkedLights; // Environment lights we need to update when switching skies
        idList<const idMaterial*> skies; // These are the skies we *could* be displaying. We only display one at once.
        idList<idDynamicSkyBrush*> skyBrushes; // These are the brushes onto which our dynamic sky is projected.
        int skyIndex; // Index of the currently used sky in the list. -1 is invalid
};

#endif