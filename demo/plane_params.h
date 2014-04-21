//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"

//------------------------------------------------------------

struct plane_params
{
//----------------
//      types

public:
    //float value with auto-initialisation
    class fvalue
    {
    public:
        inline fvalue(): value(0.0f) {}
        inline fvalue(const float& v): value(v) {}
        inline operator float&() { return value; }
        inline operator const float&() const { return value; }

    private: float value;
    };

    //ex: graph.get( argument.get( a ) )

    class argument
    {
    public:
        float get(float value) const;
        void set(int i, float value) { values[i] = value; }
        void calculate_diffs();

    private: float values[10]; float ks[10];
    };

    template<typename T> class graph
    {
    public: T get(float arg) const
        {
            const int idx = int(floorf(arg * 10));
            if(idx < 0) return values[0];
            if(idx >= 9) return values[9];
            const float k = (arg * 10.0f - idx);
            return values[idx] * (1.0f - k) + values[idx + 1] * k;
        }

        void set(int i, const T &value) { values[i] = value; }

    private: T values[10];
    };

    typedef nya_math::vec3 vec3;

//----------------
//      values

public:
    struct
    {
        struct
        {
            fvalue speedBreak;
            fvalue speedMax;
            fvalue speedCruising;
            fvalue speedBuffet;
            fvalue speedStall;
            fvalue speedMin;

        } speed;

        struct
        {
            fvalue acceleR;
            fvalue deceleR;
            fvalue thrustMinWait;
            fvalue afterburnerWait;
            fvalue thrustMaxWait;
            fvalue powerAfterBurnerR;
            fvalue speedUpperR;
            fvalue speedDownerR;
            fvalue speedRollDownerR;

        } accel;

        fvalue gndSpeedTakeoff;
        fvalue gndDrag0;
        fvalue gndDrag1;

    } move;
/*
    struct
    {
        fvalue buffetRecoverySpeedRate;
        fvalue buffetRotX;
        fvalue buffetRotY;
        fvalue buffetRotXFreq;
        fvalue buffetRotYFreq;
        fvalue buffetOffset;
        fvalue buffetRotBiasForGun;
        fvalue buffetRotBiasForReticle;
        fvalue altStall0;
        fvalue altStall1;

    } stall;
*/
    struct
    {
        fvalue rotStallR;
        fvalue pitchUpDownR;
        vec3 addRotR;
        vec3 decRotR;
        vec3 inputRotAdd;
        vec3 inputRotDec;
        fvalue standardyaw;
        fvalue stdArtificialRoll;
        fvalue pitchLimitPre;
        fvalue pitchLimitMax;
        fvalue rollLimitPre;
        fvalue rollLimitMax;
        fvalue pitchLimitPreGear;
        fvalue pitchLimitMaxGear;
        fvalue rollLimitPreGear;
        fvalue rollLimitMaxGear;

    } rot;

    struct
    {
        argument speed;
        graph<fvalue> diffNoseVelocityR;
        graph<vec3> speedRot;
        graph<vec3> rotGravR;

    } rotgraph;

    struct
    {
        struct
        {
            fvalue startInputTime;
            fvalue speedMinStart;
            fvalue speedEnd;
            fvalue stopDiffRadian;

        } startend;

        struct
        {
            fvalue addRadStartingPitch;
            fvalue addPitchR;
            fvalue pitchRate;
            fvalue yawRate;
            fvalue radLimit;
            fvalue radDecR;
            fvalue speedStartDec;
            fvalue speedDecR;
            fvalue speedDecMax;

        } move;

    } drift;

/*
    struct
    {
        fvalue springConstRotX;
        fvalue springConstRotZ;
        fvalue damperConstRotX;
        fvalue damperConstRotZ;
        fvalue timeMax;
        fvalue impactRotXMax;
        fvalue impactRotZMax;
        fvalue impactDamageMax;
        fvalue impactDamageMin;

    } stagger;
*/

//----------------
//      functions

public:
    bool load(const char *file_name);
};

//------------------------------------------------------------
