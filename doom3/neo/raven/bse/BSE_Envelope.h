#pragma once

// helper ----------------------------------------------------------------------
ID_INLINE float TableSample(const idDeclTable* tbl, float x)
{
    // idTech tables take the argument as a float, but are implemented through a
    // generated thunk (`TableLookup`).  Wrap it so we can call it like a normal
    // function.
#if 1 //k??? TODO don't cast
    return tbl ? tbl->TableLookup(x) : 0.0f;
#else
	return tbl ? static_cast<float>(tbl->TableLookup(*reinterpret_cast<const int*>(&x))) : 0.0f;
#endif
}

// ============================================================================
//  Base class ‒ only common metadata lives here
// ============================================================================
class rvEnvParms
{
public:
    // --------------------------------------------------------------------- //
    //  Meta helpers
    // --------------------------------------------------------------------- //
    bool operator!=(const rvEnvParms& rhs) const { return !Compare(rhs); }
    int  GetType()              const;
    bool GetMinMax(float& lo, float& hi) const;
    void SetDefaultType();                 // choose “linear” table if none set
    void Init();                           // zero-init to sane defaults

    /*  Calculate `rate[]` for N channels.
        – If `mIsCount`  :  rate[i] = src[i] / duration
        – If !mIsCount   :  rate[i] = src[i]              */
    void CalcRate(float* outRate, float duration, int count,
        const float* srcRate) const;

    /*  Generic 3-channel evaluator – utility for derived Evaluate().        */
    void Evaluate3(float time,
        const float* start,
        const float* rate,
        const float* end,
        float* dest) const;

    idVec3 mEnvOffset;
    idVec3 mRate;
protected:
    bool Compare(const rvEnvParms& rhs) const;

public:
    const idDeclTable* mTable; // lookup curve/table
    bool               mIsCount;    // if true, mRate* is “count” not “Hz”

    rvEnvParms()
            : mEnvOffset(idVec3(0.0f, 0.0f, 0.0f)),
              mRate(idVec3(1.0f, 1.0f, 1.0f)),
              mTable(NULL),
              mIsCount(true)
    {}
};

// ============================================================================
//  1-D variant
// ============================================================================
class rvEnvParms1 : public rvEnvParms
{
public:
    // life-cycle
    void Init(const rvEnvParms& src, float duration);

    // per-frame evaluate
    void Evaluate(float time, float* dest) const;

    // data
    float mStart;
    float mEnd;
    float mRate;
    float mEnvOffset;

    rvEnvParms1()
            : rvEnvParms(),
              mStart(0.0f),
              mEnd(0.0f),
              mRate(1.0f),
              mEnvOffset(0.0f)
    {}
};

// ============================================================================
//  2-D variant
// ============================================================================
class rvEnvParms2 : public rvEnvParms
{
public:
    void  Init(const rvEnvParms& src, float duration);
    void  Evaluate(float time, float* dest) const;

    idVec2 mStart;
    idVec2 mEnd;
    idVec2 mRate;
    idVec2 mEnvOffset;
    bool   mFixedRateAndOffset;

    rvEnvParms2()
            : rvEnvParms(),
              mStart(vec2_origin),
              mEnd(vec2_origin),
              mRate(idVec2(1.f, 1.f)),
              mEnvOffset(idVec2(0.0f, 0.0f)),
              mFixedRateAndOffset(false)
    {}
};

// ============================================================================
//  3-D variant  (already had Scale / Transform earlier)
// ============================================================================
class rvEnvParms3 : public rvEnvParms
{
public:
    // life-cycle
    void Init(const rvEnvParms& src, float duration);

    // per-frame evaluate
    void Evaluate(float time, float* dest) const;

    // extra helpers kept from previous snippet
    void Scale(float k);
    void Transform(const idVec3& normal);
    void Rotate(const idAngles& a);

    idVec3 mStart;
    idVec3 mEnd;
    idVec3 mRate;
    idVec3 mEnvOffset;
    bool   mFixedRateAndOffset;

    rvEnvParms3()
    : rvEnvParms(),
      mStart(vec3_origin),
      mEnd(vec3_origin),
      mRate(idVec3(1.f, 1.f, 1.f)),
      mEnvOffset(vec3_zero),
      mFixedRateAndOffset(false)
    {}
};
