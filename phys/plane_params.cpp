//
// open horizon -- undefined_darkness@outlook.com
//

#include "plane_params.h"
#include "util/util.h"
#include <string.h>
#include <stdio.h>

//------------------------------------------------------------

bool plane_params::load(const char *file_name)
{
    *this = plane_params(); //reset if was loaded already

    nya_memory::tmp_buffer_scoped fi_data(load_resource(file_name));
    nya_memory::memory_reader reader(fi_data.get_data(), fi_data.get_size());

    //ToDo
    unsigned short unknown_144 = reader.read<unsigned short>();
    assume(unknown_144 == 144);
    unsigned short unknown_20545 = reader.read<unsigned short>();
    assume(unknown_20545 == 20545);

    float params[183];
    for (int i = 0; i < sizeof(params) / sizeof(params[0]); ++i)
        params[i] = reader.read<float>();

    for (int i = 0; i < 10; ++i)
        rotgraph.speedRot.set(i, nya_math::vec3(&params[i * 3]));

    for (int i = 0; i < 10; ++i)
        rotgraph.rotGravR.set(i, nya_math::vec3(&params[30 + i * 3]));

    for (int i = 0; i < 10; ++i)
        rotgraph.speed.set(i, params[60 + i]);
    rotgraph.speed.calculate_diffs();

    for (int i = 0; i < 10; ++i)
        rotgraph.diffNoseVelocityR.set(i, params[70 + i]);

    rot.addRotR = nya_math::vec3(&params[83]);
    rot.decRotR = nya_math::vec3(&params[92]);
    rot.inputRotAdd = nya_math::vec3(&params[95]);
    rot.inputRotDec = nya_math::vec3(&params[98]);
    rot.standardyaw = params[101];
    rot.rotStallR = params[103];
    rot.pitchUpDownR = params[104];
    rot.stdArtificialRoll = params[105];
    rot.pitchLimitPre = params[106];
    rot.pitchLimitMax = params[107];
    rot.rollLimitPre = params[108];
    rot.rollLimitMax = params[109];
    rot.pitchLimitPreGear = params[110];
    rot.pitchLimitMaxGear = params[111];
    rot.rollLimitPreGear = params[112];
    rot.rollLimitMaxGear = params[113];

    move.accel.acceleR = params[117];
    move.accel.deceleR = params[118];
    move.accel.thrustMinWait = params[119];
    move.accel.afterburnerWait = params[120];
    move.accel.thrustMaxWait = params[121];
    move.accel.powerAfterBurnerR = params[122];
    move.accel.speedUpperR = params[123];
    move.accel.speedDownerR = params[124];
    move.accel.speedRollDownerR = params[125];

    move.speed.speedBreak = params[126];
    move.speed.speedMax = params[127];
    move.speed.speedCruising = params[128];
    move.speed.speedBuffet = params[129];
    move.speed.speedStall = params[130];
    move.speed.speedMin = params[131];
    move.gndSpeedTakeoff = params[132];
    move.gndDrag0 = params[133];
    move.gndDrag1 = params[134];

    drift.startend.startInputTime = params[179];
    drift.startend.speedMinStart = params[180];
    drift.startend.speedEnd = params[181];
    drift.startend.stopDiffRadian = params[182];

    drift.move.addRadStartingPitch = params[170];
    drift.move.addPitchR = params[171];
    drift.move.pitchRate = params[172];
    drift.move.yawRate = params[173];
    drift.move.radLimit = params[174];
    drift.move.radDecR = params[175];
    drift.move.speedStartDec = params[176];
    drift.move.speedDecR = params[177];
    drift.move.speedDecMax = params[178];

    misc.maxHp = params[168];

    return true;
}

//------------------------------------------------------------

float plane_params::argument::get(float value) const
{
    if (value < values[0]) return 0.0f;
    if (value >= values[9]) return 1.0f;
    for (int i = 8;i >= 0; --i)
        if (value >= values[i])
            return (float(i) + ks[i] * (value - values[i])) * 0.1f;

    return 0.0f;
}

//------------------------------------------------------------

void plane_params::argument::calculate_diffs()
{
    for (int i = 0; i < 9; ++i)
    {
        const float diff = values[i + 1] - values[i];
        ks[i] = diff > 0.0f ? 1.0f / diff : 0.0f;
    }
}
//------------------------------------------------------------
