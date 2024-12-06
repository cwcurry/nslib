/* emx_reader.h -- Header file for emx_reader.c

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.


   Kongsberg EM Datagram format implemented from:
     Kongsberg EM Series Multibeam Echo Sounder: EM Datagram Formats,
     Document Number 850-160692 Version O March 2012.

     HISAS PU - Output Datagram Formats.
     Document Number 445635 Revision B November 15 2019.

   Some history of the systems:
     1995 : EM3000.
     1996 : EM300.
     1998 : EM1002 replaces EM1000.
     1999 : EM120 replaces EM12 and EM121.
     2000 : EM2000.
     2004 : EM3002 replaces EM3000.
     2005 : EM710 replaces EM1002.
     2006 : EM302 replaces EM300.
     2007 : EM122 replaces EM120.
     2009 : EM2040 replaces EM3002 and EM2000.
     2012 : EM2040C.

   NOTES:
     Datagrams are not in strict sequential order.
     No navigation data is in the ping data.  It must be extrapolated from the
       Position Datagram.
     Beam depths are given relative to the transmit transducer or sonar head
       depth and the horizontal position from the positioning system's
       reference point. */

#ifndef EMX_READER_H
#define EMX_READER_H

#if defined (__cplusplus)
#include <cstdio>
#else
#include <stdio.h>
#endif

#define CPL_NEED_FIXED_WIDTH_T  /* Tell cpl_spec.h we need fixed-width types. */
#include "cpl_spec.h"


/******************************* DEFINITIONS *********************************/

/* EMX Null Definitions */
#define EMX_NULL_UINT8           0x000000FF
#define EMX_NULL_INT8            0x0000007F
#define EMX_NULL_UINT16          0x0000FFFF
#define EMX_NULL_INT16           0x00007FFF
#define EMX_NULL_UINT32          0xFFFFFFFF
#define EMX_NULL_INT32           0x7FFFFFFF

/* EMX Detection Info */
#define EMX_DETECT_INVALID       0x80  /* 128 */

/* EMX Model Definitions */
#define EM_MODEL_120             1
#define EM_MODEL_121A            2
#define EM_MODEL_122             3
#define EM_MODEL_124             4
#define EM_MODEL_300             5
#define EM_MODEL_302             6
#define EM_MODEL_710             7
#define EM_MODEL_712             8
#define EM_MODEL_ME70BO          9
#define EM_MODEL_1002            10
#define EM_MODEL_2000            11
#define EM_MODEL_2040            12
#define EM_MODEL_2040C           13
#define EM_MODEL_3000            14
#define EM_MODEL_3000D           15
#define EM_MODEL_3002            16
#define EM_MODEL_HISAS_1032      17
#define EM_MODEL_HISAS_1032D     18
#define EM_MODEL_HISAS_2040      19

/* EMX Pulse Type (signal_waveform_id) */
#define EMX_PULSE_TYPE_CW        0
#define EMX_PULSE_TYPE_LFM_UP    1
#define EMX_PULSE_TYPE_LFM_DOWN  2

/* EMX Datagram Types (datagram_type) */
#define EMX_DATAGRAM_DEPTH                 'D'  /* 0x44 = 68  */
#define EMX_DATAGRAM_DEPTH_NOMINAL         'Q'  /* 0x51 = 81  */  /* Undocumented */
#define EMX_DATAGRAM_XYZ                   'X'  /* 0x58 = 88  */
#define EMX_DATAGRAM_EXTRA_DETECTIONS      'l'  /* 0x6C = 108 */
#define EMX_DATAGRAM_CENTRAL_BEAMS         'K'  /* 0x4B = 75  */
#define EMX_DATAGRAM_RRA_101               'e'  /* 0x65 = 101 */  /* Undocumented */
#define EMX_DATAGRAM_RRA_70                'F'  /* 0x46 = 70  */
#define EMX_DATAGRAM_RRA_102               'f'  /* 0x66 = 102 */
#define EMX_DATAGRAM_RRA_78                'N'  /* 0x4E = 78  */
#define EMX_DATAGRAM_SEABED_IMAGE_83       'S'  /* 0x53 = 83  */
#define EMX_DATAGRAM_SEABED_IMAGE_89       'Y'  /* 0x59 = 89  */
#define EMX_DATAGRAM_WATER_COLUMN          'k'  /* 0x6B = 107 */
#define EMX_DATAGRAM_QUALITY_FACTOR        'O'  /* 0x4F = 79  */
#define EMX_DATAGRAM_ATTITUDE              'A'  /* 0x41 = 65  */
#define EMX_DATAGRAM_ATTITUDE_NETWORK      'n'  /* 0x6E = 110 */
#define EMX_DATAGRAM_CLOCK                 'C'  /* 0x43 = 67  */
#define EMX_DATAGRAM_HEIGHT                'h'  /* 0x68 = 104 */
#define EMX_DATAGRAM_HEADING               'H'  /* 0x48 = 72  */
#define EMX_DATAGRAM_POSITION              'P'  /* 0x50 = 80  */
#define EMX_DATAGRAM_SINGLE_BEAM_DEPTH     'E'  /* 0x45 = 69  */
#define EMX_DATAGRAM_TIDE                  'T'  /* 0x54 = 84  */
#define EMX_DATAGRAM_SSSV                  'G'  /* 0x47 = 71  */
#define EMX_DATAGRAM_SVP                   'U'  /* 0x55 = 85  */
#define EMX_DATAGRAM_SVP_EM3000            'V'  /* 0x56 = 86  */  /* Undocumented; deprecated */
#define EMX_DATAGRAM_KM_SSP_OUTPUT         'W'  /* 0x57 = 87  */
#define EMX_DATAGRAM_INSTALL_PARAMS        'I'  /* 0x49 = 73  */
#define EMX_DATAGRAM_INSTALL_PARAMS_STOP   'i'  /* 0x69 = 105 */
#define EMX_DATAGRAM_INSTALL_PARAMS_REMOTE 'j'  /* 0x70 = 106 */
#define EMX_DATAGRAM_REMOTE_PARAMS_INFO    'r'  /* 0x72 = 114 */
#define EMX_DATAGRAM_RUNTIME_PARAMS        'R'  /* 0x52 = 82  */
#define EMX_DATAGRAM_EXTRA_PARAMS          '3'  /* 0x33 = 51  */
#define EMX_DATAGRAM_PU_OUTPUT             '0'  /* 0x30 = 48  */
#define EMX_DATAGRAM_PU_STATUS             '1'  /* 0x31 = 49  */
#define EMX_DATAGRAM_PU_BIST_RESULT        'B'  /* 0x42 = 66  */
#define EMX_DATAGRAM_TRANSDUCER_TILT       'J'  /* 0x4A = 74  */
#define EMX_DATAGRAM_SYSTEM_STATUS         'o'  /* 0x6F = 111 */  /* Undocumented */
#define EMX_DATAGRAM_STAVE                 'm'  /* 0x6D = 109 */  /* Undocumented */
#define EMX_DATAGRAM_UNKNOWN1              's'  /* 0x73 = 115 */  /* Undocumented */  /* Surface sound speed (deprecated) */
#define EMX_DATAGRAM_UNKNOWN2              't'  /* 0x74 = 116 */  /* Undocumented */
#define EMX_DATAGRAM_UNKNOWN3              'v'  /* 0x76 = 118 */  /* Undocumented */  /* Input SSP */

/* EMX HISAS Datagram Types (datagram_type) */
#define EMX_DATAGRAM_HISAS_STATUS          '2'  /* 0x32 = 50  */
#define EMX_DATAGRAM_NAVIGATION_OUTPUT     '>'  /* 0x3E = 62  */
#define EMX_DATAGRAM_SIDESCAN_STATUS       '\"' /* 0x22 = 34  */
#define EMX_DATAGRAM_HISAS_1032_SIDESCAN   '%'  /* 0x25 = 37  */
#define EMX_DATAGRAM_RRA_123               '{'  /* 0x7B = 123 */


/******************************* EMX DATAGRAMS *******************************/

/* EMX Datagram Header (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t bytes_in_datagram;              /* Number of bytes in the datagram (not including this field). */
    uint8_t start_identifier;                /* Start identifier = STX (02h).                               */
    uint8_t datagram_type;                   /* Type of datagram.                                           */
    uint16_t em_model_number;                /* EM model number (example: EM 710 = 710).                    */
    uint32_t date;                           /* Date = year*10000 + month*100 + day.                        */
    uint32_t time_ms;                        /* Time since midnight in milliseconds (0-86399999).           */
    uint16_t counter;                        /* Counter (sequential counter) (0-65535)                      */
                                             /*  or byte order flag (in PU output datagram).                */
    uint16_t serial_number;                  /* System serial number (100 - ).                              */
} emx_datagram_header;


/* EMX Depth Info (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t vessel_heading;                 /* Heading of vessel in 0.01 deg (0-35999).                    */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
    uint16_t transducer_depth;               /* TX transducer depth re water level at time of ping in cm.   */
                                             /* NOTE: If the offset multiplier is -1, then transducer depth */
                                             /*  is actually -1 * 655.36 + transducer_depth.                */
    uint8_t max_beams;                       /* Maximum number of beams possible (48-).                     */
    uint8_t num_beams;                       /* Number of beams with valid detections = N.                  */
    uint8_t depth_resolution;                /* Depth (z) resolution in cm.                                 */
    uint8_t horizontal_resolution;           /* Horizontal (x and y) resolution in cm.                      */
    uint16_t sample_rate;                    /* Sample rate (f) in Hz (300-30000) or                        */
                                             /*  depth difference between sonar heads in the EM 3000D.      */
                                             /* NOTE: Assume depth difference is in cm.                     */
} emx_datagram_depth_info;


/* EMX Depth Beam Data (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t depth;                           /* Depth (z) from transmit transducer.                         */
                                             /*  (unsigned for EM120 and EM300).                            */
    int16_t across_track;                    /* Across track distance (y).  Must be multiplied by value in  */
                                             /*  horizontal_resolution.                                     */
    int16_t along_track;                     /* Along track distance (x).  Must be multiplied by value in   */
                                             /*  horizontal_resolution.                                     */
    int16_t beam_depression_angle;           /* Beam depression angle in 0.01 degrees.  Positive downwards  */
                                             /*  and 90 degrees for a vertical beam.                        */
    uint16_t beam_azimuth_angle;             /* Beam azimuth angle in 0.01 degrees.                         */
    uint16_t range;                          /* Range (one-way travel time) in samples.                     */
                                             /*  Two-way travel time = range / sampling rate / 2.           */
    uint8_t quality_factor;                  /* Quality factor (0-254).                                     */
                                             /*  The upper bit signifies whether amplitude (0) or phase (1) */
                                             /*  detection has been used.  If amplitude the 7 lowest bits   */
                                             /*  give the number of samples used in the center of gravity   */
                                             /*  calculation.  If phase the second highest bit signifies    */
                                             /*  whether a second (0) or first (1) order curve fit has been */
                                             /*  applied to determine the zero phase range, and the lowest  */
                                             /*  6 bits indicates the quality of the fit (actually the      */
                                             /*  normalized variance of the fit re the maximum allowed,     */
                                             /*  i.e, with a lower number the better the fit).              */
                                             /*  NOTE: This is not the same as XYZ quality factor.          */
    uint8_t detect_window_length;            /* Detection window length (samples/4) (1-254).                */
    int8_t backscatter;                      /* Reflectivity (BS) in 0.5 dB resolution.                     */
    uint8_t beam_number;                     /* Beam number (1-254).  Beam 128 is the first beam on the     */
                                             /*  second sonar head in an EM3000D dual head system.          */
} emx_datagram_depth_beam;


/* EMX Depth Datagram (EM2000, EM3000, EM3002, EM1002, EM300, and EM120) */
typedef struct {
    emx_datagram_depth_info *info;           /* Pointer to depth datagram info.                             */
    emx_datagram_depth_beam *beam;           /* Array of num_beams (N) depth beam data.                     */
    int8_t depth_offset_multiplier;          /* Transducer depth offset multiplier (-1 to +17).             */
} emx_datagram_depth;


/* EMX XYZ Info (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t vessel_heading;                 /* Heading of vessel (at TX time) in 0.01 degree (0-35999).    */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
    float transducer_depth;                  /* TX transducer depth re water level at time of ping in m.    */
                                             /*  This should be added to the beam depths for total depth.   */
    uint16_t num_beams;                      /* Number of beams in datagram = N.                            */
    uint16_t valid_beams;                    /* Number of beams with valid detections.                      */
    float sample_rate;                       /* Sample rate in Hz.                                          */
    uint8_t scanning_info;                   /* Scanning info (EM2040 only):                                */
                                             /*  0 - Scanning not used.                                     */
                                             /*  Lower 4 bits encodes the total number of scanning sectors. */
                                             /*  Upper 4 bits encodes the actual scanning sector.           */
    uint8_t spare[3];                        /* Spare.                                                      */
} emx_datagram_xyz_info;


/* EMX XYZ Beam Data (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float depth;                             /* Depth (z) from transmit transducer in meters.               */
                                             /*  Heave, roll, pitch, sound speed have been applied.         */
    float across_track;                      /* Across track distance (y) in meters.                        */
    float along_track;                       /* Along track distance (x) in meters.                         */
    uint16_t detect_window_length;           /* Detection window length in samples.                         */
    uint8_t quality_factor;                  /* Quality factor (0-254).                                     */
                                             /*  Scaled std dev. of the range detection, Q = 250*sd/dr.     */
    int8_t beam_adjustment;                  /* Incidence beam adjustment (IBA) in 0.1 degree (-128-126).   */
    uint8_t detection_info;                  /* Detection information:                                      */
                                             /*  0xxx 0000 - Valid detection - Amplitude detect.            */
                                             /*  0xxx 0001 - Valid detection - Phase detect.                */
                                             /*  1xxx 0000 - Invalid detection - Normal.                    */
                                             /*  1xxx 0001 - Invalid detection - Interpolated.              */
                                             /*  1xxx 0010 - Invalid detection - Estimated.                 */
                                             /*  1xxx 0011 - Invalid detection - Rejected.                  */
                                             /*  1xxx 0100 - Invalid detection - No data.                   */
    int8_t system_cleaning;                  /* Real-time cleaning information (negative = flagged).        */
    int16_t backscatter;                     /* Reflectivity (BS) in 0.1 dB resolution.                     */
                                             /* NOTE: -100 dB appears to be invalid value.                  */
} emx_datagram_xyz_beam;


/* EMX XYZ Datagram (EM2040, EM710, EM122, EM302, ME70) */
typedef struct {
    emx_datagram_xyz_info *info;             /* Pointer to XYZ datagram info.                               */
    emx_datagram_xyz_beam *beam;             /* Array of num_beams (N) XYZ 88 beam data.                    */
} emx_datagram_xyz;


/* EMX Nominal Depth Info (8 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float transducer_depth;                  /* TX transducer depth re water level at time of ping in m.    */
    uint16_t max_beams;                      /* Maximum number of beams possible (48-).                     */
    uint16_t num_beams;                      /* Number of beams with valid detections = N.                  */
} emx_datagram_depth_nominal_info;


/* EMX Nominal Depth Beam Data (14 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float depth;                             /* Depth (z) from transmit transducer in m.                    */
                                             /*  NOTE: Have seen NaN values in EM122 Data.                  */
    float across_track;                      /* Across track distance (y) in meters.                        */
    float along_track;                       /* Along track distance (x) in meters.                         */
    uint8_t detection_info;                  /* Detection info.                                             */
    int8_t system_cleaning;                  /* Real-time cleaning information (negative = flagged).        */
} emx_datagram_depth_nominal_beam;


/* EMX Nominal Depth Datagram */
typedef struct {
    emx_datagram_depth_nominal_info *info;   /* Pointer of nominal depth datagram info.                     */
    emx_datagram_depth_nominal_beam *beam;   /* Array of valid_beams (N) depth beam data.                   */
} emx_datagram_depth_nominal;


/* EMX Extra Detections Info (36 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t datagram_counter;               /* Datagram counter.                                           */
    uint16_t datagram_version;               /* Datagram version ID.                                        */
    uint16_t swath_counter;                  /* Swath counter.                                              */
    uint16_t swath_index;                    /* Swath index.                                                */
    uint16_t vessel_heading;                 /* Heading of vessel in 0.01 degree (0-35999).                 */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
    float reference_depth;                   /* Depth of reference point in meters.                         */
    float wc_sample_rate;                    /* Water column sample rate in Hz.                             */
    float raw_amplitude_sample_rate;         /* Raw amplitude (seabed image) sample rate in Hz.             */
    uint16_t rx_transducer_index;            /* Rx transducer index.                                        */
    uint16_t num_detects;                    /* Number of extra detections.                                 */
    uint16_t num_classes;                    /* Number of detection classes.                                */
    uint16_t nbytes_class;                   /* Number of bytes per class (cycle 1).                        */
    uint16_t nalarm_flags;                   /* Number of alarm flags.                                      */
    uint16_t nbytes_detect;                  /* Number of bytes per detection (cycle 2).                    */
} emx_datagram_extra_detect_info;


/* EMX Extra Detections Class (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t start_depth;                    /* Start depth (% of depth) (0-300).                           */
    uint16_t stop_depth;                     /* Stop depth (% of depth) (1-300).                            */
    uint16_t qf_threshold;                   /* 100 * QF threshold (0.01-1).                                */
    int16_t bs_threshold;                    /* Backscatter threshold in dB.                                */
    uint16_t snr_threshold;                  /* SNR threshold in dB.                                        */
    uint16_t alarm_threshold;                /* Alarm threshold (number of extra detections required).      */
    uint16_t num_detects;                    /* Number of extra detections.                                 */
    uint8_t show_class;                      /* Show class (0-1).                                           */
    uint8_t alarm_flag;                      /* Alarm flag (0 or 1/16/17).                                  */
} emx_datagram_extra_detect_class;


/* EMX Extra Detections Data (68 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float depth;                             /* Depth in meters.                                            */
    float across_track;                      /* Across-track distance in meters.                            */
    float along_track;                       /* Along-track distance in meters.                             */
    float latitude_delta;                    /* Delta latitude.                                             */
    float longitud_delta;                    /* Delta longitude.                                            */
    float beam_angle;                        /* Beam pointing angle (degrees re array).                     */
    float angle_correction;                  /* Applied pointing angle correction.                          */
    float travel_time;                       /* Two-way travel time in seconds.                             */
    float travel_time_correction;            /* Applied two-way travel time corrections in seconds.         */
    int16_t backscatter;                     /* Backscatter in 0.1 dB.                                      */
    int8_t beam_adjustment;                  /* Beam incidence angle adjustment (IBA) in 0.1 degree.        */
    int8_t detection_info;                   /* Detection info.                                             */
    uint16_t spare;                          /* Spare.                                                      */
    uint16_t tx_sector;                      /* Tx sector number/TX array index.                            */
    uint16_t detection_window_length;        /* Detection window length.                                    */
    uint16_t quality_factor;                 /* Quality factor (old).                                       */
    uint16_t system_cleaning;                /* Real-time cleaning info.                                    */
    uint16_t range_factor;                   /* Range factor in %.                                          */
    uint16_t class_number;                   /* Detection class number.                                     */
    uint16_t confidence;                     /* Confidence level.                                           */
    uint16_t qf_ifremer;                     /* QF * 10 (IFREMER quality factor).                           */
    uint16_t wc_beam_number;                 /* Water column beam number.                                   */
    float beam_angle_across;                 /* Beam angle across re vertical in degrees.                   */
    uint16_t detected_range;                 /* Detected range in (WCsr) samples.                           */
    uint16_t raw_amplitude;                  /* Raw amplitude samples (Ns).                                 */
} emx_datagram_extra_detect_data;


/* EMX Extra Detections Datagram (EM2040 and EM2040C with Slim Processing Unit) */
typedef struct {
    emx_datagram_extra_detect_info *info;    /* Extra detections info.                                      */
    emx_datagram_extra_detect_class *classes;/* Array of extra detection classes.                           */
    emx_datagram_extra_detect_data *data;    /* Array of extra detection data.                              */
    int16_t *raw_amplitude;                  /* Array of raw amplitude samples.                             */
} emx_datagram_extra_detect;


/* EMX Central Beams Info (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t mean_abs_coef;                  /* Mean absorption coefficient in 0.01 dB/km (1-20000).        */
    uint16_t pulse_length;                   /* Pulse length in us (50-).                                   */
    uint16_t range_norm;                     /* Range to normal incidence used to correct sample            */
                                             /*  amplitudes (in number of samples).                         */
    uint16_t start_range;                    /* Start range sample of TVG ramp if not enough dynamic range. */
    uint16_t stop_range;                     /* Stop range sample of TVG ramp if not enough dynamic range.  */
    int8_t normal_incidence_bs;              /* Normal incidence BS in dB (BSN) (-50-+10).                  */
    int8_t oblique_bs;                       /* Oblique BS in dB (BSO) (-60-0).                             */
    uint16_t tx_beamwidth;                   /* Tx beamwidth along in 0.1 degree (1-300).                   */
    uint8_t tvg_cross_over;                  /* TVG law cross over angle in 0.1 degree (20-300).            */
    uint8_t num_beams;                       /* Number of beams (N).                                        */
} emx_datagram_central_beams_info;


/* EMX Central Beams Data (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t beam_index;                      /* Beam index number (0-253).  The beam index number is the    */
                                             /*  beam number - 1.                                           */
    uint8_t spare;                           /* Spare byte to get even length (always zero).                */
    uint16_t num_samples;                    /* Number of samples per beam = Ns.                            */
    uint16_t start_range;                    /* Start range in samples.  The range for which the first      */
                                             /*  sample amplitude is valid for this beam is given as a      */
                                             /*  two-way range.  The detection range is given in the raw    */
                                             /*  range and angle datagram.  Note that data are provided     */
                                             /*  regardless of whether a beam has a valid detection or not. */
} emx_datagram_central_beams_data;


/* EMX Central Beams Datagram (EM120 and EM300) */
typedef struct {
    emx_datagram_central_beams_info *info;   /* Pointer to Central Beams datagram info.                     */
    emx_datagram_central_beams_data *beam;   /* Array of num_beams (N) central beam data.                   */
    int8_t *amplitude;                       /* Sample amplitudes in 0.5 dB (-128 to +126).                 */
                                             /*  Amplitudes are not corrected in accordance with the        */
                                             /*  detection parameters derived for the ping, as is done for  */
                                             /*  the seabed image data.                                     */
} emx_datagram_central_beams;


/* EMX Raw Range and Angle 101 Info (30 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t vessel_heading;                 /* Heading of vessel in 0.01 degree (0-35999).                 */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
    uint16_t transducer_depth;               /* TX transducer depth re water level at time of ping in cm.   */
    uint8_t max_beams;                       /* Maximum number of beams possible (48-).                     */
    uint8_t num_beams;                       /* Number of beams with valid detections = N.                  */
    uint8_t depth_resolution;                /* Depth (z) resolution in cm.                                 */
    uint8_t horizontal_resolution;           /* Horizontal (x and y) resolution in cm.                      */
    uint16_t sample_rate;                    /* Sample rate (f) in Hz (300-30000).                          */
    int32_t status;
    uint16_t range_norm;                     /* Range to normal incidence used to correct sample            */
                                             /*  amplitudes (in number of samples).                         */
    int8_t normal_incidence_bs;              /* Normal incidence BS in 0.1 dB (BSN).                        */
    int8_t oblique_bs;                       /* Oblique BS in 0.1 dB (BSO).                                 */
    uint8_t fixed_gain;
    int8_t tx_power;
    uint8_t mode;
    uint8_t coverage;
    uint16_t yawstab_heading;
    uint16_t tx_sectors;                     /* Number of transmit sectors = Ntx (1-).                      */
    uint16_t spare;                          /* Spare.                                                      */
} emx_datagram_rra_101_info;


/* EMX Raw Range and Angle 101 TX Beam (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t last_beam;
    int16_t tx_tilt_angle;                   /* Tilt angle re TX array in 0.01 degree (-2900-2900).         */
    uint16_t heading;
    int16_t roll;
    int16_t pitch;
    int16_t heave;
} emx_datagram_rra_101_tx_beam;


/* EMX Raw Range and Angle 101 Beam (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t range;                          /* Two-way travel time.  The range resolution in time is the   */
                                             /*  inverse of the range sampling rate given in the depth      */
                                             /*  datagram.                                                  */
    uint8_t quality_factor;                  /* Quality factor (0-254).                                     */
                                             /*  Scaled std dev. of the range detection, Q = 250*sd/dr.     */
    uint8_t detect_window_length;            /* Detection window length in samples (/4 if phase).           */
    int8_t backscatter;                      /* Reflectivity (BS) in 0.5 dB resolution.                     */
    uint8_t beam_number;                     /* Beam number (-1999 to 1999).  Beam number starts at 0.      */
    int16_t rx_beam_angle;                   /* Beam pointing angle re RX array in 0.01 degrees.            */
    uint16_t rx_heading;
    int16_t roll;
    int16_t pitch;
    int16_t heave;
} emx_datagram_rra_101_rx_beam;


/* EMX Raw Range and Angle 101 Datagram */
typedef struct {
    emx_datagram_rra_101_info *info;         /* Pointer to RRA 101 datagram info.                           */
    emx_datagram_rra_101_tx_beam *tx_beam;   /* Array of tx_sectors (Ntx) TX beams.                         */
    emx_datagram_rra_101_rx_beam *rx_beam;   /* Array of num_beams (Nrx) raw, range, and angle data.        */
} emx_datagram_rra_101;


/* EMX Raw Range and Angle 70 Info (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t max_beams;                       /* Maximum number of beams possible (48-).                     */
    uint8_t num_beams;                       /* Number of RX beams with valid detections = Nrx (1-).        */
                                             /* NOTE: Have seen data with num_beams = 0.                    */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
} emx_datagram_rra_70_info;


/* EMX Raw Range and Angle 70 Beam (8 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t beam_angle;                      /* Beam pointing angle in 0.01 degrees (positive to port).     */
    uint16_t tx_tilt_angle;                  /* Transmit tilt angle in 0.01 degrees.                        */
                                             /*  The beam pointing angle is positive to port and the        */
                                             /*  transmit tilt angle is positive forwards for a normally    */
                                             /*  mounted system looking downwards.                          */
    uint16_t range;                          /* Two-way travel time.  The range resolution in time is the   */
                                             /*  inverse of the range sampling rate given in the depth      */
                                             /*  datagram (0-65534).                                        */
    int8_t backscatter;                      /* Reflectivity (BS) in 0.5 dB resolution (-128-+126).         */
    uint8_t beam_number;                     /* Beam number (1-254).                                        */
                                             /* NOTE: The beam number does not appear to always start at 1. */
                                             /*  On some systems it is 0 and others it is 1.                */
} emx_datagram_rra_70_beam;


/* EMX Raw Range and Angle 70 Datagram */
typedef struct {
    emx_datagram_rra_70_info *info;          /* Pointer to RRA 70 datagram info.                            */
    emx_datagram_rra_70_beam *beam;          /* Array of num_beams (Nrx) raw, range, and angle data.        */
} emx_datagram_rra_70;


/* EMX Raw Range and Angle 102 Info (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t tx_sectors;                     /* Number of transmit sectors = Ntx (1-).                      */
    uint16_t num_beams;                      /* Number of RX beams with valid detections = Nrx (1-1999).    */
    uint32_t sample_rate;                    /* Sample rate in 0.01 Hz.                                     */
    int32_t rov_depth;                       /* ROV depth in cm.                                            */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
    uint16_t max_beams;                      /* Maximum number of beams possible.                           */
    uint16_t spare1;                         /* Spare.                                                      */
    uint16_t spare2;                         /* Spare.                                                      */
} emx_datagram_rra_102_info;


/* EMX Raw Range and Angle 102 TX Beam (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t tx_tilt_angle;                   /* Tilt angle re TX array in 0.01 deg (-2900-2900).            */
    uint16_t focus_range;                    /* Focus range in 0.1m (0=no focusing applied).                */
    uint32_t signal_length;                  /* Signal length in microseconds.                              */
    uint32_t tx_offset;                      /* Transmit time offset in microseconds.                       */
    uint32_t center_freq;                    /* Center frequency in Hz.                                     */
    uint16_t signal_bandwidth;               /* Bandwidth in 10 Hz.                                         */
    uint8_t signal_waveform_id;              /* Signal waveform identifier (0-99): 0 - CW, 1 - FM.          */
    uint8_t tx_sector;                       /* Transmit sector number / TX array index.                    */
} emx_datagram_rra_102_tx_beam;


/* EMX Raw Range and Angle 102 RX Beam (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t rx_beam_angle;                   /* Beam pointing angle re RX array in 0.01 degrees.            */
    uint16_t range;                          /* Range in 0.25 samples (R).                                  */
                                             /*  Two-way travel time is = R /(4 F/ 100).                    */
    uint8_t tx_sector_number;                /* Transmit sector number (0-19).                              */
    int8_t backscatter;                      /* Reflectivity (BS) in 0.5 dB resolution.                     */
    uint8_t quality_factor;                  /* Quality factor (0-254).                                     */
                                             /*  Scaled std dev. of the range detection, Q = 250*sd/dr.     */
    uint8_t detect_window_length;            /* Detection window length in samples (/4 if phase).           */
    int16_t beam_number;                     /* Beam number (-1999 to 1999).  Beam number starts at 0.      */
    uint16_t spare;                          /* Spare.                                                      */
} emx_datagram_rra_102_rx_beam;


/* EMX Raw Range and Angle 102 Datagram (EM120, EM300, EM1002, EM2000, EM3000, and EM3002) */
typedef struct {
    emx_datagram_rra_102_info *info;         /* Pointer to Raw, Range, and Angle 102 datagram info.         */
    emx_datagram_rra_102_tx_beam *tx_beam;   /* Array of tx_sectors (Ntx) TX beams.                         */
    emx_datagram_rra_102_rx_beam *rx_beam;   /* Array of num_beams (Nrx) RX beams.                          */
} emx_datagram_rra_102;


/* EMX Raw Range and Angle 78 Info (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s (14000-16000).            */
    uint16_t tx_sectors;                     /* Number of transmit sectors = Ntx (1-).                      */
    uint16_t num_beams;                      /* Number of receive beams in datagram = Nrx (1-).             */
    uint16_t valid_beams;                    /* Number of beams with a valid detection (1-).                */
    float sample_rate;                       /* Sample rate in Hz.                                          */
    uint32_t dscale;                         /* The Doppler correction applied in FM mode is documented     */
                                             /*  here to allow the uncorrected slant ranges to be           */
                                             /*  recreated if desired.  The correction is scaled by a       */
                                             /*  common scaling constant for all beams and then included    */
                                             /*  in the datagram using a signed 8-bit value for each        */
                                             /*  beam.  The uncorrected range (two-way travel time) can     */
                                             /*  be reconstructed by subtracting the correction from the    */
                                             /*  range in the datagram:                                     */
                                             /*  T(uncorrected) = T(datagram) - D(corr)/D(scale).           */
} emx_datagram_rra_78_info;


/* EMX Raw Range and Angle 78 TX Beam (24 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t tx_tilt_angle;                   /* Tilt angle re TX array in 0.01 deg (-2900-2900).            */
    uint16_t focus_range;                    /* Focus range in 0.1m (0=no focusing applied).                */
    float signal_length;                     /* Signal length in seconds.                                   */
    float sector_tx_delay;                   /* Sector transmit delay re first TX pulse in seconds.         */
    float center_freq;                       /* Center frequency in Hz.                                     */
    uint16_t mean_absorption;                /* Mean absorption coefficient in 0.01 dB/km.                  */
    uint8_t signal_waveform_id;              /* Signal waveform identifier (0-99):                          */
                                             /*  0 - CW, 1 - FM up sweep, 2 - FM down sweep.                */
    uint8_t tx_sector;                       /* Transmit sector number / TX array index.                    */
    float signal_bandwidth;                  /* Signal bandwidth in Hz.                                     */
} emx_datagram_rra_78_tx_beam;


/* EMX Raw Range and Angle 78 RX Beam (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t rx_beam_angle;                   /* Beam pointing angle re RX array in 0.01 degrees.            */
    uint8_t tx_sector_number;                /* Transmit sector number.                                     */
    uint8_t detection_info;                  /* Detection info.                                             */
    uint16_t detect_window_length;           /* Detection window length in samples.                         */
    uint8_t quality_factor;                  /* Quality factor (0-254).                                     */
                                             /*  Scaled std dev. of the range detection, Q = 250*sd/dr.     */
    int8_t doppler_correction;               /* Doppler correction applied in FM mode to TWTT (scaled).     */
    float two_way_travel_time;               /* Two-way travel time in seconds.                             */
    int16_t backscatter;                     /* Reflectivity (BS) in 0.1 dB resolution.                     */
    int8_t system_cleaning;                  /* Real-time cleaning information.                             */
    uint8_t spare;                           /* Spare.                                                      */
} emx_datagram_rra_78_rx_beam;


/* EMX Raw Range and Angle 78 Datagram (EM122, EM302, EM710, ME70 BO, EM2040, and EM2040C) */
typedef struct {
    emx_datagram_rra_78_info *info;          /* Pointer to Raw Range and Angle 78 datagram info.            */
    emx_datagram_rra_78_tx_beam *tx_beam;    /* Array of tx_sectors (Ntx) TX beams.                         */
    emx_datagram_rra_78_rx_beam *rx_beam;    /* Array of num_beams (Nrx) RX beams.                          */
} emx_datagram_rra_78;


/* EMX Seabed Image 83 Info (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t mean_abs_coef;                  /* Mean absorption coefficient in 0.01 dB/km (1-20000).        */
    uint16_t pulse_length;                   /* Pulse length in us (50-).                                   */
                                             /*  NOTE: The spec says the above two fields had other         */
                                             /*  definitions earlier.  This is in the version F (2000),     */
                                             /*  which the earliest spec I've seen.                         */
    uint16_t range_norm;                     /* Range to normal incidence used to correct sample            */
                                             /*  amplitudes (in number of samples).                         */
    uint16_t start_range;                    /* Start range sample of TVG ramp if not enough dynamic range. */
    uint16_t stop_range;                     /* Stop range sample of TVG ramp if not enough dynamic range.  */
    int8_t normal_incidence_bs;              /* Normal incidence BS in 0.1 dB (BSN).                        */
    int8_t oblique_bs;                       /* Oblique BS in 0.1 dB (BSO).                                 */
                                             /* NOTE: BSn and BSo may have been stored in dB not 0.1 dB.    */
    uint16_t tx_beamwidth;                   /* TX beamwidth in 0.1 deg (1 - 300).                          */
    uint8_t tvg_cross_over;                  /* TVG law crossover angle in 0.1 deg (20 - 300).              */
    uint8_t num_beams;                       /* Number of valid beams (N).                                  */
} emx_datagram_seabed_83_info;


/* EMX Seabed Image 83 Beam (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t beam_index;                      /* Beam index number (0-253).                                  */
    int8_t sorting_direction;                /* Sorting direction (-1 or 1).  The first sample in a beam    */
                                             /*  has lowest range if 1, highest if -1.                      */
    uint16_t num_samples;                    /* Number of samples per beam = Ns.                            */
    uint16_t detect_sample;                  /* Detection point in the beam (called center sample number).  */
                                             /* NOTE: Spec says detect_sample starts numbering at 1, but    */
                                             /*  have seen many values set at zero.                         */
                                             /* NOTE: Have seen detect_sample > num_samples in EM1002 data. */
} emx_datagram_seabed_83_beam;


/* EMX Seabed Image 83 Datagram (EM2000, EM3000, EM3002, EM1002, EM300, and EM120) */
typedef struct {
    emx_datagram_seabed_83_info *info;       /* Pointer to Seabed 83 datagram info.                         */
    emx_datagram_seabed_83_beam *beam;       /* Array of num_beams (N) beams.                               */
    int8_t *amplitude;                       /* Sample amplitudes in 0.5 dB (-128-126) (e.g., -30dB=196).   */
    int bytes_end;                           /* Number of bytes in the amplitude array including possible   */
                                             /*  spare byte to get even datagram length.                    */
                                             /* NOTE: This is not in the spec.  This just helps bound the   */
                                             /*  amplitude array size when it is calculated.                */
} emx_datagram_seabed_83;


/* EMX Seabed Image 89 Info (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float sample_rate;                       /* Amplitude sample rate in Hz.  Not the same as the sample    */
                                             /*  in the Depth or RRA datagram.                              */
    uint16_t range_norm;                     /* Range to normal incidence used to correct sample            */
                                             /*  amplitudes (in number of samples).                         */
    int16_t normal_incidence_bs;             /* Normal incidence BS in 0.1 dB (BSN).                        */
    int16_t oblique_bs;                      /* Oblique BS in 0.1 dB (BSO).                                 */
    uint16_t tx_beamwidth;                   /* Tx beamwidth in 0.1 deg (1 - 300).                          */
    uint16_t tvg_cross_over;                 /* TVG law cross over angle in 0.1 deg (20 - 300).             */
    uint16_t num_beams;                      /* Number of valid beams (N).                                  */
} emx_datagram_seabed_89_info;


/* EMX Seabed Image 89 Beam (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int8_t sorting_direction;                /* Sorting direction (-1 or 1).  The first sample in a beam    */
                                             /*  has lowest range if 1, highest if -1.                      */
    uint8_t detection_info;                  /* Detection info.                                             */
    uint16_t num_samples;                    /* Number of samples per beam = Ns.                            */
    uint16_t detect_sample;                  /* Detection point in the beam (called center sample number).  */
} emx_datagram_seabed_89_beam;


/* EMX Seabed Image 89 Datagram */
typedef struct {
    emx_datagram_seabed_89_info *info;       /* Pointer to Seabed 89 datagram info.                         */
    emx_datagram_seabed_89_beam *beam;       /* Array of num_beams (N) beams.                               */
    int16_t *amplitude;                      /* Sample amplitudes in 0.1 dB (e.g., -30.2 dB = 65234).       */
    int bytes_end;                           /* Number of bytes in the amplitude array including possible   */
                                             /*  spare byte to get even datagram length.                    */
                                             /* NOTE: This is not in the spec.  This just helps bound the   */
                                             /*  amplitude array size when it is calculated.                */
} emx_datagram_seabed_89;


/* EMX Water Column Info (24 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_datagrams;                  /* Number of datagrams Nd.                                     */
    uint16_t datagram_number;                /* Datagram number (1-Nd).                                     */
    uint16_t tx_sectors;                     /* Number of transmit sectors = Ntx (1-20).                    */
    uint16_t num_beams;                      /* Number of receive beams (1-Nd).                             */
    uint16_t datagram_beams;                 /* Number of beams in this datagram = Nrx.                     */
    uint16_t sound_speed;                    /* Sound speed in 0.1 m/s (14000-16000).                       */
    uint32_t sample_rate;                    /* Sample rate in 0.01 Hz resolution (1000-4000000).           */
    int16_t tx_heave;                        /* TX time heave (at transducer) in cm (-1000 to 1000).        */
    uint8_t tvg_function;                    /* TVG function applied (X) (20-40).                           */
                                             /*  The TVG function is X log R + 2 alpha R + OFS + C.         */
    int8_t tvg_offset;                       /* TVG offset in dB (C).                                       */
    uint8_t scanning_info;                   /* Scanning info (only for EM2040).                            */
    uint8_t spare[3];                        /* Spare.                                                      */
} emx_datagram_wc_info;


/* EMX Water Column TX Beam (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t tx_tilt_angle;                   /* Tilt angle re TX array in 0.01 deg (-1100-1100).            */
    uint16_t center_freq;                    /* Center frequency in 10 Hz (1000-50000).                     */
    uint8_t tx_sector;                       /* Transmit sector number (0-19).                              */
    uint8_t spare;                           /* Spare.                                                      */
} emx_datagram_wc_tx_beam;


/* EMX Water Column RX Beam Info (10 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int16_t beam_angle;                      /* Beam pointing angle re vertical in 0.01 deg (-11000-11000). */
    uint16_t start_range;                    /* Start range sample number (0-65534).                        */
    uint16_t num_samples;                    /* Number of samples (Ns).                                     */
    uint16_t detected_range;                 /* Detected range in samples (DR).                             */
    uint8_t tx_sector;                       /* Transmit sector number (0-19).                              */
    uint8_t beam_index;                      /* Beam index (0-254).                                         */
} emx_datagram_wc_rx_beam_info;


/* EMX Water Column RX Beam */
typedef struct {
    emx_datagram_wc_rx_beam_info *info;      /* Pointer to Water Column datagram RX beam info.              */
    int8_t *amplitude;                       /* Sum Ns entries of sample amplitude in 0.5 dB resolution.    */
} emx_datagram_wc_rx_beam;


/* EMX Water Column Datagram (EM122, EM302, EM710, EM2040, EM3002, ME70BO) */
typedef struct {
    emx_datagram_wc_info *info;              /* Pointer to Water Column datagram info.                      */
    emx_datagram_wc_tx_beam *txbeam;         /* Array of TX beams.                                          */
    uint8_t *beamData;                       /* Array of RX beams.  Parse with emx_get_wc_rxbeam ().        */
} emx_datagram_wc;


/* EMX Quality Factor Info (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_beams;                      /* Number of receive beams = Nrx (1-).                         */
    uint8_t npar;                            /* Number of parameters per beam = Npar (1-).  Currently only  */
                                             /*  one parameter is defined.                                  */
                                             /*  NOTE: Some early data may have npar=0.                     */
    uint8_t spare;                           /* Spare.                                                      */
} emx_datagram_qf_info;


/* EMX Quality Factor Datagram */
typedef struct {
    emx_datagram_qf_info *info;              /* Pointer to Quality Factor datagram info.                    */
    float *data;                             /* Array of num_beams (Nrx) IFREMER quality factor (>=0).      */
} emx_datagram_qf;


/* EMX Attitude Info (2 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_entries;                    /* Number of entries = N (1-).                                 */
} emx_datagram_attitude_info;


/* EMX Attitude Data (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t record_time;                    /* Time in milliseconds since record start.                    */
    uint16_t status;                         /* Sensor status.  The sensor status will be copied from the   */
                                             /*  input datagram's two sync bytes if the sensor uses the EM  */
                                             /*  format.                                                    */
    int16_t roll;                            /* Sensor roll in 0.01 degrees.                                */
    int16_t pitch;                           /* Sensor pitch in 0.01 degrees.                               */
    int16_t heave;                           /* Sensor heave in cm.                                         */
    uint16_t heading;                        /* Sensor heading in 0.01 degrees.                             */
} emx_datagram_attitude_data;


/* EMX Attitude Datagram */
typedef struct {
    emx_datagram_attitude_info *info;        /* Pointer to the Attitude datagram info.                      */
    emx_datagram_attitude_data *data;        /* Array of num_entries (N) attitude data.                     */
    int8_t sensor_system_descriptor;         /* The sensor system descriptor shows which sensor the data is */
                                             /*  derived from, and which of the sensor's data have been     */
                                             /*  used in real time by bit coding:                           */
                                             /*    xx00 xxxx - Motion sensor number 1.                      */
                                             /*    xx01 xxxx - Motion sensor number 2.                      */
                                             /*    xxxx xxx1 - Heading from the sensor is active.           */
                                             /*    xxxx xx0x - Roll from the sensor is active.              */
                                             /*    xxxx x0xx - Pitch from the sensor is active.             */
                                             /*    xxxx 0xxx - Heave from the sensor is active.             */
} emx_datagram_attitude;


/* EMX Network Attitude Velocity Info (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_entries;                    /* Number of entries = N (1-).                                 */
    int8_t sensor_system_descriptor;         /* The sensor system descriptor shows which sensor the data is */
                                             /*  derived from, and which of the sensor's data have been     */
                                             /*  used in real time by bit coding:                           */
                                             /*    0x10 xxxx - Attitude velocity sensor 1 (UDP5).           */
                                             /*    0x11 xxxx - Attitude velocity sensor 2 (UDP6).           */
                                             /*    01xx xxxx - Velocity from the sensor is active.          */
                                             /*    0xxx xxx1 - Heading from the sensor is active.           */
                                             /*    0xxx xx0x - Roll from the sensor is active.              */
                                             /*    0xxx x0xx - Pitch from the sensor is active.             */
                                             /*    0xxx 0xxx - Heave from the sensor is active.             */
                                             /*    1111 1111 - (-1) function is not used.                   */
    uint8_t spare;                           /* Spare.                                                      */
} emx_datagram_attitude_network_info;


/* EMX Network Attitude Velocity Data Info (11 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t record_time;                    /* Time in milliseconds since record start.                    */
    int16_t roll;                            /* Sensor roll in 0.01 degrees.                                */
    int16_t pitch;                           /* Sensor pitch in 0.01 degrees.                               */
    int16_t heave;                           /* Sensor heave in cm.                                         */
    uint16_t heading;                        /* Sensor heading in 0.01 degrees.                             */
    uint8_t num_bytes_input;                 /* Number of bytes of input datagram (Nx) (1-254).             */
} emx_datagram_attitude_network_data_info;


/* EMX Network Attitude Velocity Data */
typedef struct {
    emx_datagram_attitude_network_data_info *info; /* Pointer to attitude network data info.                */
    uint8_t *message;                        /* Pointer to attitude network input message.  The following   */
                                             /*  proprietary formats are supported.  The header can be      */
                                             /*  used for identification.                                   */
                                             /*   Seatex Binary format 11:  q (ASCII)                       */
                                             /*   Seatex Binary format 23:  AAh 51h                         */
                                             /*   Seatex Binary format 26:                                  */
                                             /*   POS-MV GRP 102/103:       $GRP102 or $GRP103 (ASCII)      */
                                             /*   Code Octopus MCOM:        E8h                             */
                                             /*  An extra byte is added at the end of the input datagram if */
                                             /*  needed for alignment.                                      */
} emx_datagram_attitude_network_data;


/* EMX Network Attitude Velocity Datagram */
typedef struct {
    emx_datagram_attitude_network_info *info;/* Pointer to attitude network info.                           */
    uint8_t *data;                           /* Array of num_entries (N) of data.  Use the function         */
                                             /*  emx_get_attitude_network_data to parse.                    */
} emx_datagram_attitude_network;


/* EMX Clock Info (9 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t date;                           /* Date = year*10000 + month*100 + day.                        */
                                             /*  Date from external clock input.                            */
    uint32_t time_ms;                        /* Time since midnight in milliseconds.                        */
                                             /*  Time from external clock input.                            */
    uint8_t PPS;                             /* 1 PPS use (0 = inactive); sync to 1PPS signal.              */
} emx_datagram_clock_info;


/* EMX Clock Datagram */
typedef struct {
    emx_datagram_clock_info *info;           /* Pointer to Clock datagram info.                             */
} emx_datagram_clock;


/* EMX Depth (Pressure) or Height Info (5 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int32_t height;                          /* Height in cm.                                               */
    uint8_t height_type;                     /* Height type (0-200).                                        */
                                             /*  0 : The height is derived from the GGK or GGA datagram     */
                                             /*      and is the height of the water level at the vertical   */
                                             /*      datum (possibly motion corrected).                     */
                                             /*  Height is derived from the active position system only.    */
                                             /*  1-99 : The height type is as given in the depth datagram.  */
                                             /*  100  : The input is depth taken from the depth datagram.   */
                                             /*  200  : Input from depth sensor.                            */
} emx_datagram_height_info;


/* EMX Depth (Pressure) or Height Datagram */
typedef struct {
    emx_datagram_height_info *info;          /* Pointer to Height datagram info.                            */
} emx_datagram_height;


/* EMX Heading Info (2 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_entries;                    /* Number of entries = N (1-).                                 */
} emx_datagram_heading_info;


/* EMX Heading Data (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t record_time;                    /* Time in milliseconds since record start.                    */
    uint16_t heading;                        /* Heading in 0.01 degrees.                                    */
} emx_datagram_heading_data;


/* EMX Heading Datagram */
typedef struct {
    emx_datagram_heading_info *info;         /* Pointer to Heading datagram info.                           */
    emx_datagram_heading_data *data;         /* Array of num_entries (N) heading data.                      */
    uint8_t heading_indicator;               /* Heading indicator (0 - Not active).                         */
} emx_datagram_heading;


/* EMX Position Info (18 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    int32_t latitude;                        /* Latitude in deg*20000000 (negative if southern hemisphere). */
    int32_t longitude;                       /* Longitude in deg*10000000 (negative if western hemisphere). */
                                             /* The following three variables: position fix quality, vessel */
                                             /*  speed and course, are only valid if available as input.    */
    uint16_t position_fix_quality;           /* Measure of position fix quality in cm.                      */
    uint16_t vessel_speed;                   /* Speed of vessel over ground in cm/s.                        */
    uint16_t vessel_course;                  /* Course of vessel over ground in 0.01 degrees.               */
    uint16_t vessel_heading;                 /* Heading of vessel in 0.01 degrees.                          */

    uint8_t position_system;                 /* Position system descriptor (1-254).                         */
                                             /*  xxxx xx01 - Positioning system no 1.                       */
                                             /*  xxxx xx10 - Positioning system no 2.                       */
                                             /*  xxxx xx11 - Positioning system no 3.                       */
                                             /*  10xx xxxx - Positioning system active, system time used.   */
                                             /*  11xx xxxx - Positioning system active, input data used.    */
                                             /*  xxxx 1xxx - Position may have to be derived from the       */
                                             /*    input datagram which is then in SIMRAD 90 format.        */

    uint8_t bytes_in_input;                  /* Number of bytes in input datagram.                          */
} emx_datagram_position_info;


/* EMX Position Datagram */
typedef struct {
    emx_datagram_position_info *info;        /* Pointer to Position datagram info.                          */
    uint8_t *message;                        /* Pointer to Position input message.                          */
} emx_datagram_position;


/* EMX Single Beam Depth Info (13 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t date;                           /* Date from input datagram.                                   */
    uint32_t time_ms;                        /* Time since midnight (from input datagram if available).     */
    uint32_t depth;                          /* Echo sounder depth from waterline in cm.                    */
    char source;                             /* Source identifier (S, T, 1, 2, or 3).                       */
} emx_datagram_sb_depth_info;


/* EMX Single Beam Depth Datagram */
typedef struct {
    emx_datagram_sb_depth_info *info;        /* Pointer to Single Beam Depth datagram info.                 */
} emx_datagram_sb_depth;


/* EMX Tide Info (11 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t date;                           /* Date from input datagram.                                   */
    uint32_t time_ms;                        /* Time since midnight (from input datagram if available).     */
    int16_t tide_offset;                     /* Tide offset in cm.                                          */
    int8_t spare;                            /* Spare.                                                      */
} emx_datagram_tide_info;


/* EMX Tide Datagram */
typedef struct {
    emx_datagram_tide_info *info;            /* Pointer to Tide datagram info.                              */
} emx_datagram_tide;


/* EMX Surface Sound Speed Info (2 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_samples;                    /* Number of entries = N (1-).                                 */
} emx_datagram_sssv_info;


/* EMX Surface Sound Speed Data (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t record_time;                    /* Time in seconds since record start.                         */
    uint16_t sound_speed;                    /* Sound speed in dm/s (14000-15999).                          */
} emx_datagram_sssv_data;


/* EMX Surface Sound Speed Datagram */
typedef struct {
    emx_datagram_sssv_info *info;            /* Pointer to SSSV datagram info.                              */
    emx_datagram_sssv_data *data;            /* Array of num_samples (N) surface sound speed records.       */
} emx_datagram_sssv;


/* EMX Sound Speed Profile Info (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t date;                           /* Date (when profile was made) = year*10000+month*100+day.    */
    uint32_t time_ms;                        /* Time (when profile was made) since midnight in ms.          */
    uint16_t num_samples;                    /* Number of entries = N (1-).                                 */
    uint16_t depth_resolution;               /* Depth resolution in cm (1-254).                             */
} emx_datagram_svp_info;


/* EMX Sound Speed Profile Data (8 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t depth;                          /* Depth in units of depth_resolution.                         */
    uint32_t sound_speed;                    /* Sound speed in dm/s (14000-17000).                          */
} emx_datagram_svp_data;


/* EMX Sound Speed Profile Datagram */
typedef struct {
    emx_datagram_svp_info *info;             /* Pointer to SVP datagram info.                               */
    emx_datagram_svp_data *data;             /* Array of sound speed records.                               */
} emx_datagram_svp;


/* EMX EM3000 Sound Speed Profile Info (12 Bytes) */
typedef emx_datagram_svp_info emx_datagram_svp_em3000_info;


/* EMX EM3000 Sound Speed Profile Data (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t depth;                          /* Depth in units of depth_resolution.                         */
    uint16_t sound_speed;                    /* Sound speed in dm/s (14000-17000).                          */
} emx_datagram_svp_em3000_data;


/* EMX Sound Speed Profile Datagram */
typedef struct {
    emx_datagram_svp_em3000_info *info;      /* Pointer to SVP datagram info.                               */
    emx_datagram_svp_em3000_data *data;      /* Array of sound speed records.                               */
} emx_datagram_svp_em3000;


/* EMX KM SSP Output Datagram */
typedef struct {
    char *data;                              /* Pointer to input datagram starting with sentence formatter  */
                                             /*  and ending with comment.                                   */
    int num_bytes;                           /* Length of input datagram.                                   */
} emx_datagram_km_ssp_output;


/* EMX Installation Parameters Info (2 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t serial_number2;                 /* Secondary system serial number (100 - ).                    */
} emx_datagram_install_params_info;


/* EMX Installation Parameters Datagram */
typedef struct {
    emx_datagram_install_params_info *info;  /* Pointer to Install Parameters datagram info.                */
    int text_length;                         /* Length of text string.                                      */
    char *text;                              /* Pointer to install parameters text.                         */
                                             /*  NOTE: This string may NOT be nul-terminated.               */
} emx_datagram_install_params;


/* EMX Installation Parameters Stop and Remote Datagrams */
typedef emx_datagram_install_params emx_datagram_install_params_stop;
typedef emx_datagram_install_params emx_datagram_install_params_remote;


/* EMX Runtime Parameters Info (33 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t operator_station_status;         /* Operator station status.                                    */

    uint8_t pu_status;                       /* Processing unit (CPU) status:                               */
                                             /*                                                             */
                                             /*  Bit         Function                        Model          */
                                             /*  xxxx xxx1 - Communication error with BSP    All but ME70BO */
                                             /*               (or CBMF)                                     */
                                             /*  xxxx xx1x - Communication error with        All but EM2040 */
                                             /*               Sonar Head or Transceiver       or ME70BO     */
                                             /*              Communication error with        EM2040/EM2040C */
                                             /*               slave PU                                      */
                                             /*              Problem with communication      ME70BO         */
                                             /*               with ME70                                     */
                                             /*  xxxx x1xx - Attitude not valid for this     All            */
                                             /*               ping                                          */
                                             /*  xxxx 1xxx - Heading not valid for this ping All            */
                                             /*  xxx1 xxxx - System clock has not been set   All            */
                                             /*               since power up                                */
                                             /*  xx1x xxxx - External trigger signal not     All but ME70BO */
                                             /*               detected                                      */
                                             /*  x1xx xxxx - CPU temperature warning         All but EM1002 */
                                             /*              Hull Unit not responding        EM1002         */
                                             /*  1xxx xxxx - Attitude velocity data not      EM122, EM302   */
                                             /*               valid for this ping             EM710, EM2040 */
                                             /*                                               EM2040C       */

    uint8_t bsp_status;                      /* BSP status.                                                 */
    uint8_t head_or_tx_status;               /* Sonar head or transceiver status.                           */

    uint8_t mode;                            /* Ping mode (0-):                                             */
                                             /*  EM3000                                                     */
                                             /*   xxxx 0000 - Nearfield (4 degree).                         */
                                             /*   xxxx 0001 - Normal (1.5 degree).                          */
                                             /*   xxxx 0010 - Target detect.                                */
                                             /*  EM3002                                                     */
                                             /*   xxxx 0000 - Wide Tx beamwidth (4 degree).                 */
                                             /*   xxxx 0001 - Normal Tx beamwidth (1.5 degree).             */
                                             /*  EM2000, EM710, EM1002, EM300, EM302, EM120 and EM122       */
                                             /*   xxxx 0000 - Very shallow.                                 */
                                             /*   xxxx 0001 - Shallow.                                      */
                                             /*   xxxx 0010 - Medium.                                       */
                                             /*   xxxx 0011 - Deep.                                         */
                                             /*   xxxx 0100 - Very deep.                                    */
                                             /*   xxxx 0101 - Extra deep.                                   */
                                             /*  EM2040                                                     */
                                             /*   xxxx 0000 - 200 kHz.                                      */
                                             /*   xxxx 0001 - 300 kHz.                                      */
                                             /*   xxxx 0010 - 400 kHz.                                      */
                                             /*  Tx Pulse Form (EM2040, EM710, EM302, EM122)                */
                                             /*   xx00 xxxx - CW.                                           */
                                             /*   xx01 xxxx - Mixed.                                        */
                                             /*   xx10 xxxx - FM.                                           */
                                             /*  Frequency (EM2040C): Frequency (Hz) = 180 + 10 * k         */
                                             /*   xxx0 0000 - k = 0 (180 kHz).                              */
                                             /*   xxx0 0001 - k = 1 (190 kHz).                              */
                                             /*   xxx1 0110 - k = 22 (400 kHz).                             */
                                             /*  Tx Pulse Form (EM2040C)                                    */
                                             /*   xx0x xxxx - CW.                                           */
                                             /*   xx1x xxxx - FM.                                           */
                                             /*  Dual Swath Mode (EM2040, EM710, EM302, EM122)              */
                                             /*   00xx xxxx - Dual swath = Off.                             */
                                             /*   01xx xxxx - Dual swath = Fixed.                           */
                                             /*   10xx xxxx - Dual swath = Dynamic.                         */

    uint8_t filter_id;                       /* Filter identifier (0-256):                                  */
                                             /*  xxxx xx00 - Spike filter set to Off.                       */
                                             /*  xxxx xx01 - Spike filter is set to Weak.                   */
                                             /*  xxxx xx10 - Spike filter is set to Medium.                 */
                                             /*  xxxx xx11 - Spike filter is set to Strong.                 */
                                             /*  xxxx x1xx - Slope filter is on.                            */
                                             /*  xxxx 1xxx - Sector tracking or Robust Bottom Detection     */
                                             /*              (EM3000) is on.                                */
                                             /*  0xx0 xxxx - Range gates have Normal size.                  */
                                             /*  0xx1 xxxx - Range gates are Large.                         */
                                             /*  1xx0 xxxx - Range gates are Small.                         */
                                             /*  xx1x xxxx - Aeration filter is on.                         */
                                             /*  x1xx xxxx - Interference filter is on.                     */

    uint16_t min_depth;                      /* Minimum depth in meters.                                    */
    uint16_t max_depth;                      /* Maximum depth in meters.                                    */

    uint16_t absorption;                     /* Absorption coefficient in 0.01 dB/km (1-20000).  The used   */
                                             /*  absorption coefficient should be derived from raw range    */
                                             /*  and angle 78 datagram or, for older systems, from the      */
                                             /*  seabed image or central beams echogram datagram if it is   */
                                             /*  automatically updated with changing depth or frequency.    */
                                             /*  This absorption coefficient is valid at the following      */
                                             /*  frequencies:                                               */
                                             /*   EM120/EM122 : 12 kHz                                      */
                                             /*   EM300/EM302 : 31.5 kHz                                    */
                                             /*   EM710 : 85 kHz                                            */
                                             /*   ME70BO : 85 kHz                                           */
                                             /*   EM1002 : 95 kHz                                           */
                                             /*   EM2000 : 200 kHz                                          */
                                             /*   EM3000/EM3002 : 300 kHz                                   */
                                             /*   EM2040/EM2040C : 300 kHz                                  */

    uint16_t tx_pulse_length;                /* Transmit pulse length in microseconds.                      */
    uint16_t tx_beamwidth;                   /* Transmit beamwidth in 0.1 degree.                           */
    int8_t tx_power;                         /* Transmit power re maximum in dB.                            */
    uint8_t rx_beamwidth;                    /* Receive beamwidth in 0.1 degree (5-80).                     */
    uint8_t rx_bandwidth;                    /* Receive bandwidth in 50 Hz resolution (1-255).              */
    uint8_t rx_fixed_gain;                   /* Mode 2 or receiver fixed gain setting in dB (0-50).         */
    uint8_t tvg_crossover;                   /* TVG law crossover angle in degrees (2-30).                  */

    uint8_t sound_speed_source;              /* Source of sound speed at transducer:                        */
                                             /*  0000 0000 - From real-time sensor.                         */
                                             /*  0000 0001 - Manually entered by operator.                  */
                                             /*  0000 0010 - Interpolated from currently used SVP.          */
                                             /*  0000 0011 - Calculated by ME70BO TRU.                      */
                                             /*  xxx1 xxxx - Extra detections enabled.                      */
                                             /*  xx1x xxxx - Sonar mode enabled.                            */
                                             /*  x1xx xxxx - Passive mode enabled.                          */
                                             /*  1xxx xxxx - 3D scanning enabled.                           */

    uint16_t max_port_swath;                 /* Maximum port swath width in meters (10-30000).              */

    uint8_t beam_spacing;                    /* Beam spacing (0-3):                                         */
                                             /*  0 - Determined by beamwidth (FFT beamformer of EM3000).    */
                                             /*  1 - Equidistant (in between for EM122 and EM302).          */
                                             /*  2 - Equiangle.                                             */
                                             /*  3 - High-density equidistant (in between for EM2000,       */
                                             /*      EM120, EM 300, and EM 1002).                           */
                                             /* For EM3002:                                                 */
                                             /*  1xxx xxxx - Two sonar heads are connected.                 */
                                             /*  Bits 0-3 - Head 1.                                         */
                                             /*  Bits 4-6 - Head 2.                                         */

    uint8_t max_port_coverage;               /* Maximum port coverage in degrees (10-110).                  */

    uint8_t yaw_pitch_mode;                  /* Yaw and pitch stabilization mode:                           */
                                             /*  xxxx xx00 - No yaw stabilization.                          */
                                             /*  xxxx xx01 - Yaw stabilization to survey line heading (not  */
                                             /*              used).                                         */
                                             /*  xxxx xx10 - Yaw stabilization to mean vessel heading.      */
                                             /*  xxxx xx11 - Yaw stabilization to manually entered heading. */
                                             /*  xxxx 00xx - Heading filter, hard.                          */
                                             /*  xxxx 01xx - Heading filter, medium.                        */
                                             /*  xxxx 10xx - Heading filter, weak.                          */
                                             /*  1xxx xxxx - Pitch stabilization is on.                     */

    uint8_t max_stbd_coverage;               /* Maximum stbd coverage in degrees (10-110).                  */
    uint16_t max_stbd_swath;                 /* Maximum stbd swath width in meters (10-30000).              */

                                             /* For EM122, EM302, EM710, or EM2040 the next two fields are: */
                                             /*  Transmit along tilt in 0.1 deg, and                        */
                                             /*  Filter identifier 2.                                       */
                                             /* For all other sonar types these fields are:                 */
                                             /*  Durotong speed in dm/s, and                                */
                                             /*  HiLo freq absorption coef ratio.                           */
    int16_t tx_along_tilt;
    uint8_t filter_id2;

} emx_datagram_runtime_params_info;


/* EMX Runtime Parameters Datagram */
typedef struct {
    emx_datagram_runtime_params_info *info;  /* Pointer to the Runtime Parameters datagram info.            */
} emx_datagram_runtime_params;


/* EMX Extra Parameters Info (2 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t content;                        /* Content identifier:                                         */
                                             /*  1 - Calib.txt file.                                        */
                                             /*  2 - Log all heights.                                       */
                                             /*  3 - Sound velocity at transducer.                          */
                                             /*  4 - Sound velocity profile.                                */
                                             /*  5 - Multicast RX status.                                   */
                                             /*  6 - Bscorr.txt file for backscatter corrections.           */
} emx_datagram_extra_params_info;


/* EMX Extra Parameters BS CORR */
typedef struct {
    uint16_t num_chars;                      /* Length of the text message.                                 */
    char *text;                              /* Pointer to bs_corr.txt text message.                        */
} emx_datagram_extra_params_bs_corr;


/* EMX Extra Parameters Data */
typedef union {
    emx_datagram_extra_params_bs_corr bs_corr;
    /* TODO: Add content 1-5 here. */
} emx_datagram_extra_params_data;


/* EMX Extra Parameters Datagram */
typedef struct {
    emx_datagram_extra_params_info *info;    /* Pointer to Extra Params datagram info.                      */
    emx_datagram_extra_params_data data;     /* Extra Params datagram data.                                 */
} emx_datagram_extra_params;


/* EMX PU Output Info (88 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t udp_port1;                      /* UDP port 1 (command datagrams).                             */
    uint16_t udp_port2;                      /* UDP port 2 (sensor datagrams except motion sensor).         */
    uint16_t udp_port3;                      /* UDP port 3 (first motion sensor).                           */
    uint16_t udp_port4;                      /* UDP port 4 (second motion sensor).                          */
    uint32_t system_descriptor;              /* System descriptor (information for internal use).           */
    char pu_software_version[16];            /* PU software version.                                        */
    char bsp_software_version[16];           /* BSP software version.                                       */
    char transceiver1_version[16];           /* Sonar head/transceiver 1 software version.                  */
    char transceiver2_version[16];           /* Sonar head/transceiver 2 software version.                  */
    uint32_t host_ip_address;                /* Host IP address.                                            */
    uint8_t tx_opening_angle;                /* TX opening angle (0,1,2, or 4).                             */
    uint8_t rx_opening_angle;                /* RX opening angle (1,2, or 4).                               */
    uint8_t spare[6];                        /* Future use.                                                 */
} emx_datagram_pu_output_info;


/* EMX PU Output Datagram */
typedef struct {
    emx_datagram_pu_output_info *info;       /* Pointer to PU Output datagram info.                         */
} emx_datagram_pu_output;


/* EMX PU Status Info (69 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t ping_rate;                      /* Ping rate in 0.01 Hz.                                       */
    uint16_t ping_counter;                   /* Ping counter of latest ping.                                */
    uint32_t swath_distance;                 /* Distance between swaths in 10%.                             */
    uint32_t status_udp_port_2;              /* Sensor input status, UDP port 2.                            */
    uint32_t status_serial_port_1;           /* Sensor input status, serial port 1.                         */
    uint32_t status_serial_port_2;           /* Sensor input status, serial port 2.                         */
    uint32_t status_serial_port_3;           /* Sensor input status, serial port 3.                         */
    uint32_t status_serial_port_4;           /* Sensor input status, serial port 4.                         */
    int8_t pps_status;                       /* 0 or negative indicates bad quality, positive OK.           */
    int8_t position_status;                  /* 0 or negative indicates bad quality, positive OK.           */
    int8_t attitude_status;                  /* 0 or negative indicates bad quality, positive OK.           */
    int8_t clock_status;                     /* 0 or negative indicates bad quality, positive OK.           */
    int8_t heading_status;                   /* 0 or negative indicates bad quality, positive OK.           */
    uint8_t pu_status;                       /* 0 = off, 1 = active, 2 = simulator                          */
    uint16_t heading;                        /* Last received heading in 0.01 deg.                          */
    int16_t roll;                            /* Last received roll in 0.01 deg.                             */
    int16_t pitch;                           /* Last received pitch in 0.01 deg.                            */
    int16_t heave;                           /* Last received heave at sonar head in cm.                    */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s.                          */
    uint32_t depth;                          /* Last received depth in cm.                                  */
    int16_t velocity;                        /* Along-ship velocity in 0.01 m/s.                            */
    uint8_t velocity_status;                 /* Velocity sensor status.                                     */
    uint8_t ramp;                            /* Mammal protection ramp.                                     */
    int8_t bs_oblique;                       /* Backscatter at oblique angle in dB.                         */
    int8_t bs_normal;                        /* Backscatter at normal incidence in dB.                      */
    int8_t gain;                             /* Fixed gain in dB.                                           */

    /* Added after Jan. 2004. */
    uint8_t depth_normal;                    /* Depth to normal incidence in meters.                        */
    uint16_t range_normal;                   /* Range to normal incidence in meters.                        */
    uint8_t port_coverage;                   /* Port coverage in degrees.                                   */
    uint8_t stbd_coverage;                   /* Stbd coverage in degrees.                                   */
    uint16_t sound_speed_svp;                /* Sound speed at transducer found from profile in dm/s.       */
    int16_t yaw_stabilization;               /* Yaw stabilization angle or tilt used at 3D scanning in dm/s.*/

    /* Added after Jan. 2005. */
    int16_t port_coverage2;                  /* Port coverage in deg or across-ship velocity in 0.01 m/s.   */
    int16_t stbd_coverage2;                  /* Stbd coverage in deg or downward velocity in 0.01 m/s.      */
    int8_t cpu_temp;                         /* EM2040 CPU temperature in deg C (0 if not used).            */
} emx_datagram_pu_status_info;


/* EMX PU Status Datagram */
typedef struct {
    emx_datagram_pu_status_info *info;       /* Pointer to PU Status datagram info.                         */
} emx_datagram_pu_status;


/* EMX PU BIST Result Info (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t test_number;                    /* Test number.                                                */
    int16_t test_result_status;              /* Test result status.                                         */
} emx_datagram_pu_bist_result_info;


/* EMX PU BIST Result Datagram */
typedef struct {
    emx_datagram_pu_bist_result_info *info;  /* Pointer to PU BIST Result datagram info.                    */
    int text_length;                         /* Length of text string.                                      */
    char *text;                              /* Pointer to BIST Result text.                                */
} emx_datagram_pu_bist_result;


/* EMX Mechanical Tilt Info (2 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t num_entries;                    /* Number of entries, N (1-).                                  */
} emx_datagram_tilt_info;


/* EMX Mechanical Tilt Data (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t record_time;                    /* Time in milliseconds since record start.                    */
    int16_t tilt;                            /* Tilt in 0.01 degrees.                                       */
} emx_datagram_tilt_data;


/* EMX Mechanical Tilt Datagram */
typedef struct {
    emx_datagram_tilt_info *info;            /* Pointer to Mechanical Tilt datagram info.                   */
    emx_datagram_tilt_data *data;            /* Array of num_entries (N) tilt data.                         */
} emx_datagram_tilt;


/* EMX HISAS Status Info (100 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t version_id;                     /* Version ID of the status output datagram:                   */
                                             /*  0 - Older version,                                         */
                                             /*  1 - Current version.                                       */
    uint16_t ping_rate;                      /* Ping rate in Hz * 100.                                      */
    uint32_t ping_counter;                   /* Ping counter of latest ping.                                */
    uint32_t pu_idle_count;                  /* PU idle count in %.                                         */
    uint32_t sensor_input_status;            /* Sensor input status, UDP port 2.                            */
    int8_t pps_status;                       /* PPS status.                                                 */
    int8_t clock_status;                     /* Clock status.                                               */
    int8_t attitude_status;                  /* Attitude status.                                            */
    uint8_t trigger_status;                  /* Trigger counter.                                            */
    uint8_t pu_modes;                        /* PU mode: 0 - Off, 1 - Active, 2 - Simulator.                */
    uint8_t logger_status;                   /* Logger status: 0 - Off, 1 - Active.                         */
    uint16_t yaw;                            /* Last received yaw in 0.01 degree.                           */
    uint16_t roll;                           /* Last received roll in 0.01 degree.                          */
    uint16_t pitch;                          /* Last received pitch in 0.01 degree.                         */
    uint16_t heave;                          /* Last received heave in cm.                                  */
    uint16_t sound_speed;                    /* Sound speed at transducer in dm/s.                          */
    uint32_t log_file_size_port;             /* Current log file size for port side in bytes.               */
    uint32_t log_file_size_stbd;             /* Current log file size for stbd side in bytes.               */
    uint32_t free_space_port;                /* Free space on port side disk in Mbytes.                     */
    uint32_t free_space_stbd;                /* Free space on stbd side disk in Mbytes.                     */
    uint16_t cbmf_1_status;                  /* CBMF 1 status.                                              */
    uint16_t cbmf_2_status;                  /* CBMF 2 status.                                              */
    uint32_t tru_board_status;               /* TRU board status.                                           */
    uint32_t pu_status;                      /* PU status.                                                  */
    int16_t cpu_temp;                        /* CPU temperature in degrees C.                               */
    int16_t lptx_temp;                       /* LPTX temperature in degrees C.                              */
    int16_t lprx_temp;                       /* LPRX temperature in degrees C.                              */
    int16_t hdd_temp;                        /* Hard disk container temperature in degrees C.               */
    int16_t last_nav_depth_input;            /* Last navigation depth input.                                */
    int16_t last_nav_altitude_input;         /* Last navigation altitude input.                             */
    uint8_t transmitters_passive;            /* Transmitters passive.                                       */
    uint8_t external_trigger_enabled;        /* External trigger enabled.                                   */
    uint8_t sidescan_bathy_enabled;          /* Sidescan bathymetry enabled.                                */
    uint8_t in_mission_sas_enabled;          /* In mission SAS enabled.                                     */
    uint8_t spare[24];                       /* Spare.                                                      */
} emx_datagram_hisas_status_info;


/* EMX HISAS Status Datagram */
typedef struct {
    emx_datagram_hisas_status_info *info;    /* Pointer to HISAS Status datagram info.                      */
} emx_datagram_hisas_status;


/* EMX Sidescan Channel Information (128 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t type_of_channel;                 /* Channel Type: Port=1, Stbd=2.                               */
    uint8_t sub_channel_number;              /* Index for which channel information structure this is.      */
    uint16_t correction_flags;               /* Always 1.                                                   */
    uint16_t uni_polar;                      /* Always 1.                                                   */
    uint16_t bytes_per_sample;               /* Bytes per sample: 2=16-bit, 4=float, 8=float real/imaginary.*/
    uint8_t spare1[4];                       /* Spare.                                                      */
    char channel_name[16];                   /* Text describing channel, e.g., "Port 500".                  */
    uint8_t spare2[4];                       /* Spare.                                                      */
    float frequency;                         /* Center transmit frequency in Hz.                            */
    float horiz_beam_angle;                  /* Horizontal beam width in degrees.                           */
    float tilt_angle;                        /* Sonar tilt angle from horizontal in degrees.                */
    float beam_width;                        /* Vertical 3-dB beam width in degrees.                        */
    float offset_x;                          /* Orientation of positive X is to starboard.                  */
    float offset_y;                          /* Orientation of positive Y is forward.                       */
    float offset_z;                          /* Orientation of positive Z is down.                          */
    float offset_yaw;                        /* Orientation of positive yaw is turn to right (-180 - 180).  */
    float offset_pitch;                      /* Orientation of positive pitch is nose up in degrees.        */
    float offset_roll;                       /* Orientation of positive roll is lean to stbd in degrees.    */
    uint8_t spare3[56];                      /* Spare.                                                      */
} emx_datagram_sidescan_status_channel;


/* EMX Sidescan Status Info (xx Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t file_format;                     /* Set to 123 (0x7B).                                          */
    uint8_t system_type;                     /* Type of system used to record this data; set to 1.          */
    uint8_t spare1[32];                      /* Spare.                                                      */
    uint16_t sonar_type;                     /* Sonar type: 48 - Kongsberg SAS.                             */
    uint8_t spare2[128];                     /* Spare.                                                      */
    uint16_t nav_units;                      /* Always 3.                                                   */
    uint16_t num_channels;                   /* Number of sidescan channels (0 - 6).                        */
    uint8_t spare3[88];                      /* Spare.                                                      */
    emx_datagram_sidescan_status_channel channel[6];
    uint8_t spare4;                          /* Spare.                                                      */
} emx_datagram_sidescan_status_info;


/* EMX Sidescan Status Datagram */
typedef struct {
    emx_datagram_sidescan_status_info *info; /* Pointer to the Sidescan Status datagram info.               */
} emx_datagram_sidescan_status;


/* EMX Sidescan Data Info (256 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t magic_number;                   /* Must be set to 0xFACE.                                      */
    uint8_t header_type;                     /* Always 0.                                                   */
    uint8_t beam_number;                     /* Beam number.                                                */
    uint16_t num_channels;                   /* Number of channels in record.                               */
    uint8_t spare1[4];                       /* Spare.                                                      */
    uint16_t num_bytes_record;               /* Total byte count for this ping including this ping header.  */
    uint8_t spare2[2];                       /* Spare.                                                      */
    uint16_t year;                           /* Ping year.                                                  */
    uint8_t month;                           /* Ping month (1 -12).                                         */
    uint8_t day;                             /* Ping day (1 - 31).                                          */
    uint8_t hour;                            /* Ping hour.                                                  */
    uint8_t minute;                          /* Ping minute.                                                */
    uint8_t second;                          /* Ping seconds.                                               */
    uint8_t hseconds;                        /* Ping hundredths of seconds (0-99).                          */
    uint8_t spare3[6];                       /* Spare.                                                      */
    uint32_t ping_number;                    /* Counts consecutively from 0 and increments for each update. */
    float sound_velocity_two_way;            /* Two-way sound velocity in m/s.                              */
    uint8_t spare4[36];                      /* Spare.                                                      */
    float sound_velocity;                    /* One-way sound velocity in m/s.                              */
    uint8_t spare5[72];                      /* Spare.                                                      */
    uint8_t fix_time_hour;                   /* Hour of most recent nav update.                             */
    uint8_t fix_time_minute;                 /* Minute of most recent nav update.                           */
    uint8_t fix_time_second;                 /* Second of most recent nav update.                           */
    uint8_t fix_time_hsecond;                /* Hundredth of a second of most recent nav update.            */
    float sensor_speed;                      /* Speed of towfish in knots.                                  */
    uint8_t spare6[4];                       /* Spare.                                                      */
    double sensor_lat;                       /* Sensor latitude in degrees.                                 */
    double sensor_lon;                       /* Sensor longitude in degrees.                                */
    uint8_t spare7[16];                      /* Spare.                                                      */
    float sensor_depth;                      /* Distance from sea surface to sensor in meters.              */
    float sensor_altitude;                   /* Distance from towfish to the sea floor in meters.           */
    float sensor_aux_altitude;               /* Auxiliary altitude in meters.                               */
    float sensor_pitch;                      /* Pitch in degrees (positive=nose up).                        */
    float sensor_roll;                       /* Roll in degrees (positive=roll to stbd).                    */
    float sensor_heading;                    /* Sensor heading in degrees.                                  */
    uint8_t spare8[40];                      /* Spare.                                                      */
} emx_datagram_sidescan_data_info;


/* EMX Sidescan Data Channel Info (64 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t channel_number;                 /* Channel number (index into header record).                  */
    uint8_t spare1[2];                       /* Spare.                                                      */
    float slant_range;                       /* Slant range of the data in meters.                          */
    uint8_t spare2[8];                       /* Spare.                                                      */
    float time_duration;                     /* Amount of time in seconds recorded.                         */
    float seconds_per_ping;                  /* Amount of time in seconds from ping to ping.                */
    uint8_t spare3[18];                      /* Spare.                                                      */
    uint16_t num_samples;                    /* Number of samples that will follow this structure.          */
    uint8_t spare4[14];                      /* Spare.                                                      */
    int16_t weight;                          /* Weighting factor.                                           */
    uint8_t spare5[4];                       /* Spare.                                                      */
} emx_datagram_sidescan_data_channel_info;


/* EMX Sidescan Data Sample */
typedef union {
    float *f32;                              /* Pointer to a float type.                                    */
    uint16_t *u16;                           /* Pointer to an unsigned 16-byte type.                        */
    void *ptr;
} emx_datagram_sidescan_data_sample;


/* EMX Sidescan Data Channel */
typedef struct {
    emx_datagram_sidescan_data_channel_info *info;
    emx_datagram_sidescan_data_sample data;
    int bytes_per_sample;                    /* Bytes per sample: 2=16-bit, 4=float, 8=float real/imaginary.*/
} emx_datagram_sidescan_data_channel;


/* EMX Sidescan Data Datagram */
typedef struct {
    emx_datagram_sidescan_data_info *info;
    emx_datagram_sidescan_data_channel channel[6];
} emx_datagram_sidescan_data;


/* EMX Navigation Output Info (112 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t data_type;                      /* Always 0xF00D.                                              */
    uint16_t bytes_per_element;              /* Always 64.                                                  */
    uint32_t num_elements;                   /* Number of elements.  Usually 1 (for future expansion).      */
    uint32_t date;                           /* Date from received telemetry data.                          */
    uint32_t time_ms;                        /* Time since midnight from received telemetry data.           */
    uint8_t spare1[32];                      /* Spare.                                                      */
    uint32_t time_offset;                    /* Time offset in ms to add to datagram time tag.  When only   */
                                             /*  one sub-record is present, this should normally be zero.   */
    double latitude;                         /* Latitude in degrees.                                        */
    double longitude;                        /* Longitude in degrees.                                       */
    float depth;                             /* Depth below surface in meters.                              */
    float heading;                           /* Heading in degrees.                                         */
    float pitch;                             /* Pitch angle in degrees.                                     */
    float roll;                              /* Roll angle in degrees.                                      */
    float velocity_forward;                  /* Velocity in forward direction in m/s.                       */
    float velocity_stbd;                     /* Velocity in starboard direction in m/s.                     */
    float velocity_down;                     /* Velocity in down direction in m/s.                          */
    float horizontal_uncertainty;            /* Standard deviation of horizontal position in meters.        */
    float altitude;                          /* Altitude above the sea floor in meters.                     */
    float sound_speed;                       /* Speed of sound in m/s.                                      */
    uint8_t spare2[2];                       /* Spare.                                                      */
    uint16_t data_validity;                  /* Data validity flags.  A '1' indicates valid data.           */
                                             /*  Bit 1 : Horizontal position (latitude, longitude).         */
                                             /*  Bit 2 : Depth.                                             */
                                             /*  Bit 3 : Heading.                                           */
                                             /*  Bit 4 : Pitch and roll.                                    */
                                             /*  Bit 5 : Horizontal velocity.                               */
                                             /*  Bit 6 : Vertical velocity.                                 */
                                             /*  Bit 7 : Std dev. of position.                              */
                                             /*  Bit 8 : Altitude.                                          */
                                             /*  Bit 9 : Speed of sound.                                    */
} emx_datagram_navigation_output_info;


/* EMX Navigation Output Datagram */
typedef struct {
    emx_datagram_navigation_output_info *info;
} emx_datagram_navigation_output;


/******************************* EMX DATA TYPES ******************************/

/* EMX Datagram */
typedef union {
    emx_datagram_depth                  depth;
    emx_datagram_xyz                    xyz;
    emx_datagram_depth_nominal          depth_nominal;
    emx_datagram_extra_detect           extra_detect;
    emx_datagram_central_beams          central_beams;
    emx_datagram_rra_101                rra_101;
    emx_datagram_rra_70                 rra_70;
    emx_datagram_rra_102                rra_102;
    emx_datagram_rra_78                 rra_78;
    emx_datagram_seabed_83              seabed_83;
    emx_datagram_seabed_89              seabed_89;
    emx_datagram_wc                     wc;
    emx_datagram_qf                     qf;
    emx_datagram_attitude               attitude;
    emx_datagram_attitude_network       attitude_network;
    emx_datagram_clock                  clock;
    emx_datagram_height                 height;
    emx_datagram_heading                heading;
    emx_datagram_position               position;
    emx_datagram_sb_depth               sb_depth;
    emx_datagram_tide                   tide;
    emx_datagram_sssv                   sssv;
    emx_datagram_svp                    svp;
    emx_datagram_svp_em3000             svp_em3000;
    emx_datagram_km_ssp_output          ssp_output;
    emx_datagram_install_params         install_params;
    emx_datagram_install_params_stop    install_params_stop;
    emx_datagram_install_params_remote  install_params_remote;
    emx_datagram_runtime_params         runtime_params;
    emx_datagram_extra_params           extra_params;
    emx_datagram_pu_output              pu_output;
    emx_datagram_pu_status              pu_status;
    emx_datagram_pu_bist_result         pu_bist_result;
    emx_datagram_tilt                   tilt;
    emx_datagram_hisas_status           hisas_status;
    emx_datagram_sidescan_status        sidescan_status;
    emx_datagram_sidescan_data          sidescan_data;
    emx_datagram_navigation_output      navigation_output;
} emx_datagram;


/* EMX Data */
typedef struct {
    emx_datagram_header header;
    emx_datagram datagram;
} emx_data;


/* Opaque EMX File Handle */
typedef struct emx_handle_struct emx_handle;


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

emx_handle * emx_open (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
int emx_close (emx_handle *);
emx_data * emx_read (emx_handle *) CPL_ATTRIBUTE_NONNULL_ALL;
void emx_print (FILE *, const emx_data *, const int) CPL_ATTRIBUTE_NONNULL (1);
void emx_set_ignore_wc (emx_handle *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
void emx_set_ignore_checksum (emx_handle *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
const uint8_t * emx_get_wc_rxbeam (emx_datagram_wc_rx_beam *, const uint8_t *) CPL_ATTRIBUTE_NONNULL_ALL;
const uint8_t * emx_get_attitude_network_data (emx_datagram_attitude_network_data *, const uint8_t *) CPL_ATTRIBUTE_NONNULL_ALL;
int emx_get_model (const uint16_t) CPL_ATTRIBUTE_CONST;
const char * emx_get_datagram_name (const uint8_t) CPL_ATTRIBUTE_RETURNS_NONNULL;
unsigned int emx_get_em3000d_sample_rate (const uint16_t, const int) CPL_ATTRIBUTE_PURE;
int emx_get_errno (const emx_handle *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int emx_identify (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void emx_set_debug (const int);


CPL_CLINKAGE_END

#endif /* EMX_READER_H */
