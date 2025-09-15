#pragma once

/*
===============================================================================

    rvDeclEffect
    ------------
    Declarative description of a complex visual effect.  Each effect
    is built from one or more rvSegmentTemplate objects (particles,
    emitters, decals, lights, trails, etc.).

===============================================================================
*/

enum {
	DEF_SOUND = 1, // 0x1
	DEF_USES_END_ORIGIN = 1 << 1, // 0x2
	DEF_ATTENUATION = 1 << 2, // 0x4 ?
};

class rvDeclEffect final : public idDecl {
public:
    // --------------------------------------------------------------------- //
    //  Life-cycle
    // --------------------------------------------------------------------- //
    rvDeclEffect();                         // constructor
    virtual        ~rvDeclEffect();               // destructor

    // --------------------------------------------------------------------- //
    //  idDecl interface overrides
    // --------------------------------------------------------------------- //
    const char* DefaultDefinition() const;    // implicit text used by the material editor
    bool            SetDefaultText();       // injects the default definition into this->base
    bool            Parse(const char* text, const int textLength, bool noCaching);
    virtual size_t	Size(void) const;       // idDecl virtual
    void            FreeData();       // idDecl virtual

    // --------------------------------------------------------------------- //
    //  Public helpers
    // --------------------------------------------------------------------- //
    void            Init();                                // reset to initial *empty* state
    void            Finish();                              // final-pass after Parse()
    void            CopyData(const rvDeclEffect& src);
    void            Revert();                              // restore mEditorOriginal
    void            CreateEditorOriginal();
    bool            CompareToEditorOriginal() const;
    void            DeleteEditorOriginal();

    // segment helpers
    const rvSegmentTemplate* GetSegmentTemplate(const char* name) const;
    const rvSegmentTemplate* GetSegmentTemplate(int index) const;
    int                 GetTrailSegmentIndex(const idStr& name) const;

    // duration helpers
    void            SetMinDuration(float duration);
    void            SetMaxDuration(float duration);

public:
    // internal comparison used by the editor diff logic
    bool            Compare(const rvDeclEffect& rhs) const;
    float           CalculateBounds() const;               // expensive * walk segments and find radius

    // --------------------------------------------------------------------- //
    //  Private data
    // --------------------------------------------------------------------- //
    idList< rvSegmentTemplate >   mSegmentTemplates/*{ 16 }*/; // granularity 16 by default
    rvDeclEffect* mEditorOriginal;          // deep-copied *snapshot* shown in the editor
public:
    int              mFlags;                 // bit-field (see Finish())
    float            mMinDuration;
    float            mMaxDuration;
    float            mSize;            // editor preview bounds
    int              mPlayCount;                 // how many times we*ve played (runtime, not parsed)
    int              mLoopCount;                 // loops before auto-stop
};
