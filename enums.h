#pragma once
enum itemTypes                // static entity types
{
    NOTUSED = 0,                // entity slot not in use in map (usually seen at deleted entities)
    LIGHT,                      // lightsource, attr1 = radius, attr2 = intensity (or attr2..4 = r-g-b)
    PLAYERSTART,                // attr1 = angle, attr2 = team
    I_CLIPS, I_AMMO, I_GRENADE, // attr1 = elevation
    I_HEALTH, I_HELMET, I_ARMOUR, I_AKIMBO,
    MAPMODEL,                   // attr1 = angle, attr2 = idx, attr3 = elevation, attr4 = texture, attr5 = pitch, attr6 = roll
    CARROT,                     // attr1 = tag, attr2 = type
    LADDER,                     // attr1 = height
    CTF_FLAG,                   // attr1 = angle, attr2 = red/blue
    SOUND,                      // attr1 = idx, attr2 = radius, attr3 = size, attr4 = volume
    CLIP,                       // attr1 = elevation, attr2 = xradius, attr3 = yradius, attr4 = height, attr6 = slope, attr7 = shape
    PLCLIP,                     // attr1 = elevation, attr2 = xradius, attr3 = yradius, attr4 = height, attr6 = slope, attr7 = shape
    DUMMYENT,                   // temporary entity without any function - will not be saved to map files, used to mark positions and for scripting
    MAXENTTYPES
};

enum gameModes                    // game modes
{
    DEMO = -1,
    TEAMDEATHMATCH = 0,           // 0
    COOPEDIT,
    DEATHMATCH,
    SURVIVOR,
    TEAMSURVIVOR,
    CTF,                          // 5
    PISTOLFRENZY,
    BOTTEAMDEATHMATCH,
    BOTDEATHMATCH,
    LASTSWISSSTANDING,
    ONESHOTONEKILL,               // 10
    TEAMONESHOTONEKILL,
    BOTONESHOTONEKILL,
    HUNTTHEFLAG,
    TEAMKEEPTHEFLAG,
    KEEPTHEFLAG,                  // 15
    TEAMPF,
    TEAMLSS,
    BOTPISTOLFRENZY,
    BOTLSS,
    BOTTEAMSURVIVOR,              // 20
    BOTTEAMONESHOTONKILL,
    NUM
};