namespace md5edit
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

    class idMD5AnimSourceFile
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
        idMD5AnimSourceFile();
    
        bool					LoadAnim(const char *filename);
        bool AppendAnim(const idMD5AnimSourceFile &other, int startFrame = 0, int endFrame = -1);
        bool CutAnim(int startFrame = 0, int endFrame = -1);
        bool ReverseAnim(int startFrame = 0, int endFrame = -1);
        bool LoopAnim(int startFrame = 0, int endFrame = -1);
        idStr ToString(void) const;
        int NumFrames() const {
            return numFrames;
        }
    
    private:
        bool Check(void) const;
        bool Compat(const idMD5AnimSourceFile &other) const;
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

    idMD5AnimSourceFile::idMD5AnimSourceFile()
    : numFrames(0),
    numJoints(0),
    frameRate(0),
    numAnimatedComponents(0)
    {
    }

    void idMD5AnimSourceFile::NormalizedFrame(int &frame) const
    {
        if(frame < 0)
            frame = numFrames + frame;
        if(frame < 0)
            frame = 0;
        else if(frame >= numFrames)
            frame = numFrames - 1;
    }

    bool idMD5AnimSourceFile::LoadAnim(const char *filename)
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

    bool idMD5AnimSourceFile::Check(void) const
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

    bool idMD5AnimSourceFile::Compat(const idMD5AnimSourceFile &other) const
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

    bool idMD5AnimSourceFile::AppendAnim(const idMD5AnimSourceFile &other, int startFrame, int endFrame)
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

    bool idMD5AnimSourceFile::CutAnim(int startFrame, int endFrame)
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

    bool idMD5AnimSourceFile::ReverseAnim(int startFrame, int endFrame)
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

    bool idMD5AnimSourceFile::LoopAnim(int startFrame, int endFrame)
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

    idStr idMD5AnimSourceFile::ToString(void) const
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

    typedef struct editAnim_s
    {
        int cmd;
        idStr animFile;
        int startFrame;
        int endFrame;
    } editAnim_t;

    class idAnimEditor
    {
    public:
        idAnimEditor(void);
        virtual ~idAnimEditor(void);
        bool Parse(const idCmdArgs &args);
        bool Handle(void);

    private:
        enum {
            CMD_INVALID = 0,
            CMD_CUT = 1,
            CMD_REVERSE,
            CMD_LOOP,
            CMD_LINK,
            CMD_OUT,
        };
        int ParseCmd(const idCmdArgs &args, int &start);
        bool ParseCut(const idCmdArgs &args, int &start);
        bool ParseReverse(const idCmdArgs &args, int &start);
        bool ParseLoop(const idCmdArgs &args, int &start);
        bool ParseLink(const idCmdArgs &args, int &start);
        bool ParseOut(const idCmdArgs &args, int &start);

        bool HandleCut(const editAnim_t &edit);
        bool HandleReverse(const editAnim_t &edit);
        bool HandleLoop(const editAnim_t &edit);
        bool HandleLink(const editAnim_t &edit);
        bool HandleOut(const editAnim_t &edit);

        bool CheckAnimFile(const char *str) const;
        void PrintEdit(const editAnim_t &edit) const;
        int AddEdit(const editAnim_t &edit);
        bool IsNumeric(const char *token) const;

    private:
        idStr animFile;
        idList<editAnim_t> cmds;

        idMD5AnimSourceFile *md5edit;
        int currentIndex;
    };

    idAnimEditor::idAnimEditor(void)
    : md5edit(NULL),
      currentIndex(-1)
    {
    }

    idAnimEditor::~idAnimEditor(void)
    {
        delete md5edit;
    }

    int idAnimEditor::ParseCmd(const idCmdArgs &args, int &start)
    {
        const char *token = args.Argv(start);
        if(idStr::Cmp(token, "-"))
        {
            common->Warning("Require '-' token, but read '%s'!", token);
            return -1;
        }
        if(start + 1 >= args.Argc())
        {
            common->Warning("Missing argument token!");
            return -2;
        }
        token = args.Argv(start + 1);
        int cmd = CMD_INVALID;
        if(!idStr::Icmp("cut", token) || !idStr::Icmp("c", token))
        {
            cmd = CMD_CUT;
        }
        else if(!idStr::Icmp("reverse", token) || !idStr::Icmp("r", token))
        {
            cmd = CMD_REVERSE;
        }
        else if(!idStr::Icmp("loop", token) || !idStr::Icmp("p", token))
        {
            cmd = CMD_LOOP;
        }
        else if(!idStr::Icmp("link", token) || !idStr::Icmp("l", token))
        {
            cmd = CMD_LINK;
        }
        else if(!idStr::Icmp("output", token) || !idStr::Icmp("o", token))
        {
            cmd = CMD_OUT;
        }

        if(cmd == CMD_INVALID)
        {
            common->Warning("Invalid command token '%s'!", token);
            return CMD_INVALID;
        }

        start += 2;
        return cmd;
    }

    bool idAnimEditor::ParseCut(const idCmdArgs &args, int &start)
    {
        editAnim_t edit;
        edit.cmd = CMD_CUT;
        const char *token = args.Argv(start);

        if(!IsNumeric(token))
        {
            common->Warning("Cut command require number for start frame, but read '%s'!", token);
            return false;
        }
        edit.startFrame = atoi(token);

        token = args.Argv(start + 1);
        if(!IsNumeric(token))
        {
            common->Warning("Cut command require number for end frame, but read '%s'!", token);
            return false;
        }
        edit.endFrame = atoi(token);

        start += 2;
        AddEdit(edit);
        return true;
    }

    bool idAnimEditor::ParseReverse(const idCmdArgs &args, int &start)
    {
        editAnim_t edit;
        edit.cmd = CMD_REVERSE;
        edit.startFrame = 0;
        edit.endFrame = -1;
        const char *token = args.Argv(start);

        if(!IsNumeric(token))
        {
            AddEdit(edit);
            return true;
        }
        edit.startFrame = atoi(token);

        token = args.Argv(start + 1);
        if(!IsNumeric(token))
        {
            start++;
            AddEdit(edit);
            return true;
        }
        edit.endFrame = atoi(token);

        start += 2;
        AddEdit(edit);
        return true;
    }

    bool idAnimEditor::ParseLoop(const idCmdArgs &args, int &start)
    {
        editAnim_t edit;
        edit.cmd = CMD_LOOP;
        edit.startFrame = 0;
        edit.endFrame = -1;
        const char *token = args.Argv(start);

        if(!IsNumeric(token))
        {
            AddEdit(edit);
            return true;
        }
        edit.startFrame = atoi(token);

        token = args.Argv(start + 1);
        if(!IsNumeric(token))
        {
            start++;
            AddEdit(edit);
            return true;
        }
        edit.endFrame = atoi(token);

        start += 2;
        AddEdit(edit);
        return true;
    }

    bool idAnimEditor::ParseLink(const idCmdArgs &args, int &start)
    {
        editAnim_t edit;
        edit.cmd = CMD_LINK;
        edit.startFrame = 0;
        edit.endFrame = -1;
        const char *token = args.Argv(start);

        if(!idStr::Cmp("-", token))
        {
            common->Warning("Link command require anim file, but read '%s'!", token);
            return false;
        }
        if(!CheckAnimFile(token))
        {
            common->Warning("Link command anim file not available '%s'!", token);
            return false;
        }
        edit.animFile = token;

        token = args.Argv(start + 1);
        if(!IsNumeric(token))
        {
            start++;
            AddEdit(edit);
            return true;
        }
        edit.startFrame = atoi(token);

        token = args.Argv(start + 2);
        if(!IsNumeric(token))
        {
            start++;
            AddEdit(edit);
            return true;
        }
        edit.endFrame = atoi(token);

        start += 3;
        AddEdit(edit);
        return true;
    }

    bool idAnimEditor::ParseOut(const idCmdArgs &args, int &start)
    {
        editAnim_t edit;
        edit.cmd = CMD_OUT;
        edit.startFrame = 0;
        edit.endFrame = -1;
        const char *token = args.Argv(start);

        if(!idStr::Cmp("-", token))
        {
            common->Warning("Output command require anim file, but read '%s'!", token);
            return false;
        }
        edit.animFile = token;

        if(!IsNumeric(token + 1))
        {
            start++;
            AddEdit(edit);
            return true;
        }
        edit.startFrame = atoi(token);

        token = args.Argv(start + 2);
        if(!IsNumeric(token))
        {
            start++;
            AddEdit(edit);
            return true;
        }
        edit.endFrame = atoi(token);

        start += 3;
        AddEdit(edit);
        return true;
    }

    bool idAnimEditor::CheckAnimFile(const char *str) const
    {
        idMD5AnimSourceFile file;
        return file.LoadAnim(str);
    }

    int idAnimEditor::AddEdit(const editAnim_t &edit)
    {
        currentIndex = cmds.Num() + 1;
        PrintEdit(edit);
        return cmds.Append(edit);
    }

    bool idAnimEditor::IsNumeric(const char *token) const
    {
        return(idStr::Cmp("-", token) && idStr::IsNumeric(token));
    }

    void idAnimEditor::PrintEdit(const editAnim_t &edit) const
    {
        idStr str;
        switch (edit.cmd) {
            case CMD_CUT:
                str.Append("Cut");
                break;
            case CMD_REVERSE:
                str.Append("Reverse");
                break;
            case CMD_LOOP:
                str.Append("Loop");
                break;
            case CMD_LINK:
                str.Append("Link");
                break;
            case CMD_OUT:
                str.Append("Output");
                break;
            default:
                return;
        }
        str.Append(" ");
        if(edit.cmd == CMD_LINK || edit.cmd == CMD_OUT)
        {
            str.Append("-> ");
            str.Append(edit.animFile);
            str.Append(" ");
        }
        str.Append(va("(%d, %d)", edit.startFrame, edit.endFrame));
        common->Printf("%2d: %s\n", currentIndex, str.c_str());
    }

    bool idAnimEditor::Parse(const idCmdArgs &args)
    {
        if(args.Argc() < 3)
        {
            common->Printf("[Usage]: %s <animation file> [-cut|c <start frame> <end frame>] [-reverse|r [<start frame> [<end frame>]]] [-loop|p [<start frame> [<end frame>]]] [-link|l <other animation file> [<start frame> [<end frame>]]] [-output|o <output animation file path> [<start frame> [<end frame>]]]\n", args.Argv(0));
            return false;
        }

        common->Printf("[idAnimEditor]: Parsing arguments(%d)\n", args.Argc());
        const char *token = args.Argv(1);
        if(!CheckAnimFile(token))
        {
            common->Warning("Input animation file not available '%s'!", token);
            return false;
        }

        animFile = token;

        bool hasOut = false;
        idStr lastOut;
        int start = 2;
        while(start < args.Argc())
        {
            int cmd = ParseCmd(args, start);

            switch (cmd) {
                case CMD_CUT:
                    if(!ParseCut(args, start))
                        return false;
                    break;
                case CMD_REVERSE:
                    if(!ParseReverse(args, start))
                        return false;
                    break;
                case CMD_LOOP:
                    if(!ParseLoop(args, start))
                        return false;
                    break;
                case CMD_LINK:
                    if(!ParseLink(args, start))
                        return false;
                    break;
                case CMD_OUT:
                    if(!ParseOut(args, start))
                        return false;
                    hasOut = true;
                    lastOut = cmds[cmds.Num() - 1].animFile;
                    break;
                default:
                    return false;
            }
        }
        if(cmds.Num() == 0)
        {
            common->Warning("Require 1 command!\n");
            return false;
        }
        if(!hasOut)
        {
            common->Warning("Require 1 output command!\n");
            return false;
        }

        // make sure last is output command
        if(cmds[cmds.Num() - 1].cmd != CMD_OUT)
        {
            editAnim_t edit;
            edit.cmd = CMD_OUT;
            edit.startFrame = 0;
            edit.endFrame = -1;
            edit.animFile = lastOut;
            AddEdit(edit);
        }

        common->Printf("[idAnimEditor]: Finish parse arguments(%s %d)\n", animFile.c_str(), cmds.Num());
        return true;
    }

    bool idAnimEditor::Handle(void)
    {
        common->Printf("[idAnimEditor]: Handling commands(%s %d)\n", animFile.c_str(), cmds.Num());
        delete md5edit;

        md5edit = new idMD5AnimSourceFile;
        md5edit->LoadAnim(animFile);
        common->Printf("[idAnimEditor]: Input animation (%d frames)\n", md5edit->NumFrames());
        currentIndex = 0;

        for(int i = 0; i < cmds.Num(); i++)
        {
            const editAnim_t &edit = cmds[i];
            currentIndex++;
            switch (edit.cmd)
            {
                case CMD_CUT:
                    if(!HandleCut(edit))
                        return false;
                    break;
                case CMD_REVERSE:
                    if(!HandleReverse(edit))
                        return false;
                    break;
                case CMD_LOOP:
                    if(!HandleLoop(edit))
                        return false;
                    break;
                case CMD_LINK:
                    if(!HandleLink(edit))
                        return false;
                    break;
                case CMD_OUT:
                    if(!HandleOut(edit))
                        return false;
                    break;
                default:
                    return false;
            }
        }

        common->Printf("[idAnimEditor]: Finish handle commands(%s %d)\n", animFile.c_str(), cmds.Num());
        return true;
    }

    bool idAnimEditor::HandleCut(const editAnim_t &edit)
    {
        if(!md5edit->CutAnim(edit.startFrame, edit.endFrame))
        {
            common->Warning("%2d. Cut md5edit error: (%d - %d)", currentIndex, edit.startFrame, edit.endFrame);
            return false;
        }

        common->Printf("%2d. Cut md5edit: (%d - %d)\n", currentIndex, edit.startFrame, edit.endFrame);
        return true;
    }

    bool idAnimEditor::HandleReverse(const md5edit::editAnim_t &edit)
    {
        if(!md5edit->ReverseAnim(edit.startFrame, edit.endFrame))
        {
            common->Warning("%2d. Reverse md5edit error: (%d - %d)", currentIndex, edit.startFrame, edit.endFrame);
            return false;
        }

        common->Printf("%2d. Reverse md5edit: (%d - %d)\n", currentIndex, edit.startFrame, edit.endFrame);
        return true;
    }

    bool idAnimEditor::HandleLoop(const md5edit::editAnim_t &edit)
    {
        if(!md5edit->LoopAnim(edit.startFrame, edit.endFrame))
        {
            common->Warning("%2d. Loop md5edit error: (%d - %d)", currentIndex, edit.startFrame, edit.endFrame);
            return false;
        }

        common->Printf("%2d. Loop md5edit: (%d - %d)\n", currentIndex, edit.startFrame, edit.endFrame);
        return true;
    }

    bool idAnimEditor::HandleLink(const md5edit::editAnim_t &edit)
    {
        md5edit::idMD5AnimSourceFile file;

        file.LoadAnim(edit.animFile);
        if(!md5edit->AppendAnim(file, edit.startFrame, edit.endFrame))
        {
            common->Warning("%2d. Append md5edit error: %s (%d - %d)", currentIndex, edit.animFile.c_str(), edit.startFrame, edit.endFrame);
            return false;
        }

        common->Printf("%2d. Append md5edit: %s (%d - %d)\n", currentIndex, edit.animFile.c_str(), edit.startFrame, edit.endFrame);
        return true;
    }

    bool idAnimEditor::HandleOut(const md5edit::editAnim_t &edit)
    {
        idStr text = md5edit->ToString();
        idStr basePath;
        idStr filename = edit.animFile;
        if(filename.Length() < strlen(".md5edit") || idStr::Icmp(filename.Right(strlen(".md5edit")), ".md5edit"))
            filename.Append(".md5edit");

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
        md5edit::WriteText(text, f);
        fileSystem->CloseFile(f);

        common->Printf("%2d. Output md5edit save to '%s' (frames: %d)\n", currentIndex, targetPath.c_str(), md5edit->NumFrames());

        return true;
    }

    static void ArgCompletion_AnimName(const idCmdArgs &args, void(*callback)(const char *s))
    {
        cmdSystem->ArgCompletion_FolderExtension(args, callback, "models/", false, ".md5edit", NULL);
    }

    static void R_EditAnim_f(const idCmdArgs &args)
    {
        idAnimEditor editor;
        if(editor.Parse(args))
            editor.Handle();
    }

    static void R_CutAnim_f(const idCmdArgs &args)
    {
        if(args.Argc() < 5)
        {
            common->Printf("[Usage]: %s <output animation file path> <input animation file> <start frame> <end frame>", args.Argv(0));
            return;
        }
        idStr cmd;
        cmd.Append("editAnim ");
        cmd.Append(args.Argv(2));
        cmd.Append(" -cut ");
        for(int i = 3; i < args.Argc(); i++)
        {
            cmd.Append(args.Argv(i));
            cmd.Append(" ");
        }
        cmd.Append("-o ");
        idStr toPath = args.Argv(1);
        toPath.SetFileExtension(".md5edit");
        cmd.Append(toPath);
        common->Printf("cutAnim -> %s\n", cmd.c_str());
        cmdSystem->BufferCommandText(CMD_EXEC_NOW, cmd.c_str());
    }

    static void R_ReverseAnim_f(const idCmdArgs &args)
    {
        if(args.Argc() < 3)
        {
            common->Printf("[Usage]: %s <output animation file path> <input animation file> [<start frame> [<end frame>]]", args.Argv(0));
            return;
        }
        idStr cmd;
        cmd.Append("editAnim ");
        cmd.Append(args.Argv(2));
        cmd.Append(" -reverse ");
        for(int i = 3; i < args.Argc(); i++)
        {
            cmd.Append(args.Argv(i));
            cmd.Append(" ");
        }
        cmd.Append("-o ");
        idStr toPath = args.Argv(1);
        toPath.SetFileExtension(".md5edit");
        cmd.Append(toPath);
        common->Printf("reverseAnim -> %s\n", cmd.c_str());
        cmdSystem->BufferCommandText(CMD_EXEC_NOW, cmd.c_str());
    }

    static void R_LoopAnim_f(const idCmdArgs &args)
    {
        if(args.Argc() < 3)
        {
            common->Printf("[Usage]: %s <output animation file path> <input animation file> [<start frame> [<end frame>]]", args.Argv(0));
            return;
        }
        idStr cmd;
        cmd.Append("editAnim ");
        cmd.Append(args.Argv(2));
        cmd.Append(" -loop ");
        for(int i = 3; i < args.Argc(); i++)
        {
            cmd.Append(args.Argv(i));
            cmd.Append(" ");
        }
        cmd.Append("-o ");
        idStr toPath = args.Argv(1);
        toPath.SetFileExtension(".md5edit");
        cmd.Append(toPath);
        common->Printf("loopAnim -> %s\n", cmd.c_str());
        cmdSystem->BufferCommandText(CMD_EXEC_NOW, cmd.c_str());
    }

    static void R_LinkAnim_f(const idCmdArgs &args)
    {
        if(args.Argc() < 4)
        {
            common->Printf("[Usage]: %s <output animation file path> <input animation file> <append animation file> [<start frame> [<end frame>]]", args.Argv(0));
            return;
        }
        idStr cmd;
        cmd.Append("editAnim ");
        cmd.Append(args.Argv(2));
        cmd.Append(" -link ");
        for(int i = 3; i < args.Argc(); i++)
        {
            cmd.Append(args.Argv(i));
            cmd.Append(" ");
        }
        cmd.Append("-o ");
        idStr toPath = args.Argv(1);
        toPath.SetFileExtension(".md5edit");
        cmd.Append(toPath);
        common->Printf("linkAnim -> %s\n", cmd.c_str());
        cmdSystem->BufferCommandText(CMD_EXEC_NOW, cmd.c_str());
    }
}

void MD5Edit_AddCommand(void)
{
    using namespace md5edit;
    cmdSystem->AddCommand("editAnim", R_EditAnim_f, CMD_FL_RENDERER, "Edit md5 animation file", ArgCompletion_AnimName);
    cmdSystem->AddCommand("cutAnim", R_CutAnim_f, CMD_FL_RENDERER, "cut md5 anim", ArgCompletion_AnimName);
    cmdSystem->AddCommand("reverseAnim", R_ReverseAnim_f, CMD_FL_RENDERER, "reverse md5 anim", ArgCompletion_AnimName);
    cmdSystem->AddCommand("linkAnim", R_LinkAnim_f, CMD_FL_RENDERER, "link md5 anim", ArgCompletion_AnimName);
    cmdSystem->AddCommand("loopAnim", R_LoopAnim_f, CMD_FL_RENDERER, "loop md5 anim", ArgCompletion_AnimName);
}
