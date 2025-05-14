
namespace modeltest
{
    typedef struct renderModelEntityDefAnimFlag_s
    {
        idStr command;
        idStrList parms;
    } renderModelEntityDefAnimFlag_t;

    typedef struct renderModelEntityDefAnim_s
    {
        int index;
        idStr name;
        idStrList file;
        idList<renderModelEntityDefAnimFlag_t> flags;
    } renderModelEntityDefAnim_t;

    typedef struct renderModelEntityDefChannel_s
    {
        idStr name;
        idStr joints;
    } renderModelEntityDefChannel_t;

    class idRenderModelEntityDef
    {
    public:
        idRenderModelEntityDef();
        int							NumAnims(void) const {
            return anims.Num();
        }
        idStr                       GetAnimName(int index) const {
            return anims[index].name;
        }
        idStrList                   GetAnimFile(int index) const {
            return anims[index].file;
        }
        bool                        Parse(const char *name);
        idRenderModel *             MeshModel(void) const;
        void                        Print(void) const;
        void                        Clear(void) {
            MakeDefault();
        }
        bool                        IsValid(void) const;


    private:
        bool                    ParseAnim(idLexer &lexer);
        void                    MakeDefault(void);
        void                    CopyDecl(const idRenderModelEntityDef &decl);

    private:
        idStr 				        name;
        idStr 				        fileName;
        int 				        lineNum;
        idVec3						offset;
        idStr 				        mesh;
        idStr 				        skin;
        idList<renderModelEntityDefChannel_t> 		channels;
        idList<renderModelEntityDefAnim_t>			anims;
    };

    idRenderModelEntityDef::idRenderModelEntityDef(void)
    {
        offset.Zero();
        anims.SetGranularity(1);
        channels.SetGranularity(1);
    }

    void idRenderModelEntityDef::MakeDefault(void)
    {
        offset.Zero();
        anims.Clear();
        anims.SetGranularity(1);
        mesh.Clear();
        skin.Clear();
        name.Clear();
        fileName.Clear();
        lineNum = 0;
        channels.SetGranularity(1);
    }

    bool idRenderModelEntityDef::Parse(const char *name)
    {
        MakeDefault();
        const idDecl *decl = declManager->FindType(DECL_MODELDEF, name, false);
        if(!decl)
        {
            common->Warning("model '%s' not found!", name);
            return false;
        }

        int textLength = decl->GetTextLength();
        if(!textLength)
        {
            common->Warning("model '%s' no source!", name);
            return false;
        }

        char *text = (char *)malloc(textLength + 1);
        decl->GetText(text);
        text[textLength] = '\0';
        // using idStr for auto free
        idStr source = text;
        free(text);

        idLexer				src;
        idToken				token;
        idToken				token2;
        idStr				extension;
        idStr				filename;
        idRenderModel		*modelHandle;
        int					num;
        int					i;
        idStr				jointnames;

        fileName = decl->GetFileName();
        lineNum = decl->GetLineNum();

        src.LoadMemory(source.c_str(), textLength, decl->GetFileName(), decl->GetLineNum());
        src.SetFlags(DECL_LEXER_FLAGS);

        this->name = name;
        src.ReadToken(&token);
        if(token == "{")
            src.UnreadToken(&token);
        else
        {
            if(token != "model")
            {
                src.Warning("Not model decl: '%s'", token.c_str());
                return false;
            }
            src.ReadToken(&token);
            this->name = token;
        }

        src.SkipUntilString("{");

        while (1) {
            if (!src.ReadToken(&token)) {
                break;
            }

            if (!token.Icmp("}")) {
                break;
            }

            if (token == "inherit") {
                if (!src.ReadToken(&token2)) {
                    src.Warning("Unexpected end of file");
                    MakeDefault();
                    return false;
                }

                idRenderModelEntityDef copy;
                boolean res = copy.Parse(token2.c_str());

                if (!res) {
                    common->Warning("Unknown model definition '%s'", token2.c_str());
                } else {
                    CopyDecl(copy);
                }
            } else if (token == "skin") {
                if (!src.ReadToken(&token2)) {
                    src.Warning("Unexpected end of file");
                    MakeDefault();
                    return false;
                }

                skin = token2;

                if (!skin) {
                    src.Warning("Skin '%s' not found", token2.c_str());
                    MakeDefault();
                    return false;
                }
            } else if (token == "mesh") {
                if (!src.ReadToken(&token2)) {
                    src.Warning("Unexpected end of file");
                    MakeDefault();
                    return false;
                }

                filename = token2;
                filename.ExtractFileExtension(extension);

                if (extension != MD5_MESH_EXT) {
                    src.Warning("Invalid model for MD5 mesh");
                    MakeDefault();
                    return false;
                }

                modelHandle = renderModelManager->FindModel(filename);

                if (!modelHandle) {
                    src.Warning("Model '%s' not found", filename.c_str());
                    MakeDefault();
                    return false;
                }

                if (modelHandle->IsDefaultModel()) {
                    src.Warning("Model '%s' defaulted", filename.c_str());
                    MakeDefault();
                    return false;
                }

                mesh = filename;
            } else if (token == "remove") {
                // removes any anims whos name matches
                if (!src.ReadToken(&token2)) {
                    src.Warning("Unexpected end of file");
                    MakeDefault();
                    return false;
                }

                num = 0;

                for (i = 0; i < anims.Num(); i++) {
                    if (token2 == anims[ i ].name) {
                        anims.RemoveIndex(i);

                        i--;
                        num++;
                        continue;
                    }
                }

                if (!num) {
                    src.Warning("Couldn't find anim '%s' to remove", token2.c_str());
                    MakeDefault();
                    return false;
                }
            } else if (token == "anim") {
                if (!modelHandle) {
                    src.Warning("Must specify mesh before defining anims");
                    MakeDefault();
                    return false;
                }

                if (!ParseAnim(src)) {
                    MakeDefault();
                    return false;
                }
            } else if (token == "offset") {
                if (!src.Parse1DMatrix(3, offset.ToFloatPtr())) {
                    src.Warning("Expected vector following 'offset'");
                    MakeDefault();
                    return false;
                }
            } else if (token == "channel") {
                if (!modelHandle) {
                    src.Warning("Must specify mesh before defining channels");
                    MakeDefault();
                    return false;
                }

                // set the channel for a group of joints
                if (!src.ReadToken(&token2)) {
                    src.Warning("Unexpected end of file");
                    MakeDefault();
                    return false;
                }

                if (!src.CheckTokenString("(")) {
                    src.Warning("Expected { after '%s'\n", token2.c_str());
                    MakeDefault();
                    return false;
                }

                jointnames = "";
                renderModelEntityDefChannel_t chan;
                chan.name = token2;

                while (!src.CheckTokenString(")")) {
                    if (!src.ReadToken(&token2)) {
                        src.Warning("Unexpected end of file");
                        MakeDefault();
                        return false;
                    }

                    jointnames += token2;

                    if ((token2 != "*") && (token2 != "-")) {
                        jointnames += " ";
                    }
                }

                chan.joints = jointnames;
                channels.Append(chan);
            } else {
                src.Warning("unknown token '%s'", token.c_str());
                MakeDefault();
                return false;
            }
        }

        anims.SetGranularity(1);
        anims.SetNum(anims.Num());
        channels.SetGranularity(1);
        channels.SetNum(channels.Num());

        return true;
    }

    bool idRenderModelEntityDef::ParseAnim(idLexer &src)
    {
        int				i;
        idStr			alias;
        idToken			realname;
        idToken			token;
        int				numAnims;

        numAnims = 0;

        if (!src.ReadToken(&realname)) {
            src.Warning("Unexpected end of file");
            MakeDefault();
            return false;
        }

        alias = realname;

        for (i = 0; i < anims.Num(); i++) {
            if (!strcmp(anims[ i ].name, realname)) {
                break;
            }
        }

        if (i < anims.Num()) {
            src.Warning("Duplicate anim '%s'", realname.c_str());
            MakeDefault();
            return false;
        }

        renderModelEntityDefAnim_t anim;
        anim.name = alias;

        // parse the anims from the string
        do {
            if (!src.ReadToken(&token)) {
                src.Warning("Unexpected end of file");
                MakeDefault();
                return false;
            }

            // add it to our list
            anim.file.Append(token);
            numAnims++;
        } while (src.CheckTokenString(","));

        if (!numAnims) {
            src.Warning("No animation specified");
            MakeDefault();
            return false;
        }

        // parse any frame commands or animflags
        if (src.CheckTokenString("{")) {
            renderModelEntityDefAnimFlag_t flag;
            while (1) {
                if (!src.ReadToken(&token)) {
                    src.Warning("Unexpected end of file");
                    MakeDefault();
                    return false;
                }

                if (token == "}") {
                    anim.flags.Append(flag);
                    break;
                }

                flag.command = token;
                flag.parms.Clear();

                while (src.ReadTokenOnLine(&token)) {
                    if (token == "}") {
                        src.UnreadToken(&token);
                        break;
                    }
                    flag.parms.Append(token);
                }
            }
            flag.parms.SetGranularity(1);
            flag.parms.SetNum(flag.parms.Num());
        }

        // set the flags
        anim.flags.SetGranularity(1);
        anim.flags.SetNum(anim.flags.Num());
        anim.file.SetGranularity(1);
        anim.file.SetNum(anim.file.Num());

        anims.Append(anim);

        return true;
    }

    void idRenderModelEntityDef::CopyDecl(const idRenderModelEntityDef &decl)
    {
        int i;

        // MakeDefault();

        offset = decl.offset;
        mesh = decl.mesh;
        skin = decl.skin;

        anims.SetNum(decl.anims.Num());

        for (i = 0; i < anims.Num(); i++) {
            anims[ i ] = decl.anims[ i ];
        }

        for (i = 0; i < sizeof(channels) / sizeof(channels[0]); i++) {
            channels[i] = decl.channels[i];
        }
    }

    idRenderModel * idRenderModelEntityDef::MeshModel(void) const
    {
        idRenderModel *modelHandle = renderModelManager->FindModel(mesh);

        if (!modelHandle) {
            common->Warning("Model '%s' not found", mesh.c_str());
        }
        return modelHandle;
    }

    void idRenderModelEntityDef::Print(void) const
    {
        common->Printf("Model %s\n", name.c_str());
        common->Printf("  File: '%s'(%d)\n", fileName.c_str(), lineNum);
        common->Printf("  Mesh: %s\n", mesh.c_str());
        common->Printf("  Skin: %s\n", skin.c_str());
        common->Printf("  Offset: %s\n", offset.ToString(6));
        common->Printf("  Channels: %d\n", channels.Num());
        for(int i = 0; i < channels.Num(); i++)
        {
            common->Printf("    %d: %s -> %s\n", i, channels[i].name.c_str(), channels[i].joints.c_str());
        }
        common->Printf("  Animations: %d\n", anims.Num());
        for(int i = 0; i < anims.Num(); i++)
        {
            const renderModelEntityDefAnim_t &anim = anims[i];
            if(anim.file.Num() > 1)
            {
                common->Printf("    %d: %s -> %d\n", i, anim.name.c_str(), anim.file.Num());
                for(int m = 0; m < anims[i].file.Num(); m++)
                {
                    common->Printf("      %d: %s\n", m, anim.file[m].c_str());
                }
            }
            else
            {
                common->Printf("    %d: %s -> %s\n", i, anim.name.c_str(), anim.file[0].c_str());
            }
            common->Printf("    Flags: %d\n", anim.flags.Num());
            for(int m = 0; m < anim.flags.Num(); m++)
            {
                const renderModelEntityDefAnimFlag_t &flag = anim.flags[m];
                idStr list;
                for(int n = 0; n < flag.parms.Num(); n++)
                {
                    list.Append(" ");
                    list.Append(flag.parms[n]);
                }

                common->Printf("      %d: %s -> %s\n", m, flag.command.c_str(), list.c_str());
            }
        }
    }

    bool idRenderModelEntityDef::IsValid(void) const
    {
        return !mesh.IsEmpty() && anims.Num() > 0;
    }
}
