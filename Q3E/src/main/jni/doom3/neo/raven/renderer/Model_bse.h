#ifndef _RV_MODEL_BSE_H
#define _RV_MODEL_BSE_H

/*
======================
rvRenderModelBSE
======================
*/
class rvRenderModelBSE : public idRenderModelStatic {
    public:
		rvRenderModelBSE();

        virtual void				InitFromFile(const char* fileName);
        virtual void				FinishSurfaces(bool useMikktspace);
};

#endif
