
#include "Model_md5mesh.h"
#include "Model_md5anim.h"

/* md5anim */
namespace md5model
{
    idMD5AnimFile::idMD5AnimFile(void)
            : version(MD5_VERSION)
    {}

    ID_INLINE idList<md5animHierarchy_t> & idMD5AnimFile::Hierarchies()
    {
        return hierarchies;
    }

    ID_INLINE idList<md5animBounds_t> & idMD5AnimFile::Bounds()
    {
        return bounds;
    }

    ID_INLINE idList<md5animBaseframe_t> & idMD5AnimFile::Baseframe()
    {
        return baseframe;
    }

    ID_INLINE idList<md5animFrames_t> & idMD5AnimFile::Frames()
    {
        return frames;
    }

    ID_INLINE md5animFrames_t & idMD5AnimFile::Frame(int index)
    {
        return frames[index];
    }

    ID_INLINE idStr & idMD5AnimFile::Commandline()
    {
        return commandline;
    }

    ID_INLINE int & idMD5AnimFile::FrameRate(void)
    {
        return frameRate;
    }

    ID_INLINE int & idMD5AnimFile::NumAnimatedComponents(void)
    {
        return numAnimatedComponents;
    }

    void idMD5AnimFile::Clear(void)
    {
        version = MD5_VERSION;
        numAnimatedComponents = 0;
        commandline.Clear();
        hierarchies.SetNum(0);
        bounds.SetNum(0);
        baseframe.SetNum(0);
        frames.SetNum(0);
    }

    void idMD5AnimFile::Write(const char *path) const
    {
        idFile *file = fileSystem->OpenFileWrite(path);

        // header
        Writefln(file, "MD5Version %d", version);
        Writefln(file, "commandline \"%s\"", commandline.c_str());
        Writeln(file);
        Writefln(file, "numFrames %d", frames.Num());
        Writefln(file, "numJoints %d", hierarchies.Num());
        Writefln(file, "frameRate %d", frameRate);
        Writefln(file, "numAnimatedComponents %d", numAnimatedComponents);
        Writeln(file);

        // hierarchy
        Writefln(file, "hierarchy {", hierarchies.Num());
        for(int i = 0; i < hierarchies.Num(); i++)
        {
            WriteIndent(file);
            const md5animHierarchy_t &joint = hierarchies[i];
            Writef(file, "\"%s\" %d %d %d", joint.boneName.c_str(), joint.parentIndex, joint.numComp, joint.frameIndex);
            if(joint.parentIndex != -1 && 0)
            {
                const md5animHierarchy_t &parent = hierarchies[joint.parentIndex];
                Writef(file, " // %d %s", i, parent.boneName.c_str());
            }
            Writeln(file);
        }
        Writefln(file, "}");
        Writeln(file);

        // bounds
        Writefln(file, "bounds {", bounds.Num());
        for(int i = 0; i < bounds.Num(); i++)
        {
            WriteIndent(file);
            const md5animBounds_t &bound = bounds[i];
            Writef(file, "( %.10f %.10f %.10f ) ( %.10f %.10f %.10f )", bound[0].x, bound[0].y, bound[0].z, bound[1].x, bound[1].y, bound[1].z);
            Writeln(file);
        }
        Writefln(file, "}");
        Writeln(file);

        // baseframe
        Writefln(file, "baseframe {", baseframe.Num());
        for(int i = 0; i < baseframe.Num(); i++)
        {
            WriteIndent(file);
            const md5animBaseframe_t &frame = baseframe[i];
            Writef(file, "( %.10f %.10f %.10f ) ( %.10f %.10f %.10f )", frame.xPos, frame.yPos, frame.zPos, frame.xOrient, frame.yOrient, frame.zOrient);
            if(0)
            {
                const md5animHierarchy_t &bone = hierarchies[i];
                Writef(file, " // %d %s", i, bone.boneName.c_str());
            }
            Writeln(file);
        }
        Writefln(file, "}");
        Writeln(file);

        // frame
        for(int m = 0; m < frames.Num(); m++)
        {
            Writefln(file, "frame %d {", m);
            const md5animFrames_t &frameTrans = frames[m];
            for(int i = 0; i < frameTrans.Num(); i++)
            {
                WriteIndent(file);
                const md5animFrame_t &frame = frameTrans[i];
                Writef(file, "%.10f %.10f %.10f %.10f %.10f %.10f", frame.xPos, frame.yPos, frame.zPos, frame.xOrient, frame.yOrient, frame.zOrient);
                if(0)
                {
                    const md5animHierarchy_t &bone = hierarchies[i];
                    Writef(file, " // %d %s", i, bone.boneName.c_str());
                }
                Writeln(file);
            }
            Writefln(file, "}");
            Writeln(file);
        }

        fileSystem->CloseFile(file);
    }

};
