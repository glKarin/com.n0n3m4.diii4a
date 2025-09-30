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

// rvDeclEffect::mFlags
// ETQW SDK
enum {
    ETFLAG_HAS_SOUND				= BITT< 0 >::VALUE,
    ETFLAG_USES_ENDORIGIN			= BITT< 1 >::VALUE,
    ETFLAG_ATTENUATES				= BITT< 2 >::VALUE,
    ETFLAG_EDITOR_MODIFIED			= BITT< 3 >::VALUE,
    ETFLAG_USES_MATERIAL_COLOR		= BITT< 4 >::VALUE,
    ETFLAG_ORIENTATE_IDENTITY		= BITT< 5 >::VALUE,
    ETFLAG_USES_AMBIENT_CUBEMAP		= BITT< 6 >::VALUE,
    ETFLAG_HAS_PHYSICS				= BITT< 7 >::VALUE,
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
