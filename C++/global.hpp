#ifndef GLOBAL_HPP
#define GLOBAL_HPP



// Global variables

// Flow rate of purge in mm3/s, (default: 12mm3/s)
extern int g_flow_rate;
// Z position (in mm) of nozzle during purge, (default: 0.5mm)
extern float g_purge_height;
// Distance between tip of filament and nozzle before purge.
// Should be similar to PRINT_END final retract amount.
extern float g_tip_distance;
// Distance the purge will be in front of the print area, (default: 8mm)
extern int g_purge_margin;
// The length of purge line, (default: 10mm)
extern int g_purge_length;
// Amount of filament to be purged prior to printing, (default: 15mm)
extern int g_purge_amount;

// True when skirt is used in print
extern bool g_CombineSkirt;
// True when firmware retraction is used
extern bool g_UseFirmwareRetraction;
// Travel speed extract from gcode file, if not found, default value is 9000
extern int g_TravelSpeed;

enum SlicerType 
{
    NOT_SUPPORTED,
    ORCA_SLICER,
    PRUSA_SLICER
};


#endif
