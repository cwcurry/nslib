/* kma_reader.h -- Header file for kma_reader.c

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


  HISTORY
  =======
  Updated to Rev. I - 25 Sept 2021.
  Updated to Rev. H - 14 Sept 2021.
  Updated to Rev. G - 18 May  2020.

  Specifications are from:
    EM datagrams on *.kmall format. Reg. no. 410224 Rev F.

  .kmall files are little endian.  Each datagram begins and ends with a four-byte
  datagram size in bytes.  The timestamp of the data may not be strictly sequential.

  NOTE: Spec says datagrams are 4-byte aligned, but that does not appear to be true.

  Supported multibeam sounders:
    EM710 and EM712
    EM2040
    EM2040C

  VCS - Vessel Coordinate System.
  ACS - Array Coordinate System.
  SCS - Surface Coordinate System.
  FCS - Fixed Coordinate System.

    This reader reads each datagram into a memory buffer and aliases the data
  structures to the buffer and therefore assumes a compiler that can pack a struct
  and a processor that is little-endian and can perform unaligned memory accesses. */

#ifndef KMA_READER_H
#define KMA_READER_H

#if defined (__cplusplus)
#include <cstdio>
#else
#include <stdio.h>
#endif

#define CPL_NEED_FIXED_WIDTH_T  /* Tell cpl_spec.h we need fixed-width types. */
#include "cpl_spec.h"


/******************************** DEFINITIONS ********************************/

/* KMA Datagram Types (dgmType) */
#define KMA_DATAGRAM_IIP  0x50494923   /* Installation parameters datagram.       */
#define KMA_DATAGRAM_IOP  0x504F4923   /* Runtime parameters datagram.            */
#define KMA_DATAGRAM_IBE  0x45424923   /* BIST error report datagram.             */
#define KMA_DATAGRAM_IBR  0x22544923   /* BIST reply datagram.                    */
#define KMA_DATAGRAM_IBS  0x53424923   /* BIST short reply datagram.              */
#define KMA_DATAGRAM_MRZ  0x5A524D23   /* Multibeam raw range and depth datagram. */
#define KMA_DATAGRAM_MWC  0x43574D23   /* Water column datagram.                  */
#define KMA_DATAGRAM_SPO  0x4F505323   /* Position datagram.                      */
#define KMA_DATAGRAM_SKM  0x4D4B5323   /* KM binary datagram.                     */
#define KMA_DATAGRAM_SVP  0x50565323   /* Sound velocity profile datagram.        */
#define KMA_DATAGRAM_SVT  0x54565323   /* Sound velocity transducer datagram.     */
#define KMA_DATAGRAM_SCL  0x4C435323   /* Clock datagram.                         */
#define KMA_DATAGRAM_SDE  0x45445323   /* Depth datagram.                         */
#define KMA_DATAGRAM_SHI  0x49485323   /* Height datagram.                        */
#define KMA_DATAGRAM_SHA  0x41485323   /* Heading (removed in Format Rev. C).     */
#define KMA_DATAGRAM_CPO  0x4F504323   /* Compatibility position sensor datagram. */
#define KMA_DATAGRAM_CHE  0x45484323   /* Compatibility heave datagram.           */
#define KMA_DATAGRAM_FCF  0x46434623   /* Calibration file (Added in Rev G).      */

/* KMA Null Types */
#define KMA_NULL_POSFIX                0xFFFF
#define KMA_NULL_LATLON                200
#define KMA_NULL_SPEED                -1
#define KMA_NULL_COURSE               -4
#define KMA_NULL_ELLIPSOIDAL_HEIGHT   -999

/* KMA Bottom Detection Type (detectionType) */
#define KMA_DETECT_TYPE_NORMAL         0
#define KMA_DETECT_TYPE_EXTRA          1
#define KMA_DETECT_TYPE_REJECTED       2

/* KMA Bottom Detection Method Type (detectionMethod) */
#define KMA_DETECT_METHOD_NONE         0
#define KMA_DETECT_METHOD_AMPLITUDE    1
#define KMA_DETECT_METHOD_PHASE        2

/* KMA Sensor Input Format Type (sensorInputFormat) */
#define KMA_SENSOR_KM_BINARY           1
#define KMA_SENSOR_EM3000              2
#define KMA_SENSOR_SAGEM               3
#define KMA_SENSOR_SEAPATH_11          4
#define KMA_SENSOR_SEAPATH_23          5
#define KMA_SENSOR_SEAPATH_26          6
#define KMA_SENSOR_POSMV_GRP102_103    7

/* KMA Signal Pulse Type (signalWaveForm) */
#define KMA_PULSE_TYPE_CW              0
#define KMA_PULSE_TYPE_FM_UP           1
#define KMA_PULSE_TYPE_FM_DOWN         2

/* KMA MWC RX Info (phaseFlag) */
#define KMA_MWC_RX_PHASE_OFF           0
#define KMA_MWC_RX_PHASE_LOW           1
#define KMA_MWC_RX_PHASE_HIGH          2

/* KMA Invalid Values */
#define KMA_INVALID_AMP               -128


/********************************* DATAGRAMS *********************************/

/* KMA Datagram Header (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t numBytesDgm;                       /* Datagram length in bytes.  Length field at beginning and end are        */
                                                /*  included (4 bytes each or 8 bytes total).                              */
                                                /*  NOTE: Some datagrams appear to be 2-byte aligned.                      */
    uint32_t dgmType;                           /* Multibeam datagram type definition, e.g. '#AAA'.                        */
    uint8_t dgmVersion;                         /* Datagram version.                                                       */
    uint8_t systemID;                           /* System ID. Parameter used for separating datagrams from different       */
                                                /*  echosounders if more than one system is connected to SIS/K-Controller. */
    uint16_t echoSounderID;                     /* Echo sounder identity, e.g. 122, 302, 304, 710, 712, 2040, 2045, 850.   */
    uint32_t time_sec;                          /* UTC time in seconds from the UNIX Epoch.                                */
    uint32_t time_nanosec;                      /* UTC nano-second component of time_sec.                                  */
} kma_datagram_header;


/* KMA Installation Parameters Data (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesCmnPart;                   /* Size in bytes of body part struct.                                      */
    uint16_t info;                              /* Information.  For future use.                                           */
    uint16_t status;                            /* Status.  For future use.                                                */
} kma_datagram_iip_data;


/* KMA Installation Parameters Datagram */
typedef struct {
    kma_datagram_iip_data *data;                /* Pointer to Install Parameters data.                                     */
    uint8_t *install_text;                      /* Install settings in text format.  Parameters separated by ';' and lines */
                                                /*  separated by ',' delimiter.  NOTE: This string is NOT nul-terminated.  */
} kma_datagram_iip;


/* KMA Runtime Parameters Data (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesCmnPart;                   /* Size in bytes of body part struct.                                      */
    uint16_t info;                              /* Information.  For future use.                                           */
    uint16_t status;                            /* Status.  For future use.                                                */
} kma_datagram_iop_data;


/* KMA Runtime Parameters Datagram */
typedef struct {
    kma_datagram_iop_data *data;                /* Pointer to Runtime Parameters data.                                     */
    uint8_t *runtime_text;                      /* Runtime settings in text format.  Parameters separated by ';' and lines */
                                                /*  separated by ',' delimiter.  NOTE: This string is NOT nul-terminated.  */
} kma_datagram_iop;


/* KMA BIST Data (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesCmnPart;                   /* Size in bytes of body part struct.                                      */
    uint8_t BISTInfo;                           /* BIST info: 0 - Last subset of the message, 1 - More messages to come.   */
    uint8_t BISTStyle;                          /* BIST style: 0 - Plain text, 1 - Use style sheet.                        */
    uint8_t BISTNumber;                         /* BIST number executed.                                                   */
    int8_t BISTStatus;                          /* BIST status:                                                            */
                                                /*  0 - BIST executed with no errors.                                      */
                                                /*  Positive number - Warning.                                             */
                                                /*  Negative number - Error.                                               */
} kma_datagram_b_data;


/* KMA BIST Datagram */
typedef struct {
    kma_datagram_b_data *data;                  /* Pointer to BIST data.                                                   */
    uint8_t *bist_text;                         /* Result of the BIST.  Starts with synopsis of the result followed by     */
                                                /*  detailed explanation.  NOTE: This string is NOT nul-terminated.        */
} kma_datagram_b;


/* KMA BIST Error Report Datagram */
typedef kma_datagram_b kma_datagram_ibe;


/* KMA BIST Reply Datagram */
typedef kma_datagram_b kma_datagram_ibr;


/* KMA BIST Short Reply Datagram */
typedef kma_datagram_b kma_datagram_ibs;


/* KMA M Partition (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
                                                /* If a datagram exceeds the limit of a UDP packet (64kB), then the        */
                                                /*  datagram is split into several datagrams from the PU.  K-Controller/   */
                                                /*  SIS merges parts into one datagram stored in the .kmall file.          */
                                                /*  Therefore, the following should always have 1 datagram.                */
    uint16_t numOfDgms;                         /* Number of datagram parts to rejoin to get one datagram, e.g., 3.        */
    uint16_t dgmNum;                            /* Datagram part number, e.g., 2 (of 3).                                   */
} kma_datagram_m_partition;


/* KMA M Common (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesCmnPart;                   /* Size in bytes of body part struct.                                      */

    uint16_t pingCnt;                           /* A ping is made of one or more RX fans and one or more TX pulses         */
                                                /*  transmitted at approximately the same time.  Ping counter incremented  */
                                                /*  at every set of TX pulses.                                             */

    uint8_t rxFansPerPing;                      /* Number of RX fans per ping.  Combine with swathsPerPing to find the     */
                                                /*  number of datagrams to join for a complete swath.                      */

    uint8_t rxFanIndex;                         /* Index 0 is the aft swath, port side.                                    */

    uint8_t swathsPerPing;                      /* Number of swaths per ping.  A swath is a complete set of across track   */
                                                /* data.  A swath may contain several transmit sectors and RX fans.        */

    uint8_t swathAlongPosition;                 /* Alongship index for the location of the swath in multi-swath mode.      */
                                                /*  Index 0 is the aftmost swath.                                          */

    uint8_t txTransducerInd;                    /* Transducer used in this RX fan:                                         */
                                                /*  0 - TRAI_TX1, 1 - TRAI_TX2, etc.                                       */

    uint8_t rxTransducerInd;                    /* Transducer used in this RX fan:                                         */
                                                /*  0 - TRAI_RX1, 1 - TRAI_RX2, etc.                                       */

    uint8_t numRxTransducers;                   /* Total number of receiving units.                                        */
    uint8_t algorithmType;                      /* For future use.  0 - Current algorithm.                                 */
} kma_datagram_m_common;


/* KMA MRZ Ping Info (152 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesInfoData;                  /* Size in bytes of body part struct.                                      */
    uint16_t padding0;                          /* For byte alignment.                                                     */
    float pingRate_Hz;                          /* Ping rate in Hz.  Filtered/averaged.                                    */
    uint8_t beamSpacing;                        /* Beam spacing: 0 - Equidistance, 1 - Equiangle, 2 - High density.        */

    uint8_t depthMode;                          /* Depth mode.  Describes setting of depth in K-Controller.  Depth mode    */
                                                /*  influences the PUs choice of pulse length and type.  If operator has   */
                                                /*  manually chosen the depth mode to use, then 100 is added to mode index.*/
                                                /*  Auto  Manual      Mode                                                 */
                                                /*   0     100      Very shallow                                           */
                                                /*   1     101      Shallow                                                */
                                                /*   2     102      Medium                                                 */
                                                /*   3     103      Deep                                                   */
                                                /*   4     104      Deeper                                                 */
                                                /*   5     105      Very deep                                              */
                                                /*   6     106      Extra deep                                             */
                                                /*   7     107      Extreme deep                                           */

    uint8_t subDepthMode;                       /* For advanced use when depth mode is set manually.  0 - Sub depth mode   */
                                                /*  is not used (when depth mode is auto).                                 */

    uint8_t distanceBtwSwath;                   /* Achieved distance between swaths, in percent relative to required swath */
                                                /*  distance:                                                              */
                                                /*  0 - Function is not used.                                              */
                                                /*  100 - Achieved swath distance equals required swath distance.          */

    uint8_t detectionMode;                      /* Detection mode: Bottom detection algorithm used:                        */
                                                /*  0 - Normal.                                                            */
                                                /*  1 - Waterway.                                                          */
                                                /*  2 - Tracking.                                                          */
                                                /*  3 - Minimum depth if system running in simulation mode.                */
                                                /*    detectionMode + 100 = simulator.                                     */

    uint8_t pulseForm;                          /* Pulse forms used for current swath:                                     */
                                                /*  0 - CW.                                                                */
                                                /*  1 - Mix.                                                               */
                                                /*  2 - FM.                                                                */

    uint16_t padding1;                          /* For byte alignment.                                                     */

    float frequencyMode_Hz;                     /* Ping frequency in Hz.  If less than 100, it is a code defined below:    */
                                                /*  Value             Frequency            Valid for EM model              */
                                                /*   -1                Not used                   -                        */
                                                /*    0               40 - 100 kHz            EM710, EM712                 */
                                                /*    1               50 - 100 kHz            EM710, EM712                 */
                                                /*    2               70 - 100 kHz            EM710, EM712                 */
                                                /*    3                     50 kHz            EM710, EM712                 */
                                                /*    4                     40 kHz            EM710, EM712                 */
                                                /*   180000-400000   180 - 400 kHz       EM2040C (10kHz steps)             */
                                                /*   200000                200 kHz          EM2040, EM2040P                */
                                                /*   300000                300 kHz          EM2040, EM2040P                */
                                                /*   400000                400 kHz          EM2040, EM2040P                */
                                                /*   600000                600 kHz          EM2040, EM2040P (Added Rev H)  */
                                                /*   700000                700 kHz          EM2040, EM2040P (Added Rev H)  */

    float freqRangeLowLim_Hz;                   /* Lowest center frequency of all sectors in this swath in Hz.             */
    float freqRangeHighLim_Hz;                  /* Highest center frequency of all sectors in this swath in Hz.            */
    float maxTotalTxPulseLength_sec;            /* Total signal length of the sector with longest TX pulse in seconds.     */
    float maxEffTxPulseLength_sec;              /* Effective signal length (-3 dB envelope) of the sector with longest     */
                                                /*  effective TX pulse in seconds.                                         */
    float maxEffTxBandwidth_Hz;                 /* Effective bandwidth (-3dB envelope) of sector with highest bandwidth.   */
    float absCoeff_dBPerkm;                     /* Average absorption coefficient, in dB/km, for vertical beam at current  */
                                                /*  depth.  Not currently used.                                            */
    float portSectorEdge_deg;                   /* Port sector edge, used by beamformer. Coverage is referred to z of SCS  */
                                                /*  in degrees.                                                            */
    float starbSectorEdge_deg;                  /* Stbd sector edge, used by beamformer. Coverage is referred to z of SCS  */
                                                /*  in degrees.                                                            */
    float portMeanCov_deg;                      /* Coverage achieved, corrected for raybending.  Coverage is referred to   */
                                                /*  z of SCS in degrees.                                                   */
    float starbMeanCov_deg;                     /* Coverage achieved, corrected for raybending.  Coverage is referred to   */
                                                /*  z of SCS in degrees.                                                   */
    int16_t portMeanCov_m;                      /* Coverage achieved, corrected for raybending.  Coverage is referred to   */
                                                /*  z of SCS in meters.                                                    */
    int16_t starbMeanCov_m;                     /* Coverage achieved, corrected for raybending.  Coverage is referred to   */
                                                /*  z of SCS in meters.                                                    */

    uint8_t modeAndStabilization;               /* Modes and stabilization settings as chosen by the operator.  Each bit   */
                                                /*  refers to one setting in K-Controller.  Unless otherwise stated,       */
                                                /*  default: 0 - Off, 1 - On/Auto.                                         */
                                                /*  Bit        Mode                                                        */
                                                /*   1    Pitch stabilization                                              */
                                                /*   2    Yaw stabilization                                                */
                                                /*   3    Sonar mode                                                       */
                                                /*   4    Angular coverage mode                                            */
                                                /*   5    Sector mode                                                      */
                                                /*   6    Swath along position (0 - fixed, 1 - dynamic)                    */
                                                /*   7-8  Future use                                                       */

    uint8_t runtimeFilter1;                     /* Filter settings as chosen by the operator.  Refers to settings in       */
                                                /*  runtime display of K-Controller.  Each bit refers to one filter        */
                                                /*  setting.  0 - Off, 1 - On/auto.                                        */
                                                /*  Bit      Filter                                                        */
                                                /*   1    Slope filter                                                     */
                                                /*   2    Aeration filter                                                  */
                                                /*   3    Sector filter                                                    */
                                                /*   4    Interference filter                                              */
                                                /*   5    Special amplitude detect                                         */
                                                /*   6-8  Future use                                                       */

    uint16_t runtimeFilter2;                    /* Filter settings as chosen by the operator.  Refers to settings in       */
                                                /*  runtime display of K-Controller.  Four bits used per filter.           */
                                                /*  Bit   Filter Choice          Setting                                   */
                                                /*  1-4   Range gate size        0 - Small, 1 - Normal, 2 - Large          */
                                                /*  5-8   Spike filter strength  0 - Off, 1 - Weak, 2 - Medium, 3 - Strong */
                                                /*  9-12  Penetration filter     0 - Off, 1 - Weak, 2 - Medium, 3 - Strong */
                                                /* 13-16  Phase ramp             0 - Short, 1 - Normal, 2 - Long           */

    uint32_t pipeTrackingStatus;                /* Pipe tracking status.  Describes how angle and range of top of pipe is  */
                                                /*  determined.  0 - For future use, 1 - PU uses guidance from SIS.        */

    float transmitArraySizeUsed_deg;            /* Transmit array size used.  Direction along ship in degrees.             */
    float receiveArraySizeUsed_deg;             /* Receiver array size used.  Direction across ship in degrees.            */
    float transmitPower_dB;                     /* Operator selected TX power re maximum in dB, e.g., 0 dB, -10 dB, -20 dB.*/
    uint16_t SLrampUpTimeRemaining;             /* For marine mammal protection. Time remaining until max SL achieved in %.*/
    uint16_t padding2;                          /* For byte alignment.                                                     */
    float yawAngle_deg;                         /* Yaw correction angle applied in degrees.                                */

    uint16_t numTxSectors;                      /* Number of TX sectors (Ntx).  Describes how many times the TX sector     */
                                                /*  specific information struct is repeated in the datagram.               */

    uint16_t numBytesPerTxSector;               /* Number of bytes in TX sector specific information struct.               */

    float headingVessel_deg;                    /* Heading of vessel at time of midpoint of first TX pulse.  From active   */
                                                /*  heading sensor.                                                        */

    float soundSpeedAtTxDepth_mPerSec;          /* Sound speed at time of midpoint of first TX pulse used in depth         */
                                                /*  calculation.                                                           */

    float txTransducerDepth_m;                  /* TX transducer depth in meters below waterline at time of midpoint of    */
                                                /*  first TX pulse.  For the TX array (head) used by this RX-fan.  Use     */
                                                /*  depth of TX1 to move depth point (XYZ) from water line to transducer.  */

    float z_waterLevelReRefPoint_m;             /* Distance between water line and vessel reference point in meters.  At   */
                                                /*  time of midpoint of first TX pulse.  Measured in the SCS.  Use this    */
                                                /*  to move depth point (XYZ) from vessel reference point to waterline.    */

    float x_kmallToall_m;                       /* Distance between *.all reference point and *.kmall reference point      */
                                                /*  (vessel reference point) in meters, in the SCS, at time of midpoint of */
                                                /*  first TX pulse.  Use to move depth point (XYZ) from vessel reference   */
                                                /*  point to the horizontal location (X,Y) of the active position sensor's */
                                                /*  reference point (old datagram format).                                 */

    float y_kmallToall_m;                       /* Distance between *.all reference point and *.kmall reference point      */
                                                /*  (vessel reference point) in meters, in the SCS, at time of midpoint of */
                                                /*  first TX pulse.  Use to move depth point (XYZ) from vessel reference   */
                                                /*  point to the horizontal location (X,Y) of the active position sensor's */
                                                /*  reference point (old datagram format).                                 */

    uint8_t latLongInfo;                        /* Method of position determination from position sensor data:             */
                                                /*  0 - Last position received.                                            */
                                                /*  1 - Interpolated.                                                      */
                                                /*  2 - Processed.                                                         */

    uint8_t posSensorStatus;                    /* Status/quality for data from active position sensor:                    */
                                                /*  0 - Valid data.                                                        */
                                                /*  1 - Invalid data.                                                      */
                                                /*  2 - Reduced performance.                                               */

    uint8_t attitudeSensorStatus;               /* Status/quality for data from active attitude sensor:                    */
                                                /*  0 - Valid data.                                                        */
                                                /*  1 - Invalid data.                                                      */
                                                /*  2 - Reduced performance.                                               */

    uint8_t padding3;                           /* For byte alignment.                                                     */

    double latitude_deg;                        /* Latitude of vessel reference point at time of midpoint of first TX      */
                                                /*  pulse in degrees.  Negative in southern hemisphere.  Set to            */
                                                /*  KMA_NULL_LATLON if not available.                                      */

    double longitude_deg;                       /* Longitude of vessel reference point at time of midpoint of first TX     */
                                                /*  pulse in degrees.  Negative in western hemisphere.  Set to             */
                                                /*  KMA_NULL_LATLON if not available.                                      */

    float ellipsoidHeightReRefPoint_m;          /* Height of vessel reference point above the ellipsoid, derived from the  */
                                                /*  active GGA sensor.  This is the GGA height corrected for motion and    */
                                                /*  installation offsets of the position sensor.                           */

    /* Added in Version 1 (Rev G) */
    float bsCorrectionOffset_dB;                /* Backscatter offset set in the installation menu in dB.                  */
    uint8_t lambertsLawApplied;                 /* Beam intensity data corrected as seabed image data (Lambert and         */
                                                /*  normal-incidence corrections).                                         */
    uint8_t iceWindow;                          /* Ice window installed.                                                   */
    uint16_t activeModes;                       /* Status for active modes (Added in Rev H):                               */
                                                /*  Bits 1 : EM Multifrequency Mode (0 = not active, 1 = active).          */
                                                /*  Bits 2-16 : Not in use.                                                */
} kma_datagram_mrz_ping_info;


/* KMA MRZ TX Sector Info Version 0 (48 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t txSectorNum;                        /* TX sector index number, used in the sounding section.  Starts at 0.     */
    uint8_t txArrNum;                           /* TX array number.  For a single TX, this is 0.                           */

    uint8_t txSubArray;                         /* For EM2040, the transmitted pulse consists of three sectors, each       */
                                                /*  transmitted from separate txSubArrays.  Orientation and numbers are    */
                                                /*  relative to ACS.  Sub array installation offsets are in IIP datagram.  */
                                                /*  0 - Port subarray (default).                                           */
                                                /*  1 - Middle subarray.                                                   */
                                                /*  2 - Starboard subarray.                                                */

    uint8_t padding0;                           /* For byte alignment.                                                     */

    float sectorTransmitDelay_sec;              /* Transmit delay of the current sector/subarray.  Delay is the time from  */
                                                /*  the midpoint of the current transmission to the midpoint of the first  */
                                                /*  transmitted pulse of the ping, i.e., relative to the time used in the  */
                                                /*  datagram header.                                                       */

    float tiltAngleReTx_deg;                    /* Along-ship steering angle of the TX beam (main lobe of transmitted      */
                                                /*  pulse), angle referred to transducer ACS in degrees.                   */

    float txNominalSourceLevel_dB;              /* Actual SL = txNominalSourceLevel_dB + highVoltageLevel_dB in dB re      */
                                                /*  1 microPascal.  Definition changed in Version 1 (Rev G).               */

    float txFocusRange_m;                       /* Focus range in meters.  0 - No focusing applied.                        */
    float centreFreq_Hz;                        /* Center frequency in Hertz.                                              */

    float signalBandWidth_Hz;                   /* Signal bandwidth in Hertz.                                              */
                                                /*  FM mode: Effective bandwidth, CW mode: 1/(effective TX pulse length)   */

    float totalSignalLength_sec;                /* Total signal length (also called pulse length) in seconds.              */

    uint8_t pulseShading;                       /* Transmit pulse is shaded in time (tapering).  Amplitude shading in      */
                                                /*  percent cos^2 function used for shading the TX pulse in time.          */

    uint8_t signalWaveForm;                     /* Transmit signal wave form:                                              */
                                                /*  0 - CW, 1 - FM upsweep, 2 - FM downsweep.                              */

    uint16_t padding1;                          /* For byte alignment.                                                     */
} kma_datagram_mrz_tx_sector_info_v0;


/* KMA MRZ TX Sector Info Version 1 (48 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t txSectorNum;                        /* TX sector index number, used in the sounding section.  Starts at 0.     */
    uint8_t txArrNum;                           /* TX array number.  For a single TX, this is 0.                           */

    uint8_t txSubArray;                         /* For EM2040, the transmitted pulse consists of three sectors, each       */
                                                /*  transmitted from separate txSubArrays.  Orientation and numbers are    */
                                                /*  relative to ACS.  Sub array installation offsets are in IIP datagram.  */
                                                /*  0 - Port subarray (default).                                           */
                                                /*  1 - Middle subarray.                                                   */
                                                /*  2 - Starboard subarray.                                                */

    uint8_t padding0;                           /* For byte alignment.                                                     */

    float sectorTransmitDelay_sec;              /* Transmit delay of the current sector/subarray.  Delay is the time from  */
                                                /*  the midpoint of the current transmission to the midpoint of the first  */
                                                /*  transmitted pulse of the ping, i.e., relative to the time used in the  */
                                                /*  datagram header.                                                       */

    float tiltAngleReTx_deg;                    /* Along-ship steering angle of the TX beam (main lobe of transmitted      */
                                                /*  pulse), angle referred to transducer ACS in degrees.                   */

    float txNominalSourceLevel_dB;              /* Actual SL = txNominalSourceLevel_dB + highVoltageLevel_dB in dB re      */
                                                /*  1 microPascal.  Definition changed in Version 1 (Rev G).               */

    float txFocusRange_m;                       /* Focus range in meters.  0 - No focusing applied.                        */
    float centreFreq_Hz;                        /* Center frequency in Hertz.                                              */

    float signalBandWidth_Hz;                   /* Signal bandwidth in Hertz.                                              */
                                                /*  FM mode: Effective bandwidth, CW mode: 1/(effective TX pulse length)   */

    float totalSignalLength_sec;                /* Total signal length (also called pulse length) in seconds.              */

    uint8_t pulseShading;                       /* Transmit pulse is shaded in time (tapering).  Amplitude shading in      */
                                                /*  percent cos^2 function used for shading the TX pulse in time.          */

    uint8_t signalWaveForm;                     /* Transmit signal wave form:                                              */
                                                /*  0 - CW, 1 - FM upsweep, 2 - FM downsweep.                              */

    uint16_t padding1;                          /* For byte alignment.                                                     */

    /* Following added in Version 1 (Rev G) */
    float highVoltageLevel_dB;                  /* 20 Log (measured high-voltage power level at Tx pulse / Nominal high-   */
                                                /*  voltage power level).  This parameter will also include the effect of  */
                                                /*  user-selected transmit power reduction (transmitPower_dB) and mammal   */
                                                /*  protection.                                                            */

    float sectorTrackingCorr_dB;                /* Backscatter correction added in sector tracking mode in dB.             */

    float effectiveSignalLength_sec;            /* Signal length used for backscatter footprint calculation in seconds.    */
                                                /*  This compensates for the TX pulse tapering and the RX filter           */
                                                /*  bandwidths.                                                            */
} kma_datagram_mrz_tx_sector_info_v1;


/* KMA MRZ TX Sector Info */
typedef struct {
    kma_datagram_mrz_tx_sector_info_v0 *v0;     /* Pointer to dgmVersion=0 struct.  Array of length numTxSectors.          */
    kma_datagram_mrz_tx_sector_info_v1 *v1;     /* Pointer to dgmVersion=1 struct.  Array of length numTxSectors.          */
                                                /*  NOTE: Either pointer can be NULL.                                      */
} kma_datagram_mrz_tx_sector_info;


/* KMA MRZ Rx Info (32 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesRxInfo;                    /* Number of bytes in the current struct.                                  */

    uint16_t numSoundingsMaxMain;               /* Maximum number of main soundings (bottom soundings) in this datagram,   */
                                                /*  extra detections (soundings in water column) excluded.  Also referred  */
                                                /*  to as Nrx.  Denotes how many bottom points given in the soundings      */
                                                /*  struct.                                                                */

    uint16_t numSoundingsValidMain;             /* Number of main soundings of valid quality.  Extra detects not included. */

    uint16_t numBytesPerSounding;               /* Number of bytes per loop of sounding (per depth point), i.e., bytes per */
                                                /*  loops of the soundings struct.                                         */

    float WCSampleRate_Hz;                      /* Sample frequency divided by water column decimation factor in Hertz.    */
    float seabedImageSampleRate_Hz;             /* Sample frequency divided by seabed image decimation factor in Hertz.    */
    float BSnormal_dB;                          /* Backscatter level at normal incidence in dB.                            */
    float BSoblique_dB;                         /* Backscatter level at oblique incidence in dB.                           */
    uint16_t extraDetectionAlarmFlag;           /* This is the sum of alarm flags.  Range 0-10.                            */
    uint16_t numExtraDetections;                /* Sum of extra detections from all classes.  Also referred to as Nd.      */
    uint16_t numExtraDetectionClasses;          /* Range 0-10.                                                             */
    uint16_t numBytesPerClass;                  /* Number of bytes in the extra detect class info struct.                  */
} kma_datagram_mrz_rx_info;


/* KMA MRZ Extra Detect Info (4 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numExtraDetInClass;                /* Number of extra detections in this class.                               */
    uint8_t padding;                            /* For byte alignment.                                                     */
    uint8_t alarmFlag;                          /* Alarm: 0 - No alarm, 1 - Alarm.                                         */
} kma_datagram_mrz_extra_det_info;


/* KMA MRZ Sounding (120 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t soundingIndex;                     /* Sounding index.  Cross reference for seabed image.  Valid range: 0 to   */
                                                /*  (numSoundingsMaxMin + numExtraDetections)-1, i.e., 0 - (Nrx+Nd)-1.     */

    uint8_t txSectorNum;                        /* Transmitting sector number.  Valid range: 0-(Ntx-1).                    */

    /* Detection Info */
    uint8_t detectionType;                      /* Bottom detection type:                                                  */
                                                /*  0 - Normal detection                                                   */
                                                /*  1 - Extra detection                                                    */
                                                /*  2 - Rejected detection                                                 */
                                                /*  In case 2, the estimated range has been used to fill in amplitude      */
                                                /*  samples in the seabed image datagram.                                  */

    uint8_t detectionMethod;                    /* Method for determining bottom detection:                                */
                                                /*  0 - No valid detection                                                 */
                                                /*  1 - Amplitude detection                                                */
                                                /*  2 - Phase detection                                                    */
                                                /*  3-15 - For future use                                                  */

    uint8_t rejectionInfo1;                     /* For Kongsberg use.                                                      */
    uint8_t rejectionInfo2;                     /* For Kongsberg use.                                                      */
    uint8_t postProcessingInfo;                 /* For Kongsberg use.                                                      */

    uint8_t detectionClass;                     /* Only used by extra detections. Detection class based on detected range. */
                                                /*  Detection class 1 to 7 corresponds to value 0-6.  If the value is      */
                                                /*  between 100 and 106, the class is disabled by the operator.  If the    */
                                                /*  value is 107, the detections are outside the threshold limits.         */

    uint8_t detectionConfidenceLevel;           /* Detection confidence level.                                             */
    uint16_t padding;                           /* For byte alignment.                                                     */
    float rangeFactor;                          /* Range factor in percent.  This is 100% if main detection.               */

    float qualityFactor;                        /* Estimated standard deviation as % of the detected depth.  QF is         */
                                                /*  calculated from IFREMER QF (IQF): QF = Est(dz)/z=100*10^-IQF.          */

    float detectionUncertaintyVer_m;            /* Vertical uncertainty, based on QF, in meters.                           */
    float detectionUncertaintyHor_m;            /* Horizontal uncertainty, based on QF, in meters.                         */
    float detectionWindowLength_sec;            /* Detection window length in seconds.  Sample data range used in final    */
                                                /*  detection.                                                             */
    float echoLength_sec;                       /* Measured echo length in seconds.                                        */

    /* Water Column Parameters */
    uint16_t WCBeamNum;                         /* Water column beam number. Info for plotting soundings together with     */
                                                /*  water column data.                                                     */
    uint16_t WCrange_samples;                   /* Water column range.  Range of bottom detection in samples.              */
    float WCNomBeamAngleAcross_deg;             /* Water column nominal beam angle across Re vertical.                     */

    /* Reflectivity Data (Backscatter Data) */
    float meanAbsCoeff_dBPerkm;                 /* Mean absorption coefficient in dB/km.  Used in TVG calc, value as used. */

    float reflectivity1_dB;                     /* Beam intensity, using the traditional KM special TVG.                   */
                                                /*  NOTE: Appears to have -100.0 dB as invalid value.                      */
                                                /*  NOTE: R0 parameter is not stored in the data; must be estimated from   */
                                                /*   the minimum swath.  Cross-over angle is stored in runtime parameters  */
                                                /*   as 'Normal incidence corr.' parameter.                                */

    float reflectivity2_dB;                     /* Beam intensity (BS), using TVG = X log(R) + 2 alpha R.  X (operator     */
                                                /*  selected) is common to all beams in datagram.  Alpha is given for each */
                                                /*  beam (current struct).  BS = EL - SL - M + TVG + BScorr, where         */
                                                /*  EL = detected echo level (not recorded in datagram), and the rest of   */
                                                /*  parameters are found below.                                            */
                                                /*  NOTE: Have seen NANs in this field for EM124 data.                     */

    float receiverSensitivityApplied_dB;        /* Receiver sensitivity (M), in dB, compensated for RX beampattern at      */
                                                /*  actual transmit frequency at current vessel attitude.                  */

    float sourceLevelApplied_dB;                /* Source level (SL) applied in dB: SL = SLnom + SLcorr, where             */
                                                /*  SLnom = Nominal maximum SL, recorded per TX sector                     */
                                                /*  (txNominalSourceLevel_dB) and SLcorr = SL correction relative to       */
                                                /*  nominal TX power based on measured high-voltage power level and any    */
                                                /*  use of digital power control. SL is corrected for TX beampattern along */
                                                /*  and across at actual transmit frequency at current vessel attitude.    */

    float BScalibration_dB;                     /* Backscatter (BScorr) calibration offset applied (default = 0 dB).       */
    float TVG_dB;                               /* Time Varying Gain (TVG) used when correcting reflectivity.              */

    /* Range and Angle Data */
    float beamAngleReRx_deg;                    /* Angle relative to the RX transducer array, except for ME70, where the   */
                                                /*  angles are relative to the horizontal plane.                           */

    float beamAngleCorrection_deg;              /* Applied beam pointing angle correction.                                 */
    float twoWayTravelTime_sec;                 /* Two-way travel time (also called range) in seconds.                     */
    float twoWayTravelTimeCorrection_sec;       /* Applied two-way travel time correction in seconds.                      */

    /* Georeferenced Depth Points */
    float deltaLatitude_deg;                    /* Latitudinal distance from vessel reference point at time of first TX    */
                                                /*  pulse in ping, to depth point in decimal degrees. Measured in the SCS. */

    float deltaLongitude_deg;                   /* Longitudinal distance from vessel reference point at time of first TX   */
                                                /*  pulse in ping, to depth point in decimal degrees. Measured in the SCS. */

    float z_reRefPoint_m;                       /* Vertical distance z.  Distance from vessel reference point at time of   */
                                                /*  first TX pulse in ping to depth point.  Measured in the SCS.           */

    float y_reRefPoint_m;                       /* Horizontal distance y.  Distance from vessel reference point at time of */
                                                /*  first TX pulse in ping to depth point.  Measured in the SCS.           */

    float x_reRefPoint_m;                       /* Horizontal distance x.  Distance from vessel reference point at time of */
                                                /*  first TX pulse in ping to depth point.  Measured in the SCS.           */

    float beamIncAngleAdj_deg;                  /* Beam incidence angle adjustment (IBA) in degrees.                       */
    uint16_t realTimeCleanInfo;                 /* For future use.                                                         */

    /* Seabed Image */
    uint16_t SIstartRange_samples;              /* Seabed image start range in sample number from transducer.  Valid only  */
                                                /*  for the current beam.                                                  */
    uint16_t SIcentreSample;                    /* Number of the center seabed image sample for the current beam.          */
    uint16_t SInumSamples;                      /* Number of range samples from current beam.  Used to form seabed image.  */
} kma_datagram_mrz_sounding;


/* KMA Multibeam Raw Range and Depth Datagram */
typedef struct {
    kma_datagram_m_partition *partition;        /* Data partition information.                                             */
    kma_datagram_m_common *common;              /* Information of transmitter and receiver used to find data in datagram.  */
    kma_datagram_mrz_ping_info *pingInfo;       /* Pointer to the MRZ Ping Info struct.                                    */
    kma_datagram_mrz_tx_sector_info txSectorInfo;
    kma_datagram_mrz_rx_info *rxInfo;           /* Pointer to MRZ Rx Info struct.                                          */
    kma_datagram_mrz_extra_det_info *extraDetInfo; /* Array of extra detect classes.  Can be NULL if none.                 */
    kma_datagram_mrz_sounding *sounding;        /* Array of soundings.  Can be NULL if none.                               */
    int16_t *sample;                            /* Seabed image sample amplitude in 0.1 dB. The number of samples is found */
                                                /*  by summing SInumSamples in the soundings struct for all beams.  Seabed */
                                                /*  image data are raw beam sample data taken from the RX beams.  The data */
                                                /*  samples are selected based on the bottom detection ranges.  First      */
                                                /*  sample for each beam has the lowest range.  The center sample for each */
                                                /*  beam is georeferenced.  The BS corrections at the center sample are    */
                                                /*  the same as used for reflectivity2_dB in soundings struct.             */
} kma_datagram_mrz;


/* KMA MWC TX Info (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesTxInfo;                    /* Number of bytes in current struct.                                      */
    uint16_t numTxSectors;                      /* Number transmitting sectors (Ntx).  Denotes the number of times the     */
                                                /*  MWC Tx Sector Data is repeated in the datagram.                        */
    uint16_t numBytesPerTxSector;               /* Number of bytes in the MWC TX Sector Data struct.                       */
    uint16_t padding;                           /* For byte alignment.                                                     */
    float heave_m;                              /* Heave at vessel reference point at time of ping, i.e., at midpoint of   */
                                                /*  first TX pulse in Rxfan.                                               */
} kma_datagram_mwc_tx_info;


/* KMA MWC TX Sector Info (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float tiltAngleReTx_deg;                    /* Along-ship steering angle of the TX beam (main lobe of transmitted      */
                                                /*  pulse), angle referred to transducer face.  Angle as used by           */
                                                /*  beamformer (includes stabilization) in degrees.                        */

    float centreFreq_Hz;                        /* Center frequency of current sector in Hertz.                            */
    float txBeamWidthAlong_deg;                 /* Corrected for frequency, sound velocity, and tilt angle in degrees.     */
    uint16_t txSectorNum;                       /* Transmitting sector number.                                             */
    uint16_t padding;                           /* For byte alignment.                                                     */
} kma_datagram_mwc_tx_sector_info;


/* KMA MWC Rx Info (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesRxInfo;                    /* Number of bytes in current struct.                                      */
    uint16_t numBeams;                          /* Number of beams in this datagram (Nrx).                                 */
    uint8_t numBytesPerBeamEntry;               /* Bytes in MWC Rx Beam Data struct, excluding sample amplitudes (which    */
                                                /*  have varying lengths).                                                 */
    uint8_t phaseFlag;                          /* Phase flag: 0 - Off, 1 - Low resolution, 2 - High resolution.           */
    uint8_t TVGfunctionApplied;                 /* Time Varying Gain function applied (X).  X log R + 2 alpha R + OFS + C, */
                                                /*  where X and C is documented in MWC datagram.  OFS is gain offset to    */
                                                /*  compensate for TX source level, receiver sensitivity, etc.             */
    int8_t TVGoffset_dB;                        /* TVGain offset used (OFS) in dB. X log R + 2 alpha R + OFS + C, where    */
                                                /*  X and C is documented in MWC datagram.  OFS is gain offset to          */
                                                /*  compensate for TX source level, receiver sensitivity, etc.             */
    float sampleFreq_Hz;                        /* The sample rate is normally decimated to be approximately the same as   */
                                                /*  the bandwidth of the transmitted pulse in Hertz.                       */
    float soundVelocity_mPerSec;                /* Sound speed at transducer in m/s.                                       */
} kma_datagram_mwc_rx_info;


/* KMA MWC RX Beam Data (16 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float beamPointAngReVertical_deg;           /* Beam pointing angle re vertical in degrees.                             */
    uint16_t startRangeSampleNum;               /* Start range sample number.                                              */
    uint16_t detectedRangeInSamples;            /* Two-way range in samples.  Approximation to calculated distance from    */
                                                /* TX to bottom detection = soundVelocity_mPerSec *                        */
                                                /*  detectedRangeInSamples / (sampleFreq_Hz * 2).  The detected range is   */
                                                /*  set to zero when the beam has no bottom detection.                     */
    uint16_t beamTxSectorNum;                   /* The transmit sector for this beam.                                      */
    uint16_t numSamples;                        /* Number of sample data for current beam.  Also denoted Ns.               */

    /* Added in Version 1 (Rev G) */
    float detectedRangeInSamplesHighResolution; /* Same as detectedRangeInSamples with higher resolution.                  */
} kma_datagram_mwc_rx_beam_data;


/* KMA MWC RX Beam */
typedef struct {
    kma_datagram_mwc_rx_beam_data *data;        /* Pointer to Rx beam data.                                                */
    int8_t *sampleAmplitude_05dB;               /* Water column amplitude data in 0.5 dB.  Array of numSamples.            */
                                                /*  NOTE: Null values appear to be set to -128.                            */
    int8_t *rxBeamPhase1;                       /* RX beam phase in 180/128 degree resolution.  Array of numSamples.       */
    int16_t *rxBeamPhase2;                      /* RX beam phase in 0.01 degree resolution.  Array of numSamples.          */
                                                /*  NOTE: Null values appear to be set to -32767.                          */
} kma_datagram_mwc_rx_beam;


/* KMA Water Column Datagram */
typedef struct {
    kma_datagram_m_partition *partition;        /* Data partition information.                                             */
    kma_datagram_m_common *common;              /* Information of transmitter and receiver used to find data in datagram.  */
    kma_datagram_mwc_tx_info *txInfo;           /* Pointer to Tx Info struct.                                              */
    kma_datagram_mwc_tx_sector_info *txSectorInfo; /* Array of numTxSectors.                                               */
    kma_datagram_mwc_rx_info *rxInfo;           /* Pointer to Rx Info struct.                                              */
    uint8_t *beamData;                          /* Beam data.  Should be parsed with kma_get_mwc_rx_beam_data ().          */
} kma_datagram_mwc;


/* KMA S Common (8 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesCmnPart;                   /* Size in bytes of current struct.                                        */

    uint16_t sensorSystem;                      /* Sensor system number as indicated when setting up the system in         */
                                                /*  K-Controller installation menu. E.g., position system 0 refers to      */
                                                /*  system POSI_1 in installation datagram IIP.  Check if this sensor is   */
                                                /*  active by using the IIP datagram.                                      */
                                                /*  SCL - Clock Datagram:                                                  */
                                                /*   Bit      Sensor System                                                */
                                                /*    0        Time synchronization from clock data.                       */
                                                /*    1        Time synchronization from active position data.             */
                                                /*    2        1 PPS is used.                                              */

    uint16_t sensorStatus;                      /* Sensor status.  To indicate quality of sensor data is valid or invalid. */
                                                /*  Quality may be invalid even if sensor is active and the PU receives    */
                                                /*  data.  Bit code varies according to the type of sensor.                */
                                                /*  Bit Number    Sensor Data                                              */
                                                /*   0             1 - Sensor is chosen as active.                         */
                                                /*                 1 - Valid data and 1PPS OK (#SCL only).                 */
                                                /*   1             0                                                       */
                                                /*   2             0 - Data OK.                                            */
                                                /*                 1 - Reduced performance.                                */
                                                /*                 1 - Reduced performance, no time sync of PU (#SCL only).*/
                                                /*   3             0                                                       */
                                                /*   4             0 - Data OK.                                            */
                                                /*                 1 - Invalid data.                                       */
                                                /*   5             0                                                       */
                                                /*   6             0 - Velocity from sensor.                               */
                                                /*                 1 - Velocity calculated by PU.                          */
                                                /*   7             0                                                       */
                                                /*  For #SPO (position) and #CPO (position compatibility) datagrams, 8-15: */
                                                /*   8             0                                                       */
                                                /*   9             0 - Time from PU used (system).                         */
                                                /*                 1 - Time from datagram used (e.g., from GGA telegram).  */
                                                /*  10             0 - No motion correction.                               */
                                                /*                 1 - With motion correction.                             */
                                                /*  11             0 - Normal quality check.                               */
                                                /*                 1 - Operator quality check.  Data always valid.         */
                                                /*  12             0                                                       */
                                                /*  13             0                                                       */
                                                /*  14             0                                                       */
                                                /*  15             0                                                       */

    uint16_t padding;                           /* For byte alignment.                                                     */
} kma_datagram_s_common;


/* KMA SPO Data (40 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t timeFromSensor_sec;                /* UTC time from position sensor in seconds.  Epoch 1970-01-01.            */
    uint32_t timeFromSensor_nanosec;            /* UTC time from position sensor in nano-second remainder.                 */
    float posFixQuality_m;                      /* Only available as input from sensor. Calculation according to format.   */

    double correctedLat_deg;                    /* Motion corrected (if enabled in K-Controller) data as used in depth     */
                                                /*  calculations in degrees.  Referred to vessel reference point.  Set to  */
                                                /*  KMA_NULL_LATLON if sensor is inactive.                                 */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    double correctedLong_deg;                   /* Motion corrected (if enabled in K-Controller) data as used in depth     */
                                                /*  calculations in degrees.  Referred to vessel reference point.  Set to  */
                                                /*  KMA_NULL_LATLON if sensor is inactive.                                 */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    float speedOverGround_mPerSec;              /* Speed over ground in m/s. Motion corrected (if enabled in K-Controller) */
                                                /*  data as used in depth calculations.  If unavailable or from inactive   */
                                                /*  sensor, then value set to KMA_NULL_SPEED.                              */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    float courseOverGround_deg;                 /* Course over ground in degrees.  Motion corrected (if enabled in         */
                                                /*  K-Controller) data as used in depth calculations.  If unavailable or   */
                                                /*  from inactive sensor, then value set to KMA_NULL_COURSE.               */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    float ellipsoidHeightReRefPoint_m;          /* Height of vessel reference point above the ellipsoid in meters.  Motion */
                                                /*  corrected (if enabled in K-Controller) data as used in depth           */
                                                /*  calculations. If unavailable or from inactive sensor, then set to      */
                                                /*  KMA_NULL_ELLIPSOIDAL_HEIGHT.                                           */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */
} kma_datagram_spo_data;


/* KMA Sensor Position Datagram */
typedef struct {
    kma_datagram_s_common *common;              /* Common part for all external sensors.                                   */
    kma_datagram_spo_data *data;                /* Pointer to SPO data.                                                    */
    uint8_t *dataFromSensor;                    /* Position data as received from sensor, i.e., uncorrected for motion.    */
} kma_datagram_spo;


/* KMA Datagram SKM Info Version 1 (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesInfoPart;                  /* Size in bytes of current struct.  Used for denoting size of rest of     */
                                                /*  datagram in cases where only one data block is attached.               */

    uint8_t sensorSystem;                       /* Attitude system number, as numbered in the installation parameters.     */
                                                /*  E.g., system 0 refers to system ATTI_1 in installation datagram #IIP.  */

    uint8_t sensorStatus;                       /* Sensor status.  Summarizes the status fields of all KM binary samples   */
                                                /*  added in the datagram (status in SKM Binary Data struct).  Only        */
                                                /*  available data from input sensor format is summarized.  Available data */
                                                /*  found in sensorDataContents.                                           */
                                                /*   Bits 0-7 common to all sensors and #MRZ sensor status:                */
                                                /* Bit Number    Sensor Data                                               */
                                                /*  0             1 - Sensor is chosen as active.                          */
                                                /*                1 - Sensor is chosen as active attitude velocity if      */
                                                /*                    using attitude split (one sensor input as attitude   */
                                                /*                    only, and one sensor input for attitude velocity).   */
                                                /*  1             0                                                        */
                                                /*  2             0 - Data OK                                              */
                                                /*                1 - Reduced performance                                  */
                                                /*  3             0                                                        */
                                                /*  4             0 - Data OK                                              */
                                                /*                1 - Invalid data                                         */
                                                /*  5             0                                                        */
                                                /*  6             0 - Velocity from sensor                                 */
                                                /*                1 - Velocity calculated by PU                            */
                                                /*  7             0                                                        */

    uint16_t sensorInputFormat;                 /* Format of raw data from input sensor, given in numerical code:          */
                                                /*  Code  Sensor Format                                                    */
                                                /*   1     KM binary sensor input                                          */
                                                /*   2     EM 3000 data                                                    */
                                                /*   3     Sagem                                                           */
                                                /*   4     Seapath binary 11                                               */
                                                /*   5     Seapath binary 23                                               */
                                                /*   6     Seapath binary 26                                               */
                                                /*   7     POS M/V GRP 102/103                                             */

    uint16_t numSamples;                        /* Number of KM binary sensor samples added in this datagram.              */
    uint16_t numBytesPerSample;                 /* Length in bytes of one whole KM binary sensor sample.                   */

    uint16_t sensorDataContents;                /* Field to indicate which information is available from the input sensor  */
                                                /*  at the given sensor format.  0 - Not available, 1 - Data is available. */
                                                /*  The bit pattern is used to determine sensorStatus from status field in */
                                                /*  #KMB samples.  Only data available from sensor is checked up against   */
                                                /*  invalid/reduced performance in status and summaries in sensorStatus.   */
                                                /*  Expected data field in sensor input:                                   */
                                                /*   Bit Number     Sensor Data                                            */
                                                /*    0              Horizontal position and velocity                      */
                                                /*    1              Roll and pitch                                        */
                                                /*    2              Heading                                               */
                                                /*    3              Heave and vertical velocity                           */
                                                /*    4              Acceleration                                          */
                                                /*    5              Delayed heave                                         */
                                                /*    6              Delayed heave                                         */
                                                /* NOTE: This field replaced padding bytes in version 1.                   */
                                                /* NOTE: Bit 3 changed from 'heave and vertical velocity' to just 'heave'  */
                                                /*       in Rev I.                                                         */
                                                /* NOTE: Bit 5 changed from 'Error fields' to 'Delayed heave'.             */
} kma_datagram_skm_info;


/* KMA Datagram SKM Binary Data (120 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint8_t dgmType[4];                         /* Set to "#KMB".  Not nul-terminated.                                     */
    uint16_t numBytesDgm;                       /* Datagram length in bytes.  The length field at the start and end of the */
                                                /*  datagram are included in the length count.                             */
    uint16_t dgmVersion;                        /* Datagram version.                                                       */
    uint32_t time_sec;                          /* UTC time from inside the KM sensor data in seconds.  Epoch 1970-01-01.  */
                                                /*  If time is unavailable from attitude sensor input, time of reception   */
                                                /*  on serial port is added to this field.                                 */
    uint32_t time_nanosec;                      /* UTC time from inside the KM sensor in nano-second remainder.            */

    uint32_t status;                            /* Sensor data validity and indication of reduced performance as bitmask:  */
                                                /*  If the bit is set, that indicates invalid or reduced performance.      */
                                                /*                                                                         */
                                                /*  Invalid Data:                                                          */
                                                /*   Bit Number     Sensor Data                                            */
                                                /*    0              Horizontal position and velocity                      */
                                                /*    1              Roll and pitch                                        */
                                                /*    2              Heading                                               */
                                                /*    3              Heave and vertical velocity                           */
                                                /*    4              Acceleration                                          */
                                                /*    5              Delayed heave (deprecated in Rev I)                   */
                                                /*    6              Delayed heave                                         */
                                                /*                                                                         */
                                                /*  Reduced Performance:                                                   */
                                                /*   Bit Number     Sensor Data                                            */
                                                /*    16             Horizontal position and velocity                      */
                                                /*    17             Roll and pitch                                        */
                                                /*    18             Heading                                               */
                                                /*    19             Heave and vertical velocity                           */
                                                /*    20             Acceleration                                          */
                                                /*    21             Delayed heave (deprecated in Rev I)                   */
                                                /*    22             Delayed heave                                         */
                                                /* NOTE: Bit 6 and 22 changed from 'Error fields' to 'Delayed heave' in    */
                                                /*       Rev. I.                                                           */

    double latitude_deg;                        /* Latitude position in degrees.                                           */
    double longitude_deg;                       /* Longitude position in degrees.                                          */

    float ellipsoidHeight_m;                    /* Height of sensor reference point above the ellipsoid.  Positive above   */
                                                /*  the ellipsoid.  Not corrected for motion and installation offsets of   */
                                                /*  the position sensor.                                                   */

    float roll_deg;                             /* Roll in degrees.                                                        */
    float pitch_deg;                            /* Pitch in degrees.                                                       */
    float heading_deg;                          /* Heading in degrees.                                                     */
    float heave_m;                              /* Heave in meters.  Positive down.                                        */
    float rollRate_degPerSec;                   /* Roll rate in deg/s.                                                     */
    float pitchRate_degPerSec;                  /* Pitch rate in deg/s.                                                    */
    float yawRate_degPerSec;                    /* Yaw (heading) rate in deg/s.                                            */
    float velNorth_mPerSec;                     /* Velocity north (X) in m/s.                                              */
    float velEast_mPerSec;                      /* Velocity east (Y) in m/s.                                               */
    float velDown_mPerSec;                      /* Velocity down (Z) in m/s.                                               */
    float latitudeError_m;                      /* Latitude error in meters.                                               */
    float longitudeError_m;                     /* Longitude error in meters.                                              */
    float ellipsoidHeightError_m;               /* Ellipsoid height error in meters.                                       */
    float rollError_deg;                        /* Roll error in degrees.                                                  */
    float pitchError_deg;                       /* Pitch error in degrees.                                                 */
    float headingError_deg;                     /* Heading error in degrees.                                               */
    float heaveError_m;                         /* Heave error in meters.                                                  */
    float northAcceleration_mPerSecSec;         /* Acceleration north in m/s^2.                                            */
    float eastAcceleration_mPerSecSec;          /* Acceleration east in m/s^2.                                             */
    float downAcceleration_mPerSecSec;          /* Acceleration down in m/s^2.                                             */
} kma_datagram_skm_binary;


/* KMA Datagram SKM Delayed Heave (12 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t time_sec;                          /* UTC time in seconds.  Epoch 1970-01-01.                                 */
    uint32_t time_nanosec;                      /* UTC time in nano-second remainder.                                      */
    float delayedHeave_m;                       /* Delayed heave in meters.                                                */
} kma_datagram_skm_delayed_heave;


/* KMA Datagram SKM Sample (132 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    kma_datagram_skm_binary KMdefault;
    kma_datagram_skm_delayed_heave delayedHeave;
} kma_datagram_skm_sample;


/* KMA KM Binary Sensor Data Datagram */
typedef struct {
    kma_datagram_skm_info *info;                /* Pointer to SKM info.                                                    */
    kma_datagram_skm_sample *data;              /* Array of SKM samples of length numSamples.                              */
} kma_datagram_skm;


/* KMA Sound Velocity Profile Datagram (28 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesInfoPart;                  /* Size in bytes of current struct.                                        */
    uint16_t numSamples;                        /* Number of sound velocity samples.                                       */
    uint8_t sensorFormat[4];                    /* Sound velocity profile format: 'S00' - SVP, 'S01' - CTD profile.        */
                                                /*   NOTE: This field has been observed to have garbage in early data.     */
    uint32_t time_sec;                          /* Time extracted from SVP.  Set to zero if not found.                     */
    double latitude_deg;                        /* Latitude in degrees from SVP.  Set to KMA_NULL_LATLON if not found.     */
    double longitude_deg;                       /* Longitude in degrees from SVP.  Set to KMA_NULL_LATLON if not found.    */
} kma_datagram_svp_info;


/* KMA Sound Velocity Profile Sample Version 1 (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float depth_m;                              /* Depth at which measurement is taken in meters (0 - 12000 m).            */
    float soundVelocity_mPerSec;                /* Sound velocity from profile in m/s.  For CTD this is calculated.        */
    uint32_t padding;                           /* Formerly absorption coefficient (Removed in version 1, Rev. F).         */
    float temp_C;                               /* Water temperature at given depth in Celsius.  For SVP this is zero.     */
    float salinity;                             /* Salinity of water at given depth in PSU.  For SVP this is zero.         */
                                                /*  NOTE: Salinity null value appears to be -99.0.                         */
} kma_datagram_svp_sample;


/* KMA Sound Velocity Profile Datagram */
typedef struct {
    kma_datagram_svp_info *info;                /* Pointer to SVP info.                                                    */
    kma_datagram_svp_sample *data;              /* Array of SVP samples of length numSamples.                              */
} kma_datagram_svp;


/* KMA Sound Velocity at Transducer Info (20 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesInfoPart;                  /* Size in bytes of current struct.  Used for denoting size of rest of     */
                                                /*  datagram in cases where only one data block is attached.               */
    uint16_t sensorStatus;                      /* Sensor status.  To indicate quality of sensor data is valid or invalid. */
                                                /*  Quality may be invalid even if sensor is active and the PU receives    */
                                                /*  data.  Bit code varies according to type of sensor.                    */
                                                /*  Bits 0-7 common to all sensors and #MRZ sensor status:                 */
                                                /*   Bit Number   Sensor Data                                              */
                                                /*    0            1 - Sensor is chosen as active.                         */
                                                /*    1            0                                                       */
                                                /*    2            0 - Data OK                                             */
                                                /*                 1 - Reduced performance                                 */
                                                /*    3            0                                                       */
                                                /*    4            0 - Data OK                                             */
                                                /*                 1 - Invalid data                                        */
                                                /*    5            0                                                       */
                                                /*    6            0                                                       */

    uint16_t sensorInputFormat;                 /* Format of raw data from input sensor given in numerical code:           */
                                                /*  Code   Sensor Format                                                   */
                                                /*   1      AML NMEA                                                       */
                                                /*   2      AML SV                                                         */
                                                /*   3      AML SVT                                                        */
                                                /*   4      AML SVP                                                        */
                                                /*   5      Micro SV                                                       */
                                                /*   6      Micro SVT                                                      */
                                                /*   7      Micro SVP                                                      */
                                                /*   8      Valeport MiniSVS                                               */
                                                /*   9      KSSIS 80                                                       */
                                                /*  10      KSSIS 43                                                       */

    uint16_t numSamples;                        /* Number of sensor samples added in this datagram.                        */
    uint16_t numBytesPerSample;                 /* Length in bytes of one whole SVT sensor sample.                         */

    uint16_t sensorDataContents;                /* Field to indicate which information is available from the input sensor  */
                                                /*  at the given sensor format.  0 - Not available, 1 - Data is available. */
                                                /* Expected data field in sensor input:                                    */
                                                /*  Bit Number    Sensor Data                                              */
                                                /*   0             Sound velocity                                          */
                                                /*   1             Temperature                                             */
                                                /*   2             Pressure                                                */
                                                /*   3             Salinity                                                */

    float filterTime_sec;                       /* Time parameter for moving median filter in seconds.                     */
    float soundVelocity_offset_mPerSec;         /* Offset for measured sound velocity set in K-Controller in m/s.          */
} kma_datagram_svt_info;


/* KMA Sound Velocity at Transducer Sample Data (24 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t time_sec;                          /* UTC Time in seconds.  Epoch 1970-01-01.                                 */
    uint32_t time_nanosec;                      /* UTC Time in nano-seconds remainder.                                     */
    float soundVelocity_mPerSec;                /* Measured sound velocity from sound velocity probe in m/s.               */
    float temp_C;                               /* Water temperature from sound velocity probe in Celsius.                 */
    float pressure_Pa;                          /* Pressure in Pascal.                                                     */
    float salinity;                             /* Salinity of water in g salt / kg sea water.                             */
} kma_datagram_svt_sample;


/* KMA Sound Velocity at Transducer Datagram */
typedef struct {
    kma_datagram_svt_info *info;                /* Sound velocity at transducer info.                                      */
    kma_datagram_svt_sample *data;              /* Array of sound velocity at transducer data samples.                     */
} kma_datagram_svt;


/* KMA SCL Data (8 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float offset_sec;                           /* Offset in seconds from K-Controller operator input.                     */
    int32_t clockDevPU_nanosec;                 /* Clock deviation from PU.  Difference between time stamp at receive of   */
                                                /*  sensor data and time in the clock source in nanoseconds.  Difference   */
                                                /*  smaller than +/- 1 second if 1PPS is active and sync from ZDA.         */
} kma_datagram_scl_data;


/* KMA Sensor Clock Datagram */
typedef struct {
    kma_datagram_s_common *common;              /* Common part for all external sensors.                                   */
    kma_datagram_scl_data *data;                /* Clock offsets.                                                          */
    uint8_t *dataFromSensor;                    /* Clock data as received from sensor, in text format.  Data are           */
                                                /*  uncorrected for offsets.                                               */
} kma_datagram_scl;


/* KMA SDE Data Version 0 (28 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float depthUsed_m;                          /* Depth as used in meters.  Corrected with installation parameters.       */
    float offset;                               /* Offset used in measuring this sample.                                   */
    float scale;                                /* Scaling factor for depth.                                               */
    double latitude_deg;                        /* Latitude in degrees.  Negative if southern hemisphere.  Position        */
                                                /*  extracted from the SVP.  Set to KMA_NULL_LATLON if not available.      */
    double longitude_deg;                       /* Longitude in degrees.  Negative if southern hemisphere.  Position       */
                                                /*  extracted from the SVP.  Set to KMA_NULL_LATLON if not available.      */
} kma_datagram_sde_data_v0;


/* KMA SDE Data Version 1 (32 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    float depthUsed_m;                          /* Depth as used in meters.  Corrected with installation parameters.       */
    float depthRaw_m;                           /* Raw depth reading from sensor, scaled and offset using sensor           */
                                                /*  parameters in meters.  NOTE: Added in Rev I; SDE datagram version 1.   */
    float offset;                               /* Offset used in measuring this sample.                                   */
    float scale;                                /* Scaling factor for depth.                                               */
    double latitude_deg;                        /* Latitude in degrees.  Negative if southern hemisphere.  Position        */
                                                /*  extracted from the SVP.  Set to KMA_NULL_LATLON if not available.      */
    double longitude_deg;                       /* Longitude in degrees.  Negative if southern hemisphere.  Position       */
                                                /*  extracted from the SVP.  Set to KMA_NULL_LATLON if not available.      */
} kma_datagram_sde_data_v1;


/* KMA SDE Data */
typedef struct {
    kma_datagram_sde_data_v0 *v0;               /* Pointer to dgmVersion=0 struct.                                         */
    kma_datagram_sde_data_v1 *v1;               /* Pointer to dgmVersion=1 struct.                                         */
} kma_datagram_sde_data;


/* KMA Sensor Depth Datagram */
typedef struct {
    kma_datagram_s_common *common;              /* Common part for all external sensors.                                   */
    kma_datagram_sde_data data;                 /* Depth as used, offsets, scale factor and data as received from sensor.  */
    uint8_t *dataFromSensor;
} kma_datagram_sde;


/* KMA SHI Data (6 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t sensorType;                        /* TODO: Undocumented.                                                     */
    float heigthUsed_m;                         /* Height corrected using installation parameters, if any, in meters.      */
} kma_datagram_shi_data;


/* KMA Sensor Height Datagram */
typedef struct {
    kma_datagram_s_common *common;              /* Common part for all external sensors.                                   */
    kma_datagram_shi_data *data;                /* Corrected and uncorrected data as received from sensor.                 */
    uint8_t *dataFromSensor;
} kma_datagram_shi;


/* KMA CPO Data (40 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint32_t timeFromSensor_sec;                /* UTC time from position sensor in seconds.  Epoch 1970-01-01.            */
    uint32_t timeFromSensor_nanosec;            /* UTC time from position sensor in nano-seconds remainder.                */
    float posFixQuality_m;                      /* Only available as input from sensor.  Calculation according to format.  */

    double correctedLat_deg;                    /* Motion corrected (if enabled in K-Controller) data as used in depth     */
                                                /*  calculations in degrees. Referred to antenna footprint at water level. */
                                                /*  Set to KMA_NULL_LATLON if sensor inactive.                             */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    double correctedLong_deg;                   /* Motion corrected (if enabled in K-Controller) data as used in depth     */
                                                /*  calculations in degrees. Referred to antenna footprint at water level. */
                                                /*  Set to KMA_NULL_LATLON if sensor inactive.                             */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    float speedOverGround_mPerSec;              /* Speed over ground in m/s.  Motion corrected (if enabled in              */
                                                /*  K-Controller) data as used in depth calculations.  If unavailable or   */
                                                /*  from inactive sensor, then value set to KMA_NULL_SPEED.                */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    float courseOverGround_deg;                 /* Course over ground in degrees.  Motion corrected (if enabled in         */
                                                /*  K-Controller) data as used in depth calculations.  If unavailable or   */
                                                /*  from inactive sensor, then value set to KMA_NULL_COURSE.               */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */

    float ellipsoidHeightReRefPoint_m;          /* Height of antenna footprint above the ellipsoid in meters.  Motion      */
                                                /*  corrected (if enabled in K-Controller) data as used in depth           */
                                                /*  calculations.  If unavailable or from inactive sensor, then value set  */
                                                /*  to KMA_NULL_ELLIPSOIDAL_HEIGHT.                                        */
                                                /* NOTE: Now added for inactive sensors in Rev. I.                         */
} kma_datagram_cpo_data;


/* KMA Compatibility Position Sensor Data */
typedef struct {
    kma_datagram_s_common *common;              /* Common part for all external sensors.                                   */
    kma_datagram_cpo_data *data;                /* Pointer to CPO data.                                                    */
    uint8_t *dataFromSensor;                    /* Position data as received from sensor, i.e.,uncorrected for motion,etc. */
} kma_datagram_cpo;


/* KMA CHE Data (4 Bytes) */
typedef struct {
    float heave_m;                              /* Heave in meters.  Positive downwards.                                   */
} kma_datagram_che_data;


/* KMA Compatibility Heave Data */
typedef struct {
    kma_datagram_m_common *common;              /* Information of transmitter and receiver used to find data in datagram.  */
    kma_datagram_che_data *data;                /* Pointer to CHE data.                                                    */
} kma_datagram_che;


/* KMA F Common (8 Bytes) */
typedef struct CPL_ATTRIBUTE_PACKED CPL_ATTRIBUTE_GCC_STRUCT {
    uint16_t numBytesCmnPart;                   /* Size in bytes of body part struct.                                      */
    int8_t fileStatus;                          /* File status: -1 = No file found, 0 = Ok, 1 = File too large (cropped).  */
    uint8_t padding;                            /* Padding.                                                                */
    uint32_t numBytesFile;                      /* File size in bytes.                                                     */
} kma_datagram_f_common;


/* KMA Calibration File */
typedef struct {
    kma_datagram_m_partition *partition;        /* Data partition information.                                             */
    kma_datagram_f_common *common;              /* Common for all file datagrams.                                          */
    char *fileName;                             /* Name of file.                                                           */
    uint8_t *bsCalibrationFile;                 /* File containing the measured backscatter offsets.                       */
} kma_datagram_fcf;


/******************************* KMA DATA TYPES ******************************/

/* KMA Datagram */
typedef union {
    kma_datagram_iip iip;
    kma_datagram_iop iop;
    kma_datagram_ibe ibe;
    kma_datagram_ibr ibr;
    kma_datagram_ibs ibs;
    kma_datagram_mrz mrz;
    kma_datagram_mwc mwc;
    kma_datagram_spo spo;
    kma_datagram_skm skm;
    kma_datagram_svp svp;
    kma_datagram_svt svt;
    kma_datagram_scl scl;
    kma_datagram_sde sde;
    kma_datagram_shi shi;
    kma_datagram_cpo cpo;
    kma_datagram_che che;
    kma_datagram_fcf fcf;
} kma_datagram;


/* KMA Data */
typedef struct {
    kma_datagram_header header;
    kma_datagram datagram;
} kma_data;


/* Opaque KMA File Handle Type */
typedef struct kma_handle_struct kma_handle;


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

kma_handle * kma_open (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
int kma_close (kma_handle *);
kma_data * kma_read (kma_handle *) CPL_ATTRIBUTE_NONNULL_ALL;
void kma_print (FILE *, const kma_data *, const int) CPL_ATTRIBUTE_NONNULL (1);
void kma_set_ignore_mwc (kma_handle *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
void kma_set_ignore_mrz (kma_handle *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
const char * kma_get_datagram_name (const uint32_t) CPL_ATTRIBUTE_RETURNS_NONNULL CPL_ATTRIBUTE_PURE;
const uint8_t * kma_get_mwc_rx_beam_data (kma_datagram_mwc_rx_beam *, const uint8_t *, const uint8_t, const uint8_t) CPL_ATTRIBUTE_NONNULL_ALL;
int kma_get_errno (const kma_handle *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int kma_identify (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void kma_set_debug (const int);


CPL_CLINKAGE_END

#endif /* KMA_READER_H */
