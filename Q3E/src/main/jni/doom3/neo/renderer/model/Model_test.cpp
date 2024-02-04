#include "../../framework/Session_local.h"

// instead game::idTestModel in engine using idGameEdit interface

#define MODEL_OFFSET_DISTANCE 50

namespace modeltest
{
    typedef struct md5anim_info_s
    {
        idStr name;
        int frames;
        int length;
    } md5anim_info_t;
    typedef idList<md5anim_info_t> md5anim_info_list_t;

    ID_INLINE static int FRAME2MS( int framenum ) {
        return ( framenum * 1000 ) / 24;
    }

    enum {
        CAN_TEST = 0,
        CAN_TEST_NO_WORLD,
        CAN_TEST_NO_PLAYER,
        CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION,
        CAN_TEST_OTHER,
    };

    class idModelTest
    {
    public:
        idModelTest();
        ~idModelTest();

        idRenderWorld *         RenderWorld(void) {
            return sessLocal.rw;
        }
        void                    Render(int time);
        void                    Clean(void);
        bool                    HasModel(void) const {
            return modelDef != -1 && worldEntity.hModel;
        }
        void					TestModel(const char *model, const char *classname = NULL, const char *skin = NULL, const idDict *dict = NULL);
        void					TestEntity(const char *classname, const char *skin = NULL, const idDict *dict = NULL);
        void					TestAnim(const char *name, int startFrame = 0, int endFrame = -1, int time = -1);
        void					TestFrameRange(int startFrame, int endFrame, int time = -1);
        void					TestSkin(const char *skin);
        int                     CanTest(void);
        bool                    IsAnimatedModel(void) const;
        int                     NextAnim(int time = -1);
        int                     PrevAnim(int time = -1);
        void                    TestFrame(int *frame = NULL);
        int                     NextFrame(int i = 1);
        int                     PrevFrame(int i = 1);
        int                     ListAnim(void) const;
        int                     AnimationList(md5anim_info_list_t &list) const;

    private:
        void                    BuildAnimation(int time);
        int                     GetAnimIndex(const char *animName) const;
        const char *            GetAnimName(int index, int *realIndex = NULL) const;
        void                    UpdateAnimTime(int time);

    private:
        renderEntity_t          worldEntity;
        idStr                   modelName;
        qhandle_t               modelDef;
        idStr                   animName;
        const idMD5Anim *       modelAnim;
        int                     animLength;
        int                     animStartTime;
        int                     animEndTime;
        bool                    updateAnimation;
        idStr                   animClass;
        int                     numAnim;
        int                     numFrame;
        int                     frameIndex;
        idStr                   skinName;
        bool                    noshadows;
        bool                    noselfshadows;
        bool                    noDynamicInteractions;
        int                     startFrame;
        int                     endFrame;

        friend void ArgCompletion_frameRange(const idCmdArgs &args, void(*callback)(const char *s));
    };

    idModelTest::idModelTest()
    : modelDef(-1),
      modelAnim(NULL),
      animLength(0),
      animStartTime(-1),
      animEndTime(-1),
      updateAnimation(false),
      animClass("func_static"),
      numAnim(0),
      numFrame(0),
      frameIndex(-1),
      noshadows(false),
      noselfshadows(false),
      noDynamicInteractions(false),
      startFrame(0),
      endFrame(-1)
    {
        memset(&worldEntity, 0, sizeof(worldEntity));
    }

    idModelTest::~idModelTest()
    {
        // Clean();
    }

    bool idModelTest::IsAnimatedModel(void) const
    {
        return(worldEntity.hModel && worldEntity.hModel->IsDynamicModel() == DM_CACHED);
    }

    int idModelTest::GetAnimIndex(const char *animName) const
    {
        if(!IsAnimatedModel())
            return -1;
        if(numAnim <= 0)
            return -1;
        idDict spawnArgs;
        spawnArgs.Clear();
        spawnArgs.Set("model", modelName.c_str());
        for(int i = 0; i < numAnim; i++)
        {
            const char *name = gameEdit->ANIM_GetAnimNameFromEntityDef(&spawnArgs, i + 1);
            if(!idStr::Icmp(animName, name))
                return i;
        }
        return -1;
    }

    const char * idModelTest::GetAnimName(int index, int *realIndex) const
    {
        if(!IsAnimatedModel())
            return "";
        if(numAnim <= 0)
            return "";
        if(index < 0)
            index = numAnim + index;
        if(index < 0 || index >= numAnim)
            return "";
        idDict spawnArgs;
        spawnArgs.Clear();
        spawnArgs.Set("model", modelName.c_str());
        if(realIndex)
            *realIndex = index;
        return gameEdit->ANIM_GetAnimNameFromEntityDef(&spawnArgs, index + 1);
    }

    int idModelTest::ListAnim(void) const
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return -1;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return -1;
        }
        common->Printf("num animation: %d\n", numAnim);
        idDict spawnArgs;
        spawnArgs.Clear();
        spawnArgs.Set("model", modelName.c_str());
        for(int i = 0; i < numAnim; i++)
        {
            const char *name = gameEdit->ANIM_GetAnimNameFromEntityDef(&spawnArgs, i + 1);
            const idMD5Anim *anim = gameEdit->ANIM_GetAnimFromEntityDef(animClass, name);
            if(anim)
                common->Printf("  %3d: %s (frames: %d, length: %d)\n", i, name, gameEdit->ANIM_GetNumFrames(anim), gameEdit->ANIM_GetLength(anim));
            else
                common->Printf("  %3d: %s (NULL)\n", i, name);
        }
        return numAnim;
    }

    int idModelTest::AnimationList(md5anim_info_list_t &list) const
    {
        if(!worldEntity.hModel)
            return -1;
        if(!IsAnimatedModel())
            return -1;
        idDict spawnArgs;
        spawnArgs.Clear();
        spawnArgs.Set("model", modelName.c_str());
        for(int i = 0; i < numAnim; i++)
        {
            const char *name = gameEdit->ANIM_GetAnimNameFromEntityDef(&spawnArgs, i + 1);
            const idMD5Anim *anim = gameEdit->ANIM_GetAnimFromEntityDef(animClass, name);
            if(anim)
            {
                md5anim_info_t info;
                info.name = name;
                info.frames = gameEdit->ANIM_GetNumFrames(anim);
                info.length = gameEdit->ANIM_GetLength(anim);
                list.Append(info);
            }
        }
        return numAnim;
    }

    int idModelTest::CanTest(void)
    {
        idRenderWorld *world = RenderWorld();
        if(!world)
            return CAN_TEST_NO_WORLD;
        if(tr.primaryWorld != world)
            return CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION;
        if(!gameEdit->PlayerIsValid())
            return CAN_TEST_NO_PLAYER;
        return CAN_TEST;
    }

    void idModelTest::Clean(void)
    {
        if(modelDef != -1)
            RenderWorld()->FreeEntityDef(modelDef);
        memset(&worldEntity, 0, sizeof(worldEntity));
        modelName.Clear();
        animName.Clear();
        modelAnim = NULL;
        modelDef = -1;
        animLength = 0;
        animStartTime = -1;
        animEndTime = -1;
        updateAnimation = false;
        animClass = "func_static";
        numAnim = 0;
        numFrame = 0;
        frameIndex = -1;
        skinName.Clear();
        noshadows = false;
        noselfshadows = false;
        noDynamicInteractions = false;
        startFrame = 0;
        endFrame = -1;
    }

    void idModelTest::BuildAnimation(int time)
    {
        if (!updateAnimation) {
            return;
        }

        updateAnimation = false;
        numFrame = 0;
        modelAnim = NULL;

        if(!IsAnimatedModel())
            return;

        if (animName.Length()) {
            worldEntity.numJoints = worldEntity.hModel->NumJoints();
            worldEntity.joints = (idJointMat *)Mem_Alloc16(worldEntity.numJoints * sizeof(*worldEntity.joints));
            modelAnim = gameEdit->ANIM_GetAnimFromEntityDef(animClass, animName);

            if (modelAnim) {
                numFrame = gameEdit->ANIM_GetNumFrames(modelAnim);
                animLength = gameEdit->ANIM_GetLength(modelAnim);
                common->Printf("frame total length: %d\n", animLength);
                UpdateAnimTime(time);
                common->Printf("num frame: %d\n", numFrame);
                common->Printf("frame length: %d\n", animLength);
            }
        }
    }

    void idModelTest::TestAnim(const char *name, int start, int end, int time)
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return;
        common->Printf("animation: %s\n", name);
        frameIndex = -1;
        numFrame = 0;
        modelAnim = NULL;
        animName = name;
        startFrame = start;
        endFrame = end;
        if(!animName.IsEmpty())
        {
            updateAnimation = true;
            if(time >= 0)
                BuildAnimation(time);
        }
    }

    void idModelTest::TestFrameRange(int start, int end, int time)
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return;
        }
        if(!modelAnim)
        {
            common->Printf("No test animation!\n");
            return;
        }
        if(numFrame <= 0)
        {
            common->Printf("No animation frame!\n");
            return;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return;
        if(start < 0)
            start = numFrame + start;
        if(start < 0 || start >= numFrame)
        {
            common->Printf("Start frame %d out of range(%d - %d)!\n", start, 0, numFrame);
            return;
        }
        if(end < 0)
            end = numFrame + end;
        if(end < start || end >= numFrame)
        {
            common->Printf("End frame %d out of range(%d - %d)!\n", end, start, numFrame);
            return;
        }
        common->Printf("animation range: %s(%d - %d)\n", animName.c_str(), start, end);
        frameIndex = -1;
        startFrame = start;
        endFrame = end;
        updateAnimation = true;
        if(time >= 0)
            BuildAnimation(time);
    }

    void idModelTest::UpdateAnimTime(int time)
    {
        if(!modelAnim)
            return;

        animLength = gameEdit->ANIM_GetLength(modelAnim);
        animEndTime = time + animLength;
        animStartTime = 0;

        if(startFrame > 0 && startFrame < numFrame)
            animStartTime = (1.0f / (float)numFrame * (float)startFrame) * animLength;
        else
            startFrame = 0;
        if(endFrame >= startFrame && endFrame < numFrame)
        {
            animLength = (1.0f / (float)numFrame * (float)(endFrame + 1 - startFrame)) * animLength;
            animEndTime = time + animLength;
        }
        else
            endFrame = -1;
    }

    void idModelTest::TestFrame(int *frame)
    {
        frameIndex = -1;
        if(!frame)
            return;
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return;
        }
        if(!modelAnim)
        {
            common->Printf("No test animation!\n");
            return;
        }
        if(numFrame <= 0)
        {
            common->Printf("No animation frame!\n");
            return;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return;
        int f = *frame;
        if(f < 0)
            f = numFrame + f;
        if(f < 0 || f >= numFrame)
        {
            common->Printf("Frame %d out or range(%d - %d)!\n", f, 0, numFrame);
            return;
        }
        common->Printf("animation frame: %d\n", f);
        *frame = f;
        frameIndex = f;
    }

    int idModelTest::NextFrame(int i)
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return -1;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return -1;
        }
        if(!modelAnim)
        {
            common->Printf("No test animation!\n");
            return -1;
        }
        if(numFrame <= 0)
        {
            common->Printf("No animation frame!\n");
            return -1;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return -1;
        int f;
        if(frameIndex < 0)
            f = 0;
        else
            f = frameIndex + i;
        TestFrame(&f);
        return f;
    }

    int idModelTest::PrevFrame(int i)
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return -1;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return -1;
        }
        if(!modelAnim)
        {
            common->Printf("No test animation!\n");
            return -1;
        }
        if(numFrame <= 0)
        {
            common->Printf("No animation frame!\n");
            return -1;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return -1;
        int f;
        if(frameIndex < 0)
            f = -1;
        else
            f = frameIndex - i;
        TestFrame(&f);
        return f;
    }

    int idModelTest::NextAnim(int time)
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return -1;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return -1;
        }
        if(numAnim <= 0)
        {
            common->Printf("Test model no animations!\n");
            return -1;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return -1;
        const char *n;
        int index;
        if(animName.IsEmpty())
        {
            index = 0;
            n = GetAnimName(index);
            common->Printf("animation index: %d\n", index);
        }
        else
        {
            index = GetAnimIndex(animName);
            index++;
            index = index % numAnim;
            n = GetAnimName(index);
            common->Printf("animation index: %d\n", index);
        }
        TestAnim(n, time);
        return n && n[0] ? index : -1;
    }

    int idModelTest::PrevAnim(int time)
    {
        if(!worldEntity.hModel)
        {
            common->Printf("No test model yet!\n");
            return -1;
        }
        if(!IsAnimatedModel())
        {
            common->Printf("Test model is not dynamic!\n");
            return -1;
        }
        if(numAnim <= 0)
        {
            common->Printf("Test model no animations!\n");
            return -1;
        }
        idRenderWorld *world = RenderWorld();
        if(!world)
            return -1;
        const char *n;
        int index;
        if(animName.IsEmpty())
        {
            index = numAnim - 1;
            n = GetAnimName(index, &index);
            common->Printf("animation index: %d\n", index);
        }
        else
        {
            index = GetAnimIndex(animName);
            index--;
            n = GetAnimName(index, &index);
            common->Printf("animation index: %d\n", index);
        }
        TestAnim(n, time);
        return n && n[0] ? index : -1;
    }

    void idModelTest::TestSkin(const char *skin)
    {
        idStr modelName = this->modelName;
        idStr animClass = this->animClass;
        idStr animName = this->animName;
        int frameIndex = this->frameIndex;

        idDict dict;
        dict.Clear();
        dict.SetBool("noshadows", noshadows);
        dict.SetBool("noselfshadows", noselfshadows);
        dict.SetBool("noDynamicInteractions", noDynamicInteractions);

        TestModel(modelName, animClass, skin, &dict);

        TestAnim(animName);
        TestFrame(&frameIndex);
    }

    void idModelTest::TestEntity(const char *classname, const char *skin, const idDict *dict)
    {
        idRenderWorld *world = RenderWorld();
        if(!world)
            return;

        const idDict *args = gameEdit->FindEntityDefDict(classname);
        idStr name = args->GetString("model");
        TestModel(name, classname, skin, dict);
    }

    void idModelTest::TestModel(const char *model, const char *classname, const char *skin, const idDict *dict)
    {
        idRenderWorld *world = RenderWorld();
        if(!world)
            return;

        Clean();

        modelName = model;
        if(modelName.IsEmpty())
            return;

        if(!classname || !classname[0])
        {
            idStr extension;
            modelName.ExtractFileExtension(extension);
            if ((extension.Icmp("ase") != 0) && (extension.Icmp("lwo") != 0) && (extension.Icmp("ma") != 0)
#ifdef _MODEL_OBJ
                && (extension.Icmp("obj") != 0)
#endif
#ifdef _MODEL_DAE
                && (extension.Icmp("dae") != 0)
#endif
                    )
                animClass = "func_static";
            else
                animClass = "func_animate";
        }
        else
            animClass = classname;

        idVec3 pos;
        gameEdit->PlayerGetEyePosition(pos);
        idAngles angles;
        gameEdit->PlayerGetViewAngles(angles);
        pos += angles.ToForward() * MODEL_OFFSET_DISTANCE;

        idStr modelOrigin;
        modelOrigin += pos.x;
        modelOrigin += " ";
        modelOrigin += pos.y;
        modelOrigin += " ";
        modelOrigin += pos.z;

        noshadows = false;
        noselfshadows = false;
        noDynamicInteractions = false;
        if(dict)
        {
            noshadows = dict->GetBool("noshadows", "0");
            noselfshadows = dict->GetBool("noselfshadows", "0");
            noDynamicInteractions = dict->GetBool("noDynamicInteractions", "0");
            if(!skin || !skin[0])
                skin = dict->GetString("skin", "");
        }
        skinName = skin;

        common->Printf("model: %s\n", model);
        common->Printf("classname: %s\n", animClass.c_str());
        common->Printf("skin: %s\n", skinName.c_str());

        idDict spawnArgs;
        memset(&worldEntity, 0, sizeof(worldEntity));
        spawnArgs.Clear();
        spawnArgs.Set("classname", animClass.c_str());
        spawnArgs.Set("model", modelName.c_str());
        spawnArgs.Set("origin", modelOrigin.c_str());
        spawnArgs.SetBool("noshadows", noshadows);
        spawnArgs.SetBool("noselfshadows", noselfshadows);
        spawnArgs.SetBool("noDynamicInteractions", noDynamicInteractions);
        if(skin && skin[0])
            spawnArgs.Set("skin", skin);
        gameEdit->ParseSpawnArgsToRenderEntity(&spawnArgs, &worldEntity);

        if (worldEntity.hModel) {
            worldEntity.shaderParms[0] = 1;
            worldEntity.shaderParms[1] = 1;
            worldEntity.shaderParms[2] = 1;
            worldEntity.shaderParms[3] = 1;
            modelDef = world->AddEntityDef(&worldEntity);

            if(IsAnimatedModel())
            {
                numAnim = gameEdit->ANIM_GetNumAnimsFromEntityDef(&spawnArgs) - 1;
                common->Printf("animated: true\n");
                common->Printf("num anim: %d\n", numAnim);
                NextAnim();
            }
            else
                common->Printf("animated: false\n");
        }
    }

    void idModelTest::Render(int time)
    {
        if (worldEntity.hModel) {
            idRenderWorld *world = RenderWorld();
            if(!world)
                return;
            if (updateAnimation) {
                BuildAnimation(time);
            }

            if (modelAnim) {
                if (time > animEndTime) {
                    UpdateAnimTime(time);
                }

                if(frameIndex < 0)
                {
                    gameEdit->ANIM_CreateAnimFrame(worldEntity.hModel, modelAnim, worldEntity.numJoints, worldEntity.joints, animLength - (animEndTime - time) + animStartTime, vec3_origin, false);
                }
                else
                    gameEdit->ANIM_CreateAnimFrame(worldEntity.hModel, modelAnim, worldEntity.numJoints, worldEntity.joints, FRAME2MS(frameIndex), vec3_origin, false);
            }

            world->UpdateEntityDef(modelDef, &worldEntity);
        }
    }

    static idModelTest modelTest;

    static bool CanTest(void)
    {
        int res;

        res = modelTest.CanTest();
        if(res == CAN_TEST)
            return true;
        switch (res) {
            case CAN_TEST_NO_WORLD:
                common->Printf("No session render world!\n");
                break;
            case CAN_TEST_NO_PLAYER:
                common->Printf("No player!\n");
                break;
            case CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION:
                common->Printf("Current render world is not session\n");
                break;
            case CAN_TEST_OTHER:
            default:
                common->Printf("Other reason!\n");
                break;
        }
        return false;
    }

    static idDict ModelTest_ParseModelArgs(const idCmdArgs &args, int start)
    {
        idDict dict;
        dict.Clear();

        idStr skin;
        bool noshadows = false;
        bool noselfshadows = false;
        bool noDynamicInteractions = false;
        for(int i = 2; i < args.Argc(); i++)
        {
            const char *arg = args.Argv(i);
            if(!idStr::Icmp(arg, "noshadows"))
                noshadows = true;
            else if(!idStr::Icmp(arg, "noselfshadows"))
                noselfshadows = true;
            else if(!idStr::Icmp(arg, "noDynamicInteractions"))
                noDynamicInteractions = true;
            else if(!idStr::Icmp(arg, "skin"))
            {
                i++;
                if(i < args.Argc())
                    skin = args.Argv(i);
                else
                    common->Warning("Missing skin param!");
            }
        }

        dict.SetBool("noshadows", noshadows);
        dict.SetBool("noselfshadows", noselfshadows);
        dict.SetBool("noDynamicInteractions", noDynamicInteractions);
        dict.Set("skin", skin.c_str());
        return dict;
    }

    void TestModel_f(const idCmdArgs &args)
    {
        idStr			name;
        idDict			dict;

        if (args.Argc() < 2) {
            common->Printf("Usage: modelTest <model_name> [<class_name> skin <skin_name> noshadows noselfshadows noDynamicInteractions].\n");
            // delete the testModel if active
            modelTest.Clean();
            return;
        }

        if (!CanTest()) {
            return;
        }

        name = args.Argv(1);
        idStr classname = args.Argc() > 2 ? args.Argv(2) : NULL;
        idDict testArgs = ModelTest_ParseModelArgs(args, 2);

        modelTest.TestModel(name, classname, testArgs.GetString("skin", ""), &testArgs);
        common->Printf("TestModel active.\n");
    }

    void CleanTestModel_f(const idCmdArgs &args)
    {
        modelTest.Clean();
        common->Printf("TestModel clean.\n");
    }

    void TestEntity_f(const idCmdArgs &args)
    {
        idStr			name;
        idDict			dict;

        if (args.Argc() < 2) {
            common->Printf("Usage: modelEntity <class_name> [skin <skin_name> noshadows noselfshadows noDynamicInteractions].\n");
            // delete the testModel if active
            modelTest.Clean();
            return;
        }

        if (!CanTest()) {
            return;
        }

        name = args.Argv(1);
        idDict testArgs = ModelTest_ParseModelArgs(args, 2);

        modelTest.TestEntity(name, NULL, &testArgs);
        common->Printf("TestEntity active.\n");
    }

    void TestSkin_f(const idCmdArgs &args)
    {
        idStr			name;
        idDict			dict;

        if (!CanTest()) {
            return;
        }

        const char *skin = args.Argc() > 1 ? args.Argv(1) : "";

        modelTest.TestSkin(skin);
        common->Printf("TestSkin active.\n");
    }

#if 1
#define _TEST_ANIM_TIME -1
#else
#define _TEST_ANIM_TIME tr.primaryView->renderView.time
#endif
    void TestAnim_f(const idCmdArgs &args)
    {
        if (args.Argc() < 2) {
            common->Printf("Usage: modelAnim <anim_name> [<start_frame> <end_frame>].\n");
            return;
        }
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        int start = args.Argc() > 2 ? atoi(args.Argv(2)) : 0;
        int end = args.Argc() > 3 ? atoi(args.Argv(3)) : -1;

        modelTest.TestAnim(args.Argv(1), start, end, _TEST_ANIM_TIME);
    }

    void PrevAnim_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        modelTest.PrevAnim(_TEST_ANIM_TIME);
    }

    void NextAnim_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        modelTest.NextAnim(_TEST_ANIM_TIME);
    }

    void TestFrameRange_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        int start = args.Argc() > 1 ? atoi(args.Argv(1)) : 0;
        int end = args.Argc() > 2 ? atoi(args.Argv(2)) : -1;
        modelTest.TestFrameRange(start, end, _TEST_ANIM_TIME);
    }

    void TestAnimFrame_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        int frame = args.Argc() > 1 ? atoi(args.Argv(1)) : 0;
        modelTest.TestFrame(args.Argc() > 1 ? &frame : NULL);
    }

    void PrevAnimFrame_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        int count = args.Argc() > 1 ? atoi(args.Argv(1)) : 1;
        modelTest.PrevFrame(count);
    }

    void NextAnimFrame_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        int count = args.Argc() > 1 ? atoi(args.Argv(1)) : 1;
        modelTest.NextFrame(count);
    }

    void ListAnim_f(const idCmdArgs &args)
    {
        if (!CanTest()) {
            return;
        }

        if (!modelTest.HasModel()) {
            common->Printf("No testModel active.\n");
            return;
        }

        modelTest.ListAnim();
    }

#undef _TEST_ANIM_TIME

    void ArgCompletion_modelTest(const idCmdArgs &args, void(*callback)(const char *s))
    {
        int i, num;

        num = declManager->GetNumDecls(DECL_MODELDEF);

        for (i = 0; i < num; i++) {
            callback(idStr(args.Argv(0)) + " " + declManager->DeclByIndex(DECL_MODELDEF, i , false)->GetName());
        }

        cmdSystem->ArgCompletion_FolderExtension(args, callback, "models/", false, ".lwo", ".ase", ".md5mesh", ".ma", ".mb",
#ifdef _MODEL_OBJ
                                         "obj",
#endif
#ifdef _MODEL_DAE
                                         "dae",
#endif
                                                 NULL);
    }

    void ArgCompletion_testAnim(const idCmdArgs &args, void(*callback)(const char *s))
    {
        md5anim_info_list_t list;
        if (modelTest.AnimationList(list) > 0) {
            for (int i = 0; i < list.Num(); i++) {
                callback(va("%s %s", args.Argv(0), list[i].name.c_str()));
            }
        }
    }

    void ArgCompletion_frameRange(const idCmdArgs &args, void(*callback)(const char *s))
    {
        callback(va("%s %s", args.Argv(0), "0"));
        idStr str;
        str += modelTest.numFrame - 1;
        callback(va("%s %s", args.Argv(0), str.c_str()));
    }
}

void ModelTest_TestModel(int time)
{
    if(modeltest::modelTest.HasModel() && modeltest::modelTest.CanTest() == modeltest::CAN_TEST)
    {
        modeltest::modelTest.Render(time);
    }
}

void ModelTest_AddCommand(void)
{
    using namespace modeltest;
    cmdSystem->AddCommand("modelTest", TestModel_f, CMD_FL_RENDERER, "test model", modeltest::ArgCompletion_modelTest);
    cmdSystem->AddCommand("modelEntity", TestEntity_f, CMD_FL_RENDERER, "test model from entity", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
    cmdSystem->AddCommand("modelSkin", TestSkin_f, CMD_FL_RENDERER, "test skin", idCmdSystem::ArgCompletion_Decl<DECL_SKIN>);
    cmdSystem->AddCommand("modelClean", CleanTestModel_f, CMD_FL_RENDERER, "clean test model");

    cmdSystem->AddCommand("modelAnim", TestAnim_f, CMD_FL_RENDERER, "test model anim", ArgCompletion_testAnim);
    cmdSystem->AddCommand("modelPrevAnim", PrevAnim_f, CMD_FL_RENDERER, "test model anim");
    cmdSystem->AddCommand("modelNextAnim", NextAnim_f, CMD_FL_RENDERER, "test model anim");

    cmdSystem->AddCommand("modelFrame", TestAnimFrame_f, CMD_FL_RENDERER, "test model anim frame", modeltest::ArgCompletion_frameRange);
    cmdSystem->AddCommand("modelPrevFrame", PrevAnimFrame_f, CMD_FL_RENDERER, "test model anim frame");
    cmdSystem->AddCommand("modelNextFrame", NextAnimFrame_f, CMD_FL_RENDERER, "test model anim frame");
    cmdSystem->AddCommand("modelFrameRange", TestFrameRange_f, CMD_FL_RENDERER, "test model anim frame range", modeltest::ArgCompletion_frameRange);

    cmdSystem->AddCommand("modelListAnim", ListAnim_f, CMD_FL_RENDERER, "list test model animations");
}