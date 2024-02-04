namespace md5anim
{
    typedef struct baseframe_s
    {
        idList<idStr> transforms;

        idStr ToString(void) const;
        bool Parse(idLexer &parser, int numJoints);
    } baseframe_t;

    typedef struct frame_s
    {
        int index;
        idList<idStr> transforms;

        idStr ToString(void) const;
        bool Parse(idLexer &parser, int numJoints);
    } frame_t;

    typedef struct hierarchy_s
    {
        idList<idStr> joints;

        idStr ToString(void) const;
        bool Parse(idLexer &parser, int numJoints);
    } hierarchy_t;

    typedef struct bounds_s
    {
        idList<idStr> frames;

        idStr ToString(void) const;
        bool Parse(idLexer &parser, int numFrames);
    } bounds_t;

    class idMD5AnimFile
    {
    private:
        int						numFrames;
        int						numJoints;
        int						frameRate;
        int						numAnimatedComponents;
        hierarchy_t             hierarchy;
        bounds_t        		bounds;
        baseframe_t             baseframe;
        idList<frame_t>	        frames;
        idStr					name;
    
    public:
        idMD5AnimFile();
    
        bool					LoadAnim(const char *filename);
        bool AppendAnim(const idMD5AnimFile &other, int startFrame = 0, int endFrame = -1);
        bool CutAnim(int startFrame = 0, int endFrame = -1);
        bool ReverseAnim(int startFrame = 0, int endFrame = -1);
        bool LoopAnim(int startFrame = 0, int endFrame = -1);
        idStr ToString(void) const;
        int NumFrames() const {
            return numFrames;
        }
    
    private:
        bool Check(void) const;
        bool Compat(const idMD5AnimFile &other) const;
        void NormalizedFrame(int &frame) const;
    };

    idStr ReadLine(idLexer &parser)
    {
        idToken token;
        parser.ReadToken(&token);
        idStr str;
        if(token.type == TT_STRING)
            str.Append("\"");
        str.Append(token);
        if(token.type == TT_STRING)
            str.Append("\"");
        parser.ReadRestOfLine(str);
        str.Strip(' ');
        // common->Printf(">> %s\n", str.c_str());
        return str;
    }

    int WriteText(const idStr &text, idFile *file)
    {
        return file->Write(text.c_str(), text.Length());
    }

    idMD5AnimFile::idMD5AnimFile()
    : numFrames(0),
    numJoints(0),
    frameRate(0),
    numAnimatedComponents(0)
    {
    }

    void idMD5AnimFile::NormalizedFrame(int &frame) const
    {
        if(frame < 0)
            frame = numFrames + frame;
        if(frame < 0)
            frame = 0;
        else if(frame >= numFrames)
            frame = numFrames - 1;
    }

    bool idMD5AnimFile::LoadAnim(const char *filename)
    {
        int		version;
        idLexer	parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT
        | LEXFL_NOFATALERRORS);
        idToken	token;
        int		i;

        if (!parser.LoadFile(filename)) {
            return false;
        }

        name = filename;

        parser.ExpectTokenString(MD5_VERSION_STRING);
        version = parser.ParseInt();

        if (version != MD5_VERSION) {
            parser.Error("Invalid version %d.  Should be version %d\n", version, MD5_VERSION);
            return false;
        }

        // skip the commandline
        parser.ExpectTokenString("commandline");
        parser.ReadToken(&token);

        // parse num frames
        parser.ExpectTokenString("numFrames");
        numFrames = parser.ParseInt();

        if (numFrames <= 0) {
            parser.Error("Invalid number of frames: %d", numFrames);
            return false;
        }

        // parse num joints
        parser.ExpectTokenString("numJoints");
        numJoints = parser.ParseInt();

        if (numJoints <= 0) {
            parser.Error("Invalid number of joints: %d", numJoints);
            return false;
        }

        // parse frame rate
        parser.ExpectTokenString("frameRate");
        frameRate = parser.ParseInt();

        if (frameRate < 0) {
            parser.Error("Invalid frame rate: %d", frameRate);
            return false;
        }

        // parse number of animated components
        parser.ExpectTokenString("numAnimatedComponents");
        numAnimatedComponents = parser.ParseInt();

        if ((numAnimatedComponents < 0) || (numAnimatedComponents > numJoints * 6)) {
            parser.Error("Invalid number of animated components: %d", numAnimatedComponents);
            return false;
        }

        // parse the hierarchy
        hierarchy.Parse(parser, numJoints);

        // parse bounds
        bounds.Parse(parser, numFrames);

        // parse base frame
        baseframe.Parse(parser, numJoints);

        // parse frames
        frames.SetGranularity(1);
        frames.SetNum(numFrames);

        for (i = 0; i < numFrames; i++) {
            frame_t &frame = frames[i];
            frame.Parse(parser, numJoints);

            if (frame.index != i) {
                parser.Error("Expected frame number %d", i);
                return false;
            }
        }

        /*idStr str = ToString();
        Sys_Printf("%s\n", str.c_str());*/
        // done
        return Check();
    }

    bool idMD5AnimFile::Check(void) const
    {
        if(numFrames <= 0)
        {
            common->Warning("numFrames <= 0: %d", numFrames);
            return false;
        }
        if(numJoints <= 0)
        {
            common->Warning("numJoints <= 0: %d", numJoints);
            return false;
        }
        if(frameRate <= 0)
        {
            common->Warning("frameRate <= 0: %d", frameRate);
            return false;
        }
        if(numAnimatedComponents <= 0)
        {
            common->Warning("numAnimatedComponents <= 0: %d", numAnimatedComponents);
            return false;
        }
        if(numAnimatedComponents != numJoints * 6)
        {
            common->Warning("numAnimatedComponents != numJoints * numFrames: %d != %d * 6", numAnimatedComponents, numJoints);
            return false;
        }

        if(bounds.frames.Num() != numFrames)
        {
            common->Warning("bounds.Num() != numFrames: %d != %d", bounds.frames.Num(), numFrames);
            return false;
        }
        if(hierarchy.joints.Num() != numJoints)
        {
            common->Warning("hierarchy.Num() != numJoints: %d != %d", hierarchy.joints.Num(), numJoints);
            return false;
        }
        if(baseframe.transforms.Num() != numJoints)
        {
            common->Warning("baseframe.Num() != numJoints: %d != %d", baseframe.transforms.Num(), numJoints);
            return false;
        }
        if(frames.Num() != numFrames)
        {
            common->Warning("frames.Num() != numFrames: %d != %d", frames.Num(), numFrames);
            return false;
        }
        
        for(int i = 0; i < numFrames; i++)
        {
            const frame_t  &frame = frames[i];
            if(frame.index != i)
            {
                common->Warning("frames.index != index: %d != %d", frame.index, i);
                return false;
            }
            if(frame.transforms.Num() != numJoints)
            {
                common->Warning("frames.Num() != numJoints: %d != %d", frame.transforms.Num(), numJoints);
                return false;
            }
        }
        return true;
    }

    bool idMD5AnimFile::Compat(const idMD5AnimFile &other) const
    {
        if(!other.Check())
            return false;

        if(numJoints != other.numJoints)
        {
            common->Warning("numJoints != numJoints: %d != %d", numJoints, other.numJoints);
            return false;
        }

        return true;
    }

    bool idMD5AnimFile::AppendAnim(const idMD5AnimFile &other, int startFrame, int endFrame)
    {
        if(numJoints == 0)
        {
            numJoints = other.numJoints;
            frameRate = other.frameRate;
            numAnimatedComponents = other.numAnimatedComponents;
            hierarchy = other.hierarchy;
            baseframe = other.baseframe;
        }
        if(!Compat(other))
            return false;

        other.NormalizedFrame(startFrame);
        other.NormalizedFrame(endFrame);

        if(startFrame > endFrame)
            return false;

        for(int i = startFrame; i <= endFrame; i++)
        {
            frame_t frame = other.frames[i];
            frame.index = this->numFrames;
            frames.Append(frame);

            idStr b = other.bounds.frames[i];
            bounds.frames.Append(b);

            this->numFrames++;
        }

        return true;
    }

    bool idMD5AnimFile::CutAnim(int startFrame, int endFrame)
    {
        NormalizedFrame(startFrame);
        NormalizedFrame(endFrame);

        if(startFrame == 0 && endFrame == numFrames - 1)
            return true;
        if(startFrame > endFrame)
            return false;

        int diff = endFrame - startFrame + 1;

        idList<frame_t> target;
        bounds_t b;
        target.SetGranularity(1);
        target.SetNum(diff);
        b.frames.SetGranularity(1);
        b.frames.SetNum(diff);
        for(int i = startFrame; i <= endFrame; i++)
        {
            int index = i - startFrame;
            target[index] = frames[i];
            target[index].index = index;
            b.frames[i - startFrame] = bounds.frames[i];
        }

        this->frames = target;
        this->bounds = b;
        this->numFrames = diff;

        return Check();
    }

    bool idMD5AnimFile::ReverseAnim(int startFrame, int endFrame)
    {
        NormalizedFrame(startFrame);
        NormalizedFrame(endFrame);

        if(startFrame >= endFrame)
            return false;

        int diff = endFrame - startFrame + 1;
        diff /= 2;

        for(int i = 0; i < diff; i++)
        {
            int a = startFrame + i;
            int b = endFrame - i;
            int indexA = frames[a].index;
            int indexB = frames[b].index;
            frame_t f = frames[a];
            frames[a] = frames[b];
            frames[b] = f;
            frames[a].index = indexA;
            frames[b].index = indexB;
            idStr bo = bounds.frames[a];
            bounds.frames[a] = bounds.frames[b];
            bounds.frames[b] = bo;
        }

        return Check();
    }

    bool idMD5AnimFile::LoopAnim(int startFrame, int endFrame)
    {
        NormalizedFrame(startFrame);
        NormalizedFrame(endFrame);

        if(startFrame >= endFrame)
            return false;

        int num = (endFrame - startFrame + 1) * 2 - 1;

        idList<frame_t> target;
        bounds_t b;
        target.SetGranularity(1);
        target.SetNum(num);
        b.frames.SetGranularity(1);
        b.frames.SetNum(num);
        int index = 0;
        // [ startFrame -> endFrame ]
        for(int i = startFrame; i <= endFrame; i++)
        {
            target[index] = frames[i];
            target[index].index = index;
            b.frames[index] = bounds.frames[i];
            index++;
        }
        // ( endFrame -> startFrame ]
        for(int i = endFrame - 1; i >= startFrame; i--)
        {
            target[index] = frames[i];
            target[index].index = index;
            b.frames[index] = bounds.frames[i];
            index++;
        }

        this->frames = target;
        this->bounds = b;
        this->numFrames = num;

        return Check();
    }

    idStr idMD5AnimFile::ToString(void) const
    {
        idStr str;

        str.Append(va("MD5Version %d \n", MD5_VERSION));
        str.Append("commandline \"\"\n");
        str.Append("\n");
        str.Append(va("numFrames %d \n", numFrames));
        str.Append(va("numJoints %d \n", numJoints));
        str.Append(va("frameRate %d \n", frameRate));
        str.Append(va("numAnimatedComponents %d \n", numAnimatedComponents));
        str.Append("\n");
        str.Append(hierarchy.ToString());
        str.Append("\n");
        str.Append(bounds.ToString());
        str.Append("\n");
        str.Append(baseframe.ToString());
        str.Append("\n");
        for(int i = 0; i < frames.Num(); i++)
        {
            str.Append(frames[i].ToString());
            str.Append("\n");
        }

        return str;
    }

    bool baseframe_s::Parse(idLexer &parser, int numJoints)
    {
        idStr str;

        transforms.SetGranularity(1);
        transforms.SetNum(numJoints);

        parser.ExpectTokenString("baseframe");
        parser.ExpectTokenString("{");

        for (int i = 0; i < numJoints; i++) {
            idStr text = ReadLine(parser);
            if(text.IsEmpty())
                return false;
            transforms[i] = text;
        }

        parser.ExpectTokenString("}");

        return !parser.HadError();
    }

    idStr baseframe_s::ToString(void) const
    {
        idStr str;

        str.Append("baseframe {\n");
        for(int i = 0; i < transforms.Num(); i++)
        {
            str.Append("  ");
            str.Append(transforms[i]);
            str.Append("\n");
        }
        str.Append("}\n");

        return str;
    }

    bool frame_s::Parse(idLexer &parser, int numJoints)
    {
        idStr str;

        transforms.SetGranularity(1);
        transforms.SetNum(numJoints);

        parser.ExpectTokenString("frame");
        index = parser.ParseInt();
        parser.ExpectTokenString("{");

        for (int i = 0; i < numJoints; i++) {
            idStr text = ReadLine(parser);
            if(text.IsEmpty())
                return false;
            transforms[i] = text;
        }

        parser.ExpectTokenString("}");

        return !parser.HadError();
    }

    idStr frame_s::ToString(void) const
    {
        idStr str;

        str.Append(va("frame %d {\n", index));
        for(int i = 0; i < transforms.Num(); i++)
        {
            str.Append("  ");
            str.Append(transforms[i]);
            str.Append("\n");
        }
        str.Append("}\n");

        return str;
    }

    bool hierarchy_s::Parse(idLexer &parser, int numJoints)
    {
        idStr str;

        joints.SetGranularity(1);
        joints.SetNum(numJoints);

        parser.ExpectTokenString("hierarchy");
        parser.ExpectTokenString("{");

        for (int i = 0; i < numJoints; i++) {
            idStr text = ReadLine(parser);
            if(text.IsEmpty())
                return false;
            joints[i] = text;
        }

        parser.ExpectTokenString("}");

        return !parser.HadError();
    }

    idStr hierarchy_s::ToString(void) const
    {
        idStr str;

        str.Append("hierarchy {\n");
        for(int i = 0; i < joints.Num(); i++)
        {
            str.Append("  ");
            str.Append(joints[i]);
            str.Append("\n");
        }
        str.Append("}\n");

        return str;
    }

    bool bounds_s::Parse(idLexer &parser, int numFrames)
    {
        idStr str;

        parser.ExpectTokenString("bounds");
        parser.ExpectTokenString("{");
        frames.SetGranularity(1);
        frames.SetNum(numFrames);

        for (int i = 0; i < numFrames; i++) {
            idStr text = ReadLine(parser);
            if(text.IsEmpty())
                return false;
            frames[i] = text;
        }

        parser.ExpectTokenString("}");

        return !parser.HadError();
    }

    idStr bounds_s::ToString(void) const
    {
        idStr str;

        str.Append("bounds {\n");
        for(int i = 0; i < frames.Num(); i++)
        {
            str.Append("  ");
            str.Append(frames[i]);
            str.Append("\n");
        }
        str.Append("}\n");

        return str;
    }

    static void R_CutAnim_f(const idCmdArgs &args)
    {
        if (args.Argc() < 4)
        {
            common->Printf("Usage: cutAnim <anim_file> <start_frame> <end_frame>");
            return;
        }

        md5anim::idMD5AnimFile file;
        const char *filename = args.Argv(1);
        if(!file.LoadAnim(filename))
        {
            common->Warning("Load md5anim file error: %s", filename);
            return;
        }

        const char *start = args.Argv(2);
        const char *end = args.Argv(3);
        if(!file.CutAnim(atoi(start), atoi(end)))
        {
            common->Warning("Cut md5anim error: %s(%s - %s)", filename, start, end);
            return;
        }

        idStr text = file.ToString();
        idStr basePath;

        idStr targetPath;
        if(!basePath.IsEmpty())
        {
            targetPath = basePath;
            targetPath.AppendPath(filename);
        }
        else
        {
            targetPath = filename;
            targetPath.StripFileExtension();
            targetPath.Append(va("_cut_%s_%s", start, end));
            targetPath.Append(".md5anim");
        }
        idFile *f = fileSystem->OpenFileWrite(targetPath.c_str(), "fs_savepath");
        md5anim::WriteText(text, f);
        fileSystem->CloseFile(f);

        common->Printf("Target md5anim save to %s(frames: %d)\n", targetPath.c_str(), file.NumFrames());
    }

    static void R_ReverseAnim_f(const idCmdArgs &args)
    {
        if (args.Argc() < 2)
        {
            common->Printf("Usage: reverseAnim <anim_file> [<start_frame> <end_frame>]");
            return;
        }

        md5anim::idMD5AnimFile file;
        const char *filename = args.Argv(1);
        if(!file.LoadAnim(filename))
        {
            common->Warning("Load md5anim file error: %s", filename);
            return;
        }

        int startFrame = 0;
        int endFrame = -1;
        if(args.Argc() > 2)
            startFrame = atoi(args.Argv(2));
        if(args.Argc() > 3)
            endFrame = atoi(args.Argv(3));
        if(!file.ReverseAnim(startFrame, endFrame))
        {
            common->Warning("Reverse md5anim error: %s(%d - %d)", filename, startFrame, endFrame);
            return;
        }

        idStr text = file.ToString();
        idStr basePath;

        idStr targetPath;
        if(!basePath.IsEmpty())
        {
            targetPath = basePath;
            targetPath.AppendPath(filename);
        }
        else
        {
            targetPath = filename;
            targetPath.StripFileExtension();
            targetPath.Append("_reverse");
            if(args.Argc() > 2)
                targetPath.Append(va("_%d", startFrame));
            if(args.Argc() > 3)
                targetPath.Append(va("_%d", endFrame));
            targetPath.Append(".md5anim");
        }
        idFile *f = fileSystem->OpenFileWrite(targetPath.c_str(), "fs_savepath");
        md5anim::WriteText(text, f);
        fileSystem->CloseFile(f);

        common->Printf("Target md5anim save to %s(frames: %d)\n", targetPath.c_str(), file.NumFrames());
    }

    static void R_LoopAnim_f(const idCmdArgs &args)
    {
        if (args.Argc() < 2)
        {
            common->Printf("Usage: loopAnim <anim_file> [<start_frame> <end_frame>]");
            return;
        }

        md5anim::idMD5AnimFile file;
        const char *filename = args.Argv(1);
        if(!file.LoadAnim(filename))
        {
            common->Warning("Load md5anim file error: %s", filename);
            return;
        }

        int startFrame = 0;
        int endFrame = -1;
        if(args.Argc() > 2)
            startFrame = atoi(args.Argv(2));
        if(args.Argc() > 3)
            endFrame = atoi(args.Argv(3));
        if(!file.LoopAnim(startFrame, endFrame))
        {
            common->Warning("Loop md5anim error: %s(%d - %d)", filename, startFrame, endFrame);
            return;
        }

        idStr text = file.ToString();
        idStr basePath;

        idStr targetPath;
        if(!basePath.IsEmpty())
        {
            targetPath = basePath;
            targetPath.AppendPath(filename);
        }
        else
        {
            targetPath = filename;
            targetPath.StripFileExtension();
            targetPath.Append("_loop");
            if(args.Argc() > 2)
                targetPath.Append(va("_%d", startFrame));
            if(args.Argc() > 3)
                targetPath.Append(va("_%d", endFrame));
            targetPath.Append(".md5anim");
        }
        idFile *f = fileSystem->OpenFileWrite(targetPath.c_str(), "fs_savepath");
        md5anim::WriteText(text, f);
        fileSystem->CloseFile(f);

        common->Printf("Target md5anim save to %s(frames: %d)\n", targetPath.c_str(), file.NumFrames());
    }

    struct animPart_s
    {
        idStr file_name;
        int start;
        int end;
    };
    struct animLoad_s
    {
        idStr file_name;
        md5anim::idMD5AnimFile anim;
    };

    static void R_LinkAnim_f(const idCmdArgs &args)
    {
        if (args.Argc() < 5)
        {
            common->Printf("Usage: cutAnim <new_anim_file> [-a <anim_file> [-f] <start_frame> <end_frame> ......]");
            return;
        }

#define PARSE_SHORT_ARG(what) \
		if(argType != 1) \
		{ \
			common->Warning("Required short argument"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{                           \
            readNum = -1; \
            common->Warning("Missing short argument `" what "` value"); \
			break; \
		} \
		arg = args.Argv(i);

        idList<animPart_s> parts;
        idList<animLoad_s> loadeds;

        animPart_s part;
        bool partSeted = false;
        int readNum = -1;
        for(int i = 2; i < args.Argc(); i++)
        {
            idStr arg = args.Argv(i);
            int argType = 0;
            if(!arg.Cmp("-"))
            {
                i++;
                if(i >= args.Argc())
                {
                    common->Warning("Missing short argument");
                    break;
                }
                arg = args.Argv(i);
                argType = 1;
            }

            if(argType == 1)
            {
                if(partSeted)
                {
                    parts.Append(part);
                    part.start = 0;
                    part.end = -1;
                    readNum = 0;
                }

                if(arg == "a")
                {
                    PARSE_SHORT_ARG("a");
                    part.file_name = arg;
                    part.start = 0;
                    part.end = -1;
                    partSeted = true;
                    readNum = 0;
                }
                else if(arg == "f")
                {
                    PARSE_SHORT_ARG("f");
                    if(!partSeted)
                    {
                        common->Warning("Missing animation file");
                        continue;
                    }
                    part.start = part.end = atoi(arg.c_str());
                    readNum = 1;
                }
                else
                {
                    readNum = -1;
                    partSeted = false;
                    common->Warning("Missing short argument");
                    continue;
                }
            }
            else
            {
                if(!partSeted)
                {
                    common->Warning("Missing animation file");
                    continue;
                }
                if(readNum == 0)
                {
                    part.start = part.end = atoi(arg.c_str());
                    readNum++;
                }
                else if(readNum == 1)
                {
                    part.end = atoi(arg.c_str());
                    readNum++;
                }
                else
                {
                    readNum = -1;
                    partSeted = false;
                    common->Warning("More frame index: %s", arg.c_str());
                    continue;
                }
            }
        }
        if(readNum >= 0)
            parts.Append(part);

        for(int i = 0; i < parts.Num(); i++)
        {
            const animPart_s &part = parts[i];
            common->Printf("Link %d: %s(%d - %d)\n", i, part.file_name.c_str(), part.start, part.end);

            // check need cache
            bool exists = false;
            for(int m = 0; m < loadeds.Num(); m++)
            {
                if(loadeds[i].file_name == part.file_name)
                {
                    exists = true;
                    break;
                }
            }
            // cache
            if(!exists)
            {
                animLoad_s loaded;
                if(!loaded.anim.LoadAnim(part.file_name))
                {
                    common->Warning("Load md5anim file error: %s", part.file_name.c_str());
                    return;
                }
                loaded.file_name = part.file_name;
                loadeds.Append(loaded);
            }
        }

        md5anim::idMD5AnimFile file;
        for(int i = 0; i < parts.Num(); i++)
        {
            const animPart_s &part = parts[i];
            const md5anim::idMD5AnimFile *f = NULL;
            for(int m = 0; m < loadeds.Num(); m++)
            {
                if(loadeds[i].file_name == part.file_name)
                {
                    f = &loadeds[i].anim;
                    break;
                }
            }
            if(!file.AppendAnim(*f, part.start, part.end))
            {
                common->Warning("Append md5anim error: %d - %s(%d - %d)", i, part.file_name.c_str(), part.start, part.end);
                return;
            }
        }

        idStr text = file.ToString();
        idStr basePath;
        idStr filename = args.Argv(1);
        if(filename.Length() < strlen(".md5anim") || idStr::Icmp(filename.Right(strlen(".md5anim")), ".md5anim"))
            filename.Append(".md5anim");

        idStr targetPath;
        if(!basePath.IsEmpty())
        {
            targetPath = basePath;
            targetPath.AppendPath(filename);
        }
        else
        {
            targetPath = filename;
        }
        idFile *f = fileSystem->OpenFileWrite(targetPath.c_str(), "fs_savepath");
        md5anim::WriteText(text, f);
        fileSystem->CloseFile(f);

        common->Printf("Target md5anim save to %s(frames: %d)\n", targetPath.c_str(), file.NumFrames());

#undef PARSE_SHORT_ARG
    }

    static void ArgCompletion_AnimName(const idCmdArgs &args, void(*callback)(const char *s))
    {
        cmdSystem->ArgCompletion_FolderExtension(args, callback, "models/", false, ".md5anim", NULL);
    }
}

void MD5Anim_AddCommand(void)
{
    using namespace md5anim;
    cmdSystem->AddCommand("cutAnim", R_CutAnim_f, CMD_FL_RENDERER, "cut md5 anim", ArgCompletion_AnimName);
    cmdSystem->AddCommand("reverseAnim", R_ReverseAnim_f, CMD_FL_RENDERER, "reverse md5 anim", ArgCompletion_AnimName);
    cmdSystem->AddCommand("linkAnim", R_LinkAnim_f, CMD_FL_RENDERER, "link md5 anim", ArgCompletion_AnimName);
    cmdSystem->AddCommand("loopAnim", R_LoopAnim_f, CMD_FL_RENDERER, "loop md5 anim", ArgCompletion_AnimName);
}
