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
        idList<idStr> bounds;

        idStr ToString(void) const;
        bool Parse(idLexer &parser, int numFrames);
    } bounds_t;

    class file
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
        file();
    
        bool					LoadAnim(const char *filename);
        bool AppendAnim(const file &other, int startFrame = 0, int endFrame = -1);
        bool CutAnim(int startFrame = 0, int endFrame = -1);
        idStr ToString(void) const;
    
    private:
        bool Check(void) const;
        bool Compat(const file &other) const;

        friend void R_CutAnim_f(const idCmdArgs &args);
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

    file::file()
    : numFrames(0),
    numJoints(0),
    frameRate(0),
    numAnimatedComponents(0)
    {
    }

    bool file::LoadAnim(const char *filename)
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

    bool file::Check(void) const
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

        if(bounds.bounds.Num() != numFrames)
        {
            common->Warning("bounds.Num() != numFrames: %d != %d", bounds.bounds.Num(), numFrames);
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

    bool file::Compat(const file &other) const
    {
        if(!other.Check())
            return false;

        if(numJoints != other.numJoints)
            return false;

        return true;
    }

    bool file::AppendAnim(const file &other, int startFrame, int endFrame)
    {
        if(numJoints == 0)
        {
            numJoints = other.numJoints;
            frameRate = other.frameRate;
            numAnimatedComponents = other.numAnimatedComponents;
        }
        if(Compat(other))
            return false;

        if(startFrame < 0)
            startFrame = 0;
        if(endFrame >= other.numFrames || endFrame < 0)
            endFrame = other.numFrames - 1;

        if(startFrame > endFrame)
            return false;

        for(int i = startFrame; i <= endFrame; i++)
        {
            frame_t frame = other.frames[i];
            frame.index = this->numFrames;
            frames.Append(frame);

            idStr b = other.bounds.bounds[i];
            bounds.bounds.Append(b);

            this->numFrames++;
        }

        return true;
    }

    bool file::CutAnim(int startFrame, int endFrame)
    {
        if(startFrame < 0)
            startFrame = 0;
        if(endFrame >= numFrames || endFrame < 0)
            endFrame = numFrames - 1;

        if(startFrame == 0 && endFrame == numFrames - 1)
            return true;
        if(startFrame > endFrame)
            return false;

        int diff = endFrame - startFrame + 1;

        idList<frame_t> target;
        bounds_t b;
        target.SetGranularity(1);
        target.SetNum(diff);
        b.bounds.SetGranularity(1);
        b.bounds.SetNum(diff);
        for(int i = startFrame; i <= endFrame; i++)
        {
            int index = i - startFrame;
            target[index] = frames[i];
            target[index].index = index;
            b.bounds[i - startFrame] = bounds.bounds[i];
        }

        this->frames = target;
        this->bounds = b;
        this->numFrames = diff;

        return Check();
    }

    idStr file::ToString(void) const
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
        bounds.SetGranularity(1);
        bounds.SetNum(numFrames);

        for (int i = 0; i < numFrames; i++) {
            idStr text = ReadLine(parser);
            if(text.IsEmpty())
                return false;
            bounds[i] = text;
        }

        parser.ExpectTokenString("}");

        return !parser.HadError();
    }

    idStr bounds_s::ToString(void) const
    {
        idStr str;

        str.Append("bounds {\n");
        for(int i = 0; i < bounds.Num(); i++)
        {
            str.Append("  ");
            str.Append(bounds[i]);
            str.Append("\n");
        }
        str.Append("}\n");

        return str;
    }

    void R_CutAnim_f(const idCmdArgs &args)
    {
        if (args.Argc() < 4)
        {
            common->Printf("Usage: cutAnim <anim_file> <start_frame> <end_frame>");
            return;
        }

        md5anim::file file;
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

        common->Printf("Target md5anim save to %s(frames: %d)\n", targetPath.c_str(), file.numFrames);
    }
}

void R_CutAnim_f(const idCmdArgs &args)
{
    md5anim::R_CutAnim_f(args);
}

void ArgCompletion_AnimName(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "models/", false, ".md5anim", NULL);
}