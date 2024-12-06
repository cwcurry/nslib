/* emx_reader.c -- Read Kongsberg Maritime EM Datagram format.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "emx_reader.h"
#include "cpl_timedate.h"
#include "cpl_debug.h"
#include "cpl_alloc.h"
#include "cpl_error.h"
#include "cpl_bswap.h"
#include "cpl_file.h"
#include "cpl_str.h"


/* EMX Datagram Identifier */
#define EMX_START_BYTE      0x02
#define EMX_END_BYTE        0x03

/* EMX Max TX Sectors */
#define EMX_MAX_TX_SECTORS  20


/* EMX File Handle */
struct emx_handle_struct {
    char *buffer;                      /* File I/O buffer.            */
    size_t buffer_size;                /* Allocated buffer size.      */
    emx_data d;                        /* Static emx_data struct.     */
    int fd;                            /* File descriptor.            */
    int emx_errno;                     /* Error condition code.       */
    int ignore_wc;                     /* Boolean to ignore WC data.  */
    int ignore_checksum;               /* Boolean to ignore checksum. */
    int swap;                          /* Boolean to byte-swap data.  */
    size_t hisas_bytes_per_sample[6];  /* HISAS data bytes per sample. */
};


/* Private Function Prototypes */
static int emx_get_header (emx_datagram_header *, int *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
static int emx_valid_header (const emx_datagram_header *) CPL_ATTRIBUTE_NONNULL_ALL;
static int emx_valid_date (const uint32_t) CPL_ATTRIBUTE_PURE;
static int emx_byte_order (const uint32_t, const uint16_t) CPL_ATTRIBUTE_PURE;
static void emx_swap_header (emx_datagram_header *) CPL_ATTRIBUTE_NONNULL_ALL;
static void emx_swap_datagram (emx_datagram *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
static int emx_valid_checksum (const emx_datagram_header *, const char *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
static int set_buffer_size (emx_handle *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;


/* Private Variables */
static int emx_debug = 0;


/******************************************************************************
*
* emx_open - Open the file given by FILE_NAME and return a handle to it.  The
*   caller must call emx_close() to close the file and free the memory after use.
*
* Return: A file handle pointer to the open file, or
*         NULL if the file can not be opened for reading.
*
******************************************************************************/

emx_handle * emx_open (
    const char *file_name) {

    emx_handle *h;
    int fd;


    /* Make sure there is no padding in the data structs. */
    assert (sizeof (emx_datagram_header) == 20);
    assert (sizeof (emx_datagram_depth_info) == 12);
    assert (sizeof (emx_datagram_depth_beam) == 16);
    assert (sizeof (emx_datagram_xyz_info) == 20);
    assert (sizeof (emx_datagram_xyz_beam) == 20);
    assert (sizeof (emx_datagram_depth_nominal_info) == 8);
    assert (sizeof (emx_datagram_depth_nominal_beam) == 14);
    assert (sizeof (emx_datagram_extra_detect_info) == 36);
    assert (sizeof (emx_datagram_extra_detect_class) == 16);
    assert (sizeof (emx_datagram_extra_detect_data) == 68);
    assert (sizeof (emx_datagram_central_beams_info) == 16);
    assert (sizeof (emx_datagram_central_beams_data) == 6);
    assert (sizeof (emx_datagram_rra_101_info) == 30);
    assert (sizeof (emx_datagram_rra_101_tx_beam) == 12);
    assert (sizeof (emx_datagram_rra_101_rx_beam) == 16);
    assert (sizeof (emx_datagram_rra_70_info) == 4);
    assert (sizeof (emx_datagram_rra_70_beam) == 8);
    assert (sizeof (emx_datagram_rra_102_info) == 20);
    assert (sizeof (emx_datagram_rra_102_tx_beam) == 20);
    assert (sizeof (emx_datagram_rra_102_rx_beam) == 12);
    assert (sizeof (emx_datagram_rra_78_info) == 16);
    assert (sizeof (emx_datagram_rra_78_tx_beam) == 24);
    assert (sizeof (emx_datagram_rra_78_rx_beam) == 16);
    assert (sizeof (emx_datagram_seabed_83_info) == 16);
    assert (sizeof (emx_datagram_seabed_83_beam) == 6);
    assert (sizeof (emx_datagram_seabed_89_info) == 16);
    assert (sizeof (emx_datagram_seabed_89_beam) == 6);
    assert (sizeof (emx_datagram_wc_info) == 24);
    assert (sizeof (emx_datagram_wc_tx_beam) == 6);
    assert (sizeof (emx_datagram_wc_rx_beam_info) == 10);
    assert (sizeof (emx_datagram_qf_info) == 4);
    assert (sizeof (emx_datagram_attitude_info) == 2);
    assert (sizeof (emx_datagram_attitude_data) == 12);
    assert (sizeof (emx_datagram_attitude_network_info) == 4);
    assert (sizeof (emx_datagram_attitude_network_data_info) == 11);
    assert (sizeof (emx_datagram_clock_info) == 9);
    assert (sizeof (emx_datagram_height_info) == 5);
    assert (sizeof (emx_datagram_heading_info) == 2);
    assert (sizeof (emx_datagram_heading_data) == 4);
    assert (sizeof (emx_datagram_position_info) == 18);
    assert (sizeof (emx_datagram_sb_depth_info) == 13);
    assert (sizeof (emx_datagram_tide_info) == 11);
    assert (sizeof (emx_datagram_sssv_info) == 2);
    assert (sizeof (emx_datagram_sssv_data) == 4);
    assert (sizeof (emx_datagram_svp_info) == 12);
    assert (sizeof (emx_datagram_svp_data) == 8);
    assert (sizeof (emx_datagram_svp_em3000_data) == 4);
    assert (sizeof (emx_datagram_install_params_info) == 2);
    assert (sizeof (emx_datagram_runtime_params_info) == 33);
    assert (sizeof (emx_datagram_extra_params_info) == 2);
    assert (sizeof (emx_datagram_pu_output_info) == 88);
    assert (sizeof (emx_datagram_pu_status_info) == 69);
    assert (sizeof (emx_datagram_pu_bist_result_info) == 4);
    assert (sizeof (emx_datagram_tilt_info) == 2);
    assert (sizeof (emx_datagram_tilt_data) == 4);
    assert (sizeof (emx_datagram_hisas_status_info) == 100);
    assert (sizeof (emx_datagram_sidescan_status_channel) == 128);
    assert (sizeof (emx_datagram_sidescan_status_info) == 1025);
    assert (sizeof (emx_datagram_sidescan_data_info) == 256);
    assert (sizeof (emx_datagram_sidescan_data_channel_info) == 64);
    assert (sizeof (emx_datagram_navigation_output_info) == 112);

    if (!file_name) return NULL;

    cpl_debug (emx_debug, "Opening file '%s'\n", file_name);

    /* Open the binary file for read-only access and save the file descriptor. */
    fd = cpl_open (file_name, CPL_OPEN_RDONLY | CPL_OPEN_BINARY | CPL_OPEN_SEQUENTIAL);

    if (fd < 0) {
        cpl_debug (emx_debug, "Failed to open file '%s'\n", file_name);
        return NULL;
    }

    /* Allocate memory storage for the handle.  This must be free'd later. */
    h = (emx_handle *) cpl_malloc (sizeof (emx_handle));
    if (!h) {
        cpl_close (fd);
        return NULL;
    }

    /* Initialize the file buffer. */
    h->buffer_size = 0;
    h->buffer = NULL;

    /* Initialize the file handle. */
    h->emx_errno = CS_ENONE;
    h->fd = fd;

    /* Set boolean to ignore watercolumn data to false by default. */
    h->ignore_wc = 0;

    /* Set boolean to ignore the datagram checksum to false by default. */
    h->ignore_checksum = 0;

    /* Set boolean to byte swap to be undefined. */
    h->swap = -1;

    h->hisas_bytes_per_sample[0] = 0;
    h->hisas_bytes_per_sample[1] = 0;
    h->hisas_bytes_per_sample[2] = 0;
    h->hisas_bytes_per_sample[3] = 0;
    h->hisas_bytes_per_sample[4] = 0;
    h->hisas_bytes_per_sample[5] = 0;

    return h;
}


/******************************************************************************
*
* emx_close - Close the file and free allocated memory given by the handle H.
*
* Errors: CS_ECLOSE
*
******************************************************************************/

int emx_close (
    emx_handle *h) {

    int status = CS_ENONE;


    if (h) {
        cpl_debug (emx_debug, "Closing file\n");

        /* Close the file if it wasn't already closed. */
        if (h->fd != -1) {
            if (cpl_close (h->fd) != 0) {
                status = CS_ECLOSE;
            }
            h->fd = -1;
        }

        /* Free the internal storage buffer. */
        if (h->buffer) {
            cpl_free (h->buffer);
            h->buffer = NULL;
        }

        /* Free the file handle. */
        cpl_free (h);
    }

    return status;
}


/******************************************************************************
*
* emx_read - Read the datagram and the channel data from the file given by the
*   handle H.  Data is read at the current file position.  When finished the
*   file pointer will be at the start of the next datagram.  The memory
*   storage for the datagram and channel data will be overwritten in
*   subsequent calls to this function or if the file is closed.  If they are to
*   be preserved, then they should be copied to a user-defined buffer location.
*
* Return: A pointer to the data struct if the data was read successfully,
*         NULL if no valid data was found or EOF was reached.
*
* Errors: CS_ENOMEM
*         CS_EREAD
*         CS_ESEEK
*         CS_EBADDATA
*         CS_EUNSUP
*
******************************************************************************/

emx_data * emx_read (
    emx_handle *h) {

    size_t actual_read_size;
    size_t read_size;
    size_t n;
    ssize_t result;
    int i, status;
    char *p;


    assert (h);

    /* Read the datagram header from the file descriptor.  The header will be validated and byte swapped as needed. */
L1: status = emx_get_header (&h->d.header, &h->swap, h->fd);

    /* Check for EOF. */
    if (status == 0) return NULL;

    /* Check for error condition. */
    if (status < 0) {
        h->emx_errno = status;
        return NULL;
    }

    cpl_debug (emx_debug >= 2, "bytes_in_datagram=%5u, model=%u, date=%u, time_ms=%u, counter=%5u, serial=%u, datagram_type=%3u (%s)\n",
        h->d.header.bytes_in_datagram, h->d.header.em_model_number, h->d.header.date, h->d.header.time_ms, h->d.header.counter,
        h->d.header.serial_number, h->d.header.datagram_type, emx_get_datagram_name (h->d.header.datagram_type));

    /* Now read the rest of the datagram.  The size to read is the datagram size plus a four-byte
       field at the end of the datagram minus the header we just read. */
    read_size = h->d.header.bytes_in_datagram + sizeof (uint32_t) - sizeof (emx_datagram_header);

    /* Skip any water column datagrams if requested.  These datagrams are large, so if
       the read time is greater than the seek time on the storage device, then seeking
       past this data is advantageous if this data is not desired. */
    if (h->ignore_wc && (h->d.header.datagram_type == EMX_DATAGRAM_WATER_COLUMN)) {
        if (cpl_seek (h->fd, read_size, SEEK_CUR) < 0) {
            cpl_debug (emx_debug, "Seek failed\n");
            h->emx_errno = CS_ESEEK;
            return NULL;
        }

        /* Now go back and read another datagram. */
        goto L1;
    }

    /* Resize the buffer as needed to fit the datagram size. */
    if (set_buffer_size (h, read_size) != 0) {
        h->emx_errno = CS_ENOMEM;
        return NULL;
    }

    /* Read the rest of the datagram from the file descriptor to the buffer. */
    result = cpl_read (h->buffer, read_size, h->fd);
    if (result < 0) {
        cpl_debug (emx_debug, "Read error occurred\n");
        h->emx_errno = CS_EREAD;
        return NULL;
    }

    actual_read_size = (size_t) result;

    /* Make sure we read at least the minimum size. */
    if (actual_read_size != read_size) {
        /* If the expected amount was not read, then the file is corrupt. */
        cpl_debug (emx_debug, "Unexpected end of file\n");
        h->emx_errno = CS_EBADDATA;
        return NULL;
    }

    /* Have found data with an undocumented datagram of type 0x74 ('t') that does
       not appear to have a valid checksum or date/timestamp.  The datagram appears
       to contain some directory information.  Need to skip this datagram here. */
    if (h->d.header.datagram_type != EMX_DATAGRAM_UNKNOWN2) {

        /* Verify the checksum and end identifier are valid. */
        if (emx_valid_checksum (&(h->d.header), h->buffer, h->swap) == 0) {
            /* Checksum errors appear to be common in older data.  If we have an error then discard this datagram and
               read another unless requested to continue processing it. */
            if (h->ignore_checksum == 0) {
                cpl_debug (emx_debug, "Discarding bad datagram\n");
                goto L1;
            }
        }
    }

    /* Set the pointer p to the buffer, which is the start of the datagram (after the header). */
    p = h->buffer;

    /* Set the pointers of the datagram array (channel) data into the correct places in the buffer. */
    switch (h->d.header.datagram_type) {

        /* Byte-swapping is done after setting the pointers here, so if any parameters are needed to set the
           pointers, then they must be byte-swapped here, but not for single-byte parameters. */

        /* TODO: We should verify that the pointer P never points passed the end of the buffer in the following code. */

        case EMX_DATAGRAM_DEPTH :

            h->d.datagram.depth.info = (emx_datagram_depth_info *) p;
            p += sizeof (emx_datagram_depth_info);
            h->d.datagram.depth.beam = (emx_datagram_depth_beam *) p;
            p += h->d.datagram.depth.info->num_beams * sizeof (emx_datagram_depth_beam);
            h->d.datagram.depth.depth_offset_multiplier = (int8_t) *p;

            break;

        case EMX_DATAGRAM_DEPTH_NOMINAL :

            h->d.datagram.depth_nominal.info = (emx_datagram_depth_nominal_info *) p;
            p += sizeof (emx_datagram_depth_nominal_info);
            h->d.datagram.depth_nominal.beam = (emx_datagram_depth_nominal_beam *) p;

            break;

        case EMX_DATAGRAM_XYZ :

            h->d.datagram.xyz.info = (emx_datagram_xyz_info *) p;
            p += sizeof (emx_datagram_xyz_info);
            h->d.datagram.xyz.beam = (emx_datagram_xyz_beam *) p;

            break;

        case EMX_DATAGRAM_EXTRA_DETECTIONS :

            {
                uint16_t datagram_version;
                uint16_t num_bytes;

                h->d.datagram.extra_detect.info = (emx_datagram_extra_detect_info *) p;

                /* Extract the datagram version and make sure it is set to 1. */
                if (h->swap) {
                    datagram_version = CPL_BSWAP (h->d.datagram.extra_detect.info->datagram_version);
                } else {
                    datagram_version = h->d.datagram.extra_detect.info->datagram_version;
                }

                if (datagram_version != 1) {
                    cpl_debug (emx_debug, "Invalid Extra Detects datagram version (%d)\n", datagram_version);
                    goto L1;
                }

                /* Extract the number of bytes for the Extra Detects Class and make sure it is set to 16 bytes. */
                if (h->swap) {
                    num_bytes = CPL_BSWAP (h->d.datagram.extra_detect.info->nbytes_class);
                } else {
                    num_bytes = h->d.datagram.extra_detect.info->nbytes_class;
                }

                if (num_bytes != sizeof (emx_datagram_extra_detect_class)) {
                    cpl_debug (emx_debug, "Invalid Extra Detect class size (%d)\n", num_bytes);
                    goto L1;
                }

                /* Extract the number of bytes for the Extra Detects Data and make sure it is set to 68 bytes. */
                if (h->swap) {
                    num_bytes = CPL_BSWAP (h->d.datagram.extra_detect.info->nbytes_detect);
                } else {
                    num_bytes = h->d.datagram.extra_detect.info->nbytes_detect;
                }

                if (num_bytes != sizeof (emx_datagram_extra_detect_data)) {
                    cpl_debug (emx_debug, "Invalid Extra Detect data size (%d)\n", num_bytes);
                    goto L1;
                }

                p += sizeof (emx_datagram_extra_detect_info);
                h->d.datagram.extra_detect.classes = (emx_datagram_extra_detect_class *) p;
                if (h->swap) {
                    n = ((size_t) CPL_BSWAP (h->d.datagram.extra_detect.info->num_classes)) * CPL_BSWAP (h->d.datagram.extra_detect.info->nbytes_class);
                } else {
                    n = ((size_t) h->d.datagram.extra_detect.info->num_classes) * h->d.datagram.extra_detect.info->nbytes_class;
                }
                p += n;
                h->d.datagram.extra_detect.data = (emx_datagram_extra_detect_data *) p;
                if (h->swap) {
                    n = ((size_t) CPL_BSWAP (h->d.datagram.extra_detect.info->num_detects)) * CPL_BSWAP (h->d.datagram.extra_detect.info->nbytes_detect);
                } else {
                    n = ((size_t) h->d.datagram.extra_detect.info->num_detects) * h->d.datagram.extra_detect.info->nbytes_detect;
                }
                p += n;
                h->d.datagram.extra_detect.raw_amplitude = (int16_t *) p;
            }

            break;

        case EMX_DATAGRAM_CENTRAL_BEAMS :

            h->d.datagram.central_beams.info = (emx_datagram_central_beams_info *) p;
            p += sizeof (emx_datagram_central_beams_info);
            h->d.datagram.central_beams.beam = (emx_datagram_central_beams_data *) p;
            p += h->d.datagram.central_beams.info->num_beams * sizeof (emx_datagram_central_beams_info);
            h->d.datagram.central_beams.amplitude = (int8_t *) p;

            break;

        case EMX_DATAGRAM_RRA_101 :

            h->d.datagram.rra_101.info = (emx_datagram_rra_101_info *) p;
            p += sizeof (emx_datagram_rra_101_info);
            h->d.datagram.rra_101.tx_beam = (emx_datagram_rra_101_tx_beam *) p;
            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.rra_101.info->tx_sectors);
            } else {
                n = h->d.datagram.rra_101.info->tx_sectors;
            }
            if (n > EMX_MAX_TX_SECTORS) {
                cpl_debug (emx_debug, "Invalid number of TX sectors (%lu)\n", (unsigned long) n);
                h->emx_errno = CS_EBADDATA;
                return NULL;
            }
            p += n * sizeof (emx_datagram_rra_101_tx_beam);
            h->d.datagram.rra_101.rx_beam = (emx_datagram_rra_101_rx_beam *) p;

            break;

        case EMX_DATAGRAM_RRA_70 :

            h->d.datagram.rra_70.info = (emx_datagram_rra_70_info *) p;
            p += sizeof (emx_datagram_rra_70_info);
            h->d.datagram.rra_70.beam = (emx_datagram_rra_70_beam *) p;

            break;

        case EMX_DATAGRAM_RRA_102 :

            h->d.datagram.rra_102.info = (emx_datagram_rra_102_info *) p;
            p += sizeof (emx_datagram_rra_102_info);
            h->d.datagram.rra_102.tx_beam = (emx_datagram_rra_102_tx_beam *) p;
            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.rra_102.info->tx_sectors);
            } else {
                n = h->d.datagram.rra_102.info->tx_sectors;
            }
            if (n > EMX_MAX_TX_SECTORS) {
                cpl_debug (emx_debug, "Invalid number of TX sectors (%lu)\n", (unsigned long) n);
                h->emx_errno = CS_EBADDATA;
                return NULL;
            }
            p += n * sizeof (emx_datagram_rra_102_tx_beam);
            h->d.datagram.rra_102.rx_beam = (emx_datagram_rra_102_rx_beam *) p;

            break;

        case EMX_DATAGRAM_RRA_78 :

            h->d.datagram.rra_78.info = (emx_datagram_rra_78_info *) p;
            p += sizeof (emx_datagram_rra_78_info);
            h->d.datagram.rra_78.tx_beam = (emx_datagram_rra_78_tx_beam *) p;
            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.rra_78.info->tx_sectors);
            } else {
                n = h->d.datagram.rra_78.info->tx_sectors;
            }
            if (n > EMX_MAX_TX_SECTORS) {
                cpl_debug (emx_debug, "Invalid number of TX sectors (%lu)\n", (unsigned long) n);
                h->emx_errno = CS_EBADDATA;
                return NULL;
            }
            p += n * sizeof (emx_datagram_rra_78_tx_beam);
            h->d.datagram.rra_78.rx_beam = (emx_datagram_rra_78_rx_beam *) p;

            break;

        case EMX_DATAGRAM_SEABED_IMAGE_83 :

            h->d.datagram.seabed_83.info = (emx_datagram_seabed_83_info *) p;
            p += sizeof (emx_datagram_seabed_83_info);
            h->d.datagram.seabed_83.beam = (emx_datagram_seabed_83_beam *) p;
            p += h->d.datagram.seabed_83.info->num_beams * sizeof (emx_datagram_seabed_83_beam);
            h->d.datagram.seabed_83.amplitude = (int8_t *) p;

            h->d.datagram.seabed_83.bytes_end = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header) - sizeof (emx_datagram_seabed_83_info) -
                h->d.datagram.seabed_83.info->num_beams * sizeof (emx_datagram_seabed_83_beam) + 1;  /* Add one for possible spare byte. */

            break;

        case EMX_DATAGRAM_SEABED_IMAGE_89 :

            h->d.datagram.seabed_89.info = (emx_datagram_seabed_89_info *) p;
            p += sizeof (emx_datagram_seabed_89_info);
            h->d.datagram.seabed_89.beam = (emx_datagram_seabed_89_beam *) p;
            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.seabed_89.info->num_beams);
            } else {
                n = h->d.datagram.seabed_89.info->num_beams;
            }
            p += n * sizeof (emx_datagram_seabed_89_beam);
            h->d.datagram.seabed_89.amplitude = (int16_t *) p;

            h->d.datagram.seabed_89.bytes_end = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header) - sizeof (emx_datagram_seabed_89_info) -
                n * sizeof (emx_datagram_seabed_89_beam) + 1;  /* Add one for possible spare byte. */

            break;

        case EMX_DATAGRAM_WATER_COLUMN :

            h->d.datagram.wc.info = (emx_datagram_wc_info *) p;
            p += sizeof (emx_datagram_wc_info);
            h->d.datagram.wc.txbeam = (emx_datagram_wc_tx_beam *) p;
            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.wc.info->tx_sectors);
            } else {
                n = h->d.datagram.wc.info->tx_sectors;
            }
            if (n > EMX_MAX_TX_SECTORS) {
                cpl_debug (emx_debug, "Invalid number of TX sectors (%lu)\n", (unsigned long) n);
                h->emx_errno = CS_EBADDATA;
                return NULL;
            }
            p += n * sizeof (emx_datagram_wc_tx_beam);
            h->d.datagram.wc.beamData = (uint8_t *) p;

            break;

        case EMX_DATAGRAM_QUALITY_FACTOR :

            h->d.datagram.qf.info = (emx_datagram_qf_info *) p;
            p += sizeof (emx_datagram_qf_info);

            if (h->d.datagram.qf.info->npar > 1) {
                cpl_debug (emx_debug, "Unsupported number of parameters per beam (%u)\n", h->d.datagram.qf.info->npar);
                h->emx_errno = CS_EUNSUP;
                return NULL;
            }

            h->d.datagram.qf.data = (float *) p;

            break;

        case EMX_DATAGRAM_ATTITUDE :

            h->d.datagram.attitude.info = (emx_datagram_attitude_info *) p;
            p += sizeof (emx_datagram_attitude_info);
            h->d.datagram.attitude.data = (emx_datagram_attitude_data *) p;

            break;

        case EMX_DATAGRAM_ATTITUDE_NETWORK :

            h->d.datagram.attitude_network.info = (emx_datagram_attitude_network_info *) p;
            p += sizeof (emx_datagram_attitude_network_info);
            h->d.datagram.attitude_network.data = (uint8_t *) p;

            break;

        case EMX_DATAGRAM_CLOCK :

            h->d.datagram.clock.info = (emx_datagram_clock_info *) p;

            break;

        case EMX_DATAGRAM_HEIGHT :

            h->d.datagram.height.info = (emx_datagram_height_info *) p;

            break;

        case EMX_DATAGRAM_HEADING :

            h->d.datagram.heading.info = (emx_datagram_heading_info *) p;
            p += sizeof (emx_datagram_heading_info);

            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.heading.info->num_entries);
            } else {
                n = h->d.datagram.heading.info->num_entries;
            }

            h->d.datagram.heading.data = (emx_datagram_heading_data *) p;
            p += n * sizeof (emx_datagram_heading_data);
            h->d.datagram.heading.heading_indicator = (uint8_t) *p;

            break;

        case EMX_DATAGRAM_POSITION :

            h->d.datagram.position.info = (emx_datagram_position_info *) p;
            p += sizeof (emx_datagram_position_info);
            h->d.datagram.position.message = (uint8_t *) p;

            break;

        case EMX_DATAGRAM_SINGLE_BEAM_DEPTH :

            h->d.datagram.sb_depth.info = (emx_datagram_sb_depth_info *) p;

            break;

        case EMX_DATAGRAM_TIDE :

            h->d.datagram.tide.info = (emx_datagram_tide_info *) p;

            break;

        case EMX_DATAGRAM_SSSV :

            h->d.datagram.sssv.info = (emx_datagram_sssv_info *) p;
            p += sizeof (emx_datagram_sssv_info);
            h->d.datagram.sssv.data = (emx_datagram_sssv_data *) p;

            break;

        case EMX_DATAGRAM_SVP :

            h->d.datagram.svp.info = (emx_datagram_svp_info *) p;
            p += sizeof (emx_datagram_svp_info);
            h->d.datagram.svp.data = (emx_datagram_svp_data *) p;

            break;

        case EMX_DATAGRAM_SVP_EM3000 :

            h->d.datagram.svp_em3000.info = (emx_datagram_svp_em3000_info *) p;
            p += sizeof (emx_datagram_svp_em3000_info);
            h->d.datagram.svp_em3000.data = (emx_datagram_svp_em3000_data *) p;

            break;

        case EMX_DATAGRAM_KM_SSP_OUTPUT :

            h->d.datagram.ssp_output.data = p;
            h->d.datagram.ssp_output.num_bytes = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS :

            h->d.datagram.install_params.info = (emx_datagram_install_params_info *) p;
            p += sizeof (emx_datagram_install_params_info);
            h->d.datagram.install_params.text = p;
            h->d.datagram.install_params.text_length = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header) - sizeof (emx_datagram_install_params_info);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS_STOP :

            h->d.datagram.install_params_stop.info = (emx_datagram_install_params_info *) p;
            p += sizeof (emx_datagram_install_params_info);
            h->d.datagram.install_params_stop.text = p;
            h->d.datagram.install_params_stop.text_length = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header) - sizeof (emx_datagram_install_params_info);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS_REMOTE :

            h->d.datagram.install_params_remote.info = (emx_datagram_install_params_info *) p;
            p += sizeof (emx_datagram_install_params_info);
            h->d.datagram.install_params_remote.text = p;
            h->d.datagram.install_params_remote.text_length = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header) - sizeof (emx_datagram_install_params_info);

            break;

        case EMX_DATAGRAM_REMOTE_PARAMS_INFO :

            cpl_debug (emx_debug, "Unsupported Remote Params Info Datagram\n");

            /* TODO: */

            break;

        case EMX_DATAGRAM_RUNTIME_PARAMS :

            h->d.datagram.runtime_params.info = (emx_datagram_runtime_params_info *) p;

            break;

        case EMX_DATAGRAM_EXTRA_PARAMS :

            h->d.datagram.extra_params.info = (emx_datagram_extra_params_info *) p;
            p += sizeof (emx_datagram_extra_params_info);

            if (h->swap) {
                n = CPL_BSWAP (h->d.datagram.extra_params.info->content);
            } else {
                n = h->d.datagram.extra_params.info->content;
            }

            switch (n) {

                /* TODO: Add content 1-5 here. */

                case 6 :
                    h->d.datagram.extra_params.data.bs_corr.num_chars = *((uint16_t *) p);
                    p += sizeof (uint16_t);
                    h->d.datagram.extra_params.data.bs_corr.text = p;
                    break;

                default :
                    /* Alert us to unsupported content types. */
                    cpl_debug (emx_debug, "Extra Params Datagram with unknown content type (%lu)\n", (unsigned long) n);

                    break;
            }

            break;

        case EMX_DATAGRAM_PU_OUTPUT :

            h->d.datagram.pu_output.info = (emx_datagram_pu_output_info *) p;

            break;

        case EMX_DATAGRAM_PU_STATUS :

            h->d.datagram.pu_status.info = (emx_datagram_pu_status_info *) p;

            break;

        case EMX_DATAGRAM_PU_BIST_RESULT :

            h->d.datagram.pu_bist_result.info = (emx_datagram_pu_bist_result_info *) p;
            p += sizeof (emx_datagram_pu_bist_result_info);
            h->d.datagram.pu_bist_result.text = p;

            h->d.datagram.pu_bist_result.text_length = h->d.header.bytes_in_datagram - sizeof (emx_datagram_header) - sizeof (emx_datagram_pu_bist_result_info);

            break;

        case EMX_DATAGRAM_TRANSDUCER_TILT :

            h->d.datagram.tilt.info = (emx_datagram_tilt_info *) p;
            p += sizeof (emx_datagram_tilt_info);
            h->d.datagram.tilt.data = (emx_datagram_tilt_data *) p;

            break;

        case EMX_DATAGRAM_SYSTEM_STATUS :

            cpl_debug (emx_debug, "Unsupported System Status Datagram\n");

            /* TODO: */

            break;

        case EMX_DATAGRAM_STAVE :

            cpl_debug (emx_debug, "Unsupported Stave Datagram\n");

            /* TODO: */

            break;

        case EMX_DATAGRAM_HISAS_STATUS :

            h->d.datagram.hisas_status.info = (emx_datagram_hisas_status_info *) p;

            break;

        case EMX_DATAGRAM_SIDESCAN_STATUS :

            h->d.datagram.sidescan_status.info = (emx_datagram_sidescan_status_info *) p;

            if (h->d.datagram.sidescan_status.info->num_channels > 6) {
                cpl_debug (emx_debug, "Invalid number of sonar channels (%u)\n", h->d.datagram.sidescan_status.info->num_channels);
                h->emx_errno = CS_EBADDATA;
                return NULL;
            }

            for (i=0; i<h->d.datagram.sidescan_status.info->num_channels; i++) {

                if ((h->d.datagram.sidescan_status.info->channel[i].bytes_per_sample != 2) &&
                    (h->d.datagram.sidescan_status.info->channel[i].bytes_per_sample != 4) &&
                    (h->d.datagram.sidescan_status.info->channel[i].bytes_per_sample != 8)) {
                    cpl_debug (emx_debug, "Invalid bytes per sample (%u)\n", h->d.datagram.sidescan_status.info->channel[i].bytes_per_sample);
                    h->emx_errno = CS_EBADDATA;
                    return NULL;
                }

                /* Save the bytes per sample to the file handle since this will be needed by the EMX_DATAGRAM_HISAS_1032_SIDESCAN datagram. */
                h->hisas_bytes_per_sample[i] = h->d.datagram.sidescan_status.info->channel[i].bytes_per_sample;

                /* Make sure character strings are nul-terminated. */
                h->d.datagram.sidescan_status.info->channel[i].channel_name[15] = '\0';
            }

            break;

        case EMX_DATAGRAM_HISAS_1032_SIDESCAN :

            h->d.datagram.sidescan_data.info = (emx_datagram_sidescan_data_info *) p;

            if (h->d.datagram.sidescan_data.info->num_channels > 6) {
                cpl_debug (emx_debug, "Invalid number of sonar channels (%u)\n", h->d.datagram.sidescan_data.info->num_channels);
                h->emx_errno = CS_EBADDATA;
                return NULL;
            }

            p += sizeof (emx_datagram_sidescan_data_info);

            for (i=0; i<h->d.datagram.sidescan_data.info->num_channels; i++) {

                h->d.datagram.sidescan_data.channel[i].info = (emx_datagram_sidescan_data_channel_info *) p;

                /* If the bytes per sample is still zero, then we didn't encounter a sidescan status datagram. */
                if (h->hisas_bytes_per_sample[i] == 0) {
                    cpl_debug (emx_debug, "Sidescan status datagram not found\n");
                    h->emx_errno = CS_EBADDATA;
                    return NULL;
                }

                h->d.datagram.sidescan_data.channel[i].bytes_per_sample = h->hisas_bytes_per_sample[i];

                p += sizeof (emx_datagram_sidescan_data_channel_info);

                h->d.datagram.sidescan_data.channel[i].data.ptr = (emx_datagram_sidescan_data_sample *) p;

                p += h->hisas_bytes_per_sample[i] * h->d.datagram.sidescan_data.channel[i].info->num_samples;
            }

            break;

        case EMX_DATAGRAM_NAVIGATION_OUTPUT :

            h->d.datagram.navigation_output.info = (emx_datagram_navigation_output_info *) p;

            break;

        default :

            cpl_debug (emx_debug, "Unknown message type (%u) of size %u bytes\n",
                h->d.header.datagram_type, h->d.header.bytes_in_datagram);

            break;
    }

    /* Byte swap the datagram if necessary. */
    if (h->swap) {
        emx_swap_datagram (&h->d.datagram, h->d.header.datagram_type);
    }

    return &(h->d);
}


/******************************************************************************
*
* emx_get_wc_rxbeam - The water column data RX beam data stored in the EMX format
*   consists of the emx_datagram_wc_rx_beam_info struct followed by num_samples
*   of amplitude data.  This function will set the pointers in the Rx beam data
*   object D for the data stream starting at P.  The start of the next Rx beam
*   data stream will be returned.
*
******************************************************************************/

const uint8_t * emx_get_wc_rxbeam (
    emx_datagram_wc_rx_beam *d,
    const uint8_t *p) {

    assert (d);
    assert (p);

    d->info = (emx_datagram_wc_rx_beam_info *) p;
    p = p + sizeof (emx_datagram_wc_rx_beam_info);
    d->amplitude = (int8_t *) p;
    p = p + d->info->num_samples * sizeof (int8_t);

    return p;
}


/******************************************************************************
*
* emx_get_attitude_network_data - The attitude network data stored in the EMX
*   format consists of the emx_datagram_attitude_network_data_info struct
*   followed by num_bytes_input of the attitude message.  This function will
*   set the pointers in the attitude network data object D for the data stream
*   starting at P.  The start of the next attitude network data stream will be
*   returned.
*
******************************************************************************/

const uint8_t * emx_get_attitude_network_data (
    emx_datagram_attitude_network_data *d,
    const uint8_t *p) {

    assert (d);
    assert (p);

    d->info = (emx_datagram_attitude_network_data_info *) p;
    p = p + sizeof (emx_datagram_attitude_network_data_info);
    d->message = (uint8_t *) p;
    p = p + d->info->num_bytes_input;

    return p;
}


/******************************************************************************
*
* emx_print - Print the data in D to the file stream FP in a human-readable
*   format.  The OUTPUT_LEVEL can be set to 0, 1, or 2 for increasingly more
*   information.
*
******************************************************************************/

void emx_print (
    FILE *fp,
    const emx_data *d,
    const int output_level) {

    int i;


    assert (fp);

    if (!d) return;

    fprintf (fp, "\
====== DATAGRAM =======\n\
bytes_in_datagram     : %u Bytes\n\
start_identifier      : %u\n\
datagram_type         : %s\n\
em_model_number       : EM %u\n\
date                  : %u\n\
time_ms               : %u ms\n\
counter               : %u\n\
serial_number         : %u\n\
-----------------------\n", d->header.bytes_in_datagram, d->header.start_identifier,
        emx_get_datagram_name (d->header.datagram_type), d->header.em_model_number, d->header.date,
        d->header.time_ms, d->header.counter, d->header.serial_number);

    switch (d->header.datagram_type) {

        case EMX_DATAGRAM_DEPTH :

            fprintf (fp, "\
vessel_heading        : %0.1f deg\n\
sound_speed           : %0.1f m/s\n\
transducer_depth      : %0.2f m\n\
max_beams             : %u\n\
num_beams             : %u\n\
depth_resolution      : %u cm\n\
horizontal_resolution : %u cm\n\
sample_rate           : %u Hz (%0.2f m)\n\
depth_offset_mult     : %d\n", d->datagram.depth.info->vessel_heading * 0.01, d->datagram.depth.info->sound_speed * 0.1,
                d->datagram.depth.info->transducer_depth * 0.01, d->datagram.depth.info->max_beams, d->datagram.depth.info->num_beams,
                d->datagram.depth.info->depth_resolution, d->datagram.depth.info->horizontal_resolution, d->datagram.depth.info->sample_rate,
                750.0 / d->datagram.depth.info->sample_rate, d->datagram.depth.depth_offset_multiplier);

            if (output_level > 0) {

                for (i=0; i<d->datagram.depth.info->num_beams; i++) {

                    fprintf (fp, "\
--- Sounding [ %d ] ----\n\
depth                 : ", i);

                    if ((emx_get_model (d->header.em_model_number) == EM_MODEL_120) || (emx_get_model (d->header.em_model_number) == EM_MODEL_121A) ||
                        (emx_get_model (d->header.em_model_number) == EM_MODEL_300)) {
                        fprintf (fp, "%u\n", (uint16_t) d->datagram.depth.beam[i].depth);
                    } else {
                        fprintf (fp, "%d\n", d->datagram.depth.beam[i].depth);
                    }

                    fprintf (fp, "\
across_track          : %d\n\
along_track           : %d\n\
beam_depression_angle : %0.1f deg\n\
beam_azimuth_angle    : %0.1f deg\n\
range                 : %u\n\
quality_factor        : %u\n\
detect_window_length  : %u\n\
backscatter           : %0.1f dB\n\
beam_number           : %u\n", d->datagram.depth.beam[i].across_track, d->datagram.depth.beam[i].along_track,
                        d->datagram.depth.beam[i].beam_depression_angle / 100.0, d->datagram.depth.beam[i].beam_azimuth_angle / 100.0, d->datagram.depth.beam[i].range,
                        d->datagram.depth.beam[i].quality_factor, d->datagram.depth.beam[i].detect_window_length, d->datagram.depth.beam[i].backscatter * 0.5,
                        d->datagram.depth.beam[i].beam_number);
                }
            }

            break;

        case EMX_DATAGRAM_DEPTH_NOMINAL :

            fprintf (fp, "\
transducer_depth      : %0.2f m\n\
max_beams             : %u\n\
valid_beams           : %u\n", d->datagram.depth_nominal.info->transducer_depth, d->datagram.depth_nominal.info->max_beams,
                d->datagram.depth_nominal.info->num_beams);

            if (output_level > 0) {

                for (i=0; i<d->datagram.depth_nominal.info->num_beams; i++) {

                    fprintf (fp, "\
--- Sounding [ %d ] ----\n\
depth                 : %0.2f m\n\
across_track          : %0.2f m\n\
along_track           : %0.2f m\n\
detection_info        : %u\n\
system_cleaning       : %d\n", i, d->datagram.depth_nominal.beam[i].depth, d->datagram.depth_nominal.beam[i].across_track,
                        d->datagram.depth_nominal.beam[i].along_track, d->datagram.depth_nominal.beam[i].detection_info,
                        d->datagram.depth_nominal.beam[i].system_cleaning);
                }
            }

            break;

        case EMX_DATAGRAM_XYZ :

            fprintf (fp, "\
vessel_heading        : %0.1f deg\n\
sound_speed           : %0.1f m/s\n\
transducer_depth      : %0.2f m\n\
num_beams             : %u beams\n\
valid_beams           : %u beams\n\
sample_rate           : %0.2f Hz (%0.2f m)\n\
scanning_info         : %u\n", d->datagram.xyz.info->vessel_heading * 0.01, d->datagram.xyz.info->sound_speed * 0.1, d->datagram.xyz.info->transducer_depth,
                d->datagram.xyz.info->num_beams, d->datagram.xyz.info->valid_beams, d->datagram.xyz.info->sample_rate,
                750.0 / d->datagram.xyz.info->sample_rate, d->datagram.xyz.info->scanning_info);

            if (output_level > 0) {

                for (i=0; i<d->datagram.xyz.info->num_beams; i++) {

                    fprintf (fp, "\
--- Sounding [ %d ] ----\n\
depth                 : %0.2f m\n\
across_track          : %0.2f m\n\
along_track           : %0.2f m\n\
detect_window_length  : %u\n\
quality_factor        : %u\n\
beam_adjustment       : %0.1f deg\n\
detection_info        : %u\n\
system_cleaning       : %d\n\
backscatter           : %0.1f dB\n", i, d->datagram.xyz.beam[i].depth, d->datagram.xyz.beam[i].across_track, d->datagram.xyz.beam[i].along_track,
                        d->datagram.xyz.beam[i].detect_window_length, d->datagram.xyz.beam[i].quality_factor, d->datagram.xyz.beam[i].beam_adjustment * 0.1,
                        d->datagram.xyz.beam[i].detection_info, d->datagram.xyz.beam[i].system_cleaning, d->datagram.xyz.beam[i].backscatter * 0.1);
                }
            }

            break;

        case EMX_DATAGRAM_EXTRA_DETECTIONS :

            fprintf (fp, "\
datagram_counter      : %u\n\
datagram_version      : %u\n\
swath_counter         : %u\n\
swath_index           : %u\n\
vessel_heading        : %0.2f deg\n\
sound_speed           : %0.1f m\n\
reference_depth       : %0.2f m\n\
wc_sample_rate        : %0.2f Hz\n\
raw_amp_sample_rate   : %0.2f Hz\n\
rx_transducer_index   : %u\n\
num_detects           : %u\n\
num_classes           : %u\n\
nbytes_class          : %u Bytes\n\
nalarm_flags          : %u\n\
nbytes_detect         : %u Bytes\n", d->datagram.extra_detect.info->datagram_counter, d->datagram.extra_detect.info->datagram_version,
                d->datagram.extra_detect.info->swath_counter, d->datagram.extra_detect.info->swath_index, d->datagram.extra_detect.info->vessel_heading * 0.01,
                d->datagram.extra_detect.info->sound_speed * 0.1, d->datagram.extra_detect.info->reference_depth, d->datagram.extra_detect.info->wc_sample_rate,
                d->datagram.extra_detect.info->raw_amplitude_sample_rate, d->datagram.extra_detect.info->rx_transducer_index,
                d->datagram.extra_detect.info->num_detects, d->datagram.extra_detect.info->num_classes, d->datagram.extra_detect.info->nbytes_class,
                d->datagram.extra_detect.info->nalarm_flags, d->datagram.extra_detect.info->nbytes_detect);

            if (output_level > 0) {

                for (i=0; i<d->datagram.extra_detect.info->num_classes; i++) {

                    fprintf (fp, "\
----- Class [ %d ] -----\n\
start_depth           : %u %%\n\
stop_depth            : %u %%\n\
qf_threshold          : %u\n\
bs_threshold          : %d dB\n\
snr_threshold         : %u dB\n\
alarm_threshold       : %u\n\
num_detects           : %u\n\
show_class            : %u\n\
alarm_flag            : %u\n", i, d->datagram.extra_detect.classes[i].start_depth, d->datagram.extra_detect.classes[i].stop_depth,
                        d->datagram.extra_detect.classes[i].qf_threshold, d->datagram.extra_detect.classes[i].bs_threshold, d->datagram.extra_detect.classes[i].snr_threshold,
                        d->datagram.extra_detect.classes[i].alarm_threshold, d->datagram.extra_detect.classes[i].num_detects, d->datagram.extra_detect.classes[i].show_class,
                        d->datagram.extra_detect.classes[i].alarm_flag);
                }
            }

            if (output_level > 1) {

                for (i=0; i<d->datagram.extra_detect.info->num_detects; i++) {

                    fprintf (fp, "\
depth                 : %0.2f m\n\
across_track          : %0.2f m\n\
along_track           : %0.2f m\n\
latitude_delta        : %0.8f\n\
longitud_delta        : %0.8f\n\
beam_angle            : %0.2f deg\n\
angle_correction      : %0.2f\n\
travel_time           : %0.5f s\n\
travel_time_correction: %0.5f s\n\
backscatter           : %0.1f dB\n\
beam_adjustment       : %0.1f deg\n\
detection_info        : %d\n\
tx_sector             : %u\n\
detect_window_length  : %u\n\
quality_factor        : %u\n\
system_cleaning       : %u\n\
range_factor          : %u %%\n\
class_number          : %u\n\
confidence            : %u\n\
qf_ifremer            : %u\n\
wc_beam_number        : %u\n\
beam_angle_across     : %0.2f deg\n\
detected_range        : %u\n\
raw_amplitude         : %u\n", d->datagram.extra_detect.data[i].depth, d->datagram.extra_detect.data[i].across_track, d->datagram.extra_detect.data[i].along_track,
                        d->datagram.extra_detect.data[i].latitude_delta, d->datagram.extra_detect.data[i].longitud_delta, d->datagram.extra_detect.data[i].beam_angle,
                        d->datagram.extra_detect.data[i].angle_correction, d->datagram.extra_detect.data[i].travel_time, d->datagram.extra_detect.data[i].travel_time_correction,
                        d->datagram.extra_detect.data[i].backscatter / 10.0, d->datagram.extra_detect.data[i].beam_adjustment / 10.0, d->datagram.extra_detect.data[i].detection_info,
                        d->datagram.extra_detect.data[i].tx_sector, d->datagram.extra_detect.data[i].detection_window_length, d->datagram.extra_detect.data[i].quality_factor,
                        d->datagram.extra_detect.data[i].system_cleaning, d->datagram.extra_detect.data[i].range_factor, d->datagram.extra_detect.data[i].class_number,
                        d->datagram.extra_detect.data[i].confidence, d->datagram.extra_detect.data[i].qf_ifremer, d->datagram.extra_detect.data[i].wc_beam_number,
                        d->datagram.extra_detect.data[i].beam_angle_across, d->datagram.extra_detect.data[i].detected_range, d->datagram.extra_detect.data[i].raw_amplitude);
                }
            }

            /* TODO: Output raw amplitudes. */

            break;

        case EMX_DATAGRAM_CENTRAL_BEAMS :

            fprintf (fp, "\
mean_abs_coeff        : %0.2f dB/km\n\
pulse_length          : %u us\n\
range_norm            : %u\n\
start_range           : %u\n\
stop_range            : %u\n\
normal_incidence_bs   : %d dB\n\
oblique_bs            : %d dB\n\
tx_beamwidth          : %0.1f deg\n\
tvg_cross_over        : %0.1f deg\n\
num_beams             : %u\n", d->datagram.central_beams.info->mean_abs_coef *.01, d->datagram.central_beams.info->pulse_length,
                d->datagram.central_beams.info->range_norm, d->datagram.central_beams.info->start_range, d->datagram.central_beams.info->stop_range,
                d->datagram.central_beams.info->normal_incidence_bs, d->datagram.central_beams.info->oblique_bs, d->datagram.central_beams.info->tx_beamwidth * 0.1,
                d->datagram.central_beams.info->tvg_cross_over * 0.1, d->datagram.central_beams.info->num_beams);

            if (output_level > 0) {

                for (i=0; i<d->datagram.central_beams.info->num_beams; i++) {

                    fprintf (fp, "\
----- Beam [ %d ] ------\n\
beam_index            : %u\n\
num_samples           : %u\n\
start_range           : %u\n", i, d->datagram.central_beams.beam[i].beam_index, d->datagram.central_beams.beam[i].num_samples, d->datagram.central_beams.beam[i].start_range);
                }
            }

            break;

        case EMX_DATAGRAM_RRA_101 :

            fprintf (fp, "\
vessel_heading        : %0.2f deg\n\
sound_speed           : %0.1f m/s\n\
transducer_depth      : %0.2f m\n\
max_beams             : %u\n\
num_beams             : %u\n\
depth_resolution      : %0.2f m\n\
horizontal_resolution : %0.2f m\n\
sample_rate           : %u Hz\n\
status                : %d\n\
range_norm            : %u\n\
normal_incidence_bs   : %0.1f dB\n\
oblique_bs            : %0.1f dB\n\
fixed_gain            : %u\n\
tx_power              : %d\n\
mode                  : %u\n\
coverage              : %u\n\
yawstab_heading       : %u\n\
tx_sectors            : %u\n", d->datagram.rra_101.info->vessel_heading / 100.0, d->datagram.rra_101.info->sound_speed / 10.0, d->datagram.rra_101.info->transducer_depth / 100.0,
                d->datagram.rra_101.info->max_beams, d->datagram.rra_101.info->num_beams, d->datagram.rra_101.info->depth_resolution / 100.0, d->datagram.rra_101.info->horizontal_resolution / 100.0,
                d->datagram.rra_101.info->sample_rate, d->datagram.rra_101.info->status, d->datagram.rra_101.info->range_norm, d->datagram.rra_101.info->normal_incidence_bs / 10.0,
                d->datagram.rra_101.info->oblique_bs / 10.0, d->datagram.rra_101.info->fixed_gain, d->datagram.rra_101.info->tx_power, d->datagram.rra_101.info->mode,
                d->datagram.rra_101.info->coverage, d->datagram.rra_101.info->yawstab_heading, d->datagram.rra_101.info->tx_sectors);

            for (i=0; i<d->datagram.rra_101.info->tx_sectors; i++) {

                fprintf (fp, "\
---- Sector [ %d ] -----\n\
last_beam             : %u\n\
tx_tilt_angle         : %0.2f deg\n\
heading               : %0.1f deg\n\
roll                  : %0.1f deg\n\
pitch                 : %0.1f deg\n\
heave                 : %0.2f m\n", i, d->datagram.rra_101.tx_beam[i].last_beam, d->datagram.rra_101.tx_beam[i].tx_tilt_angle * 0.01,
                    d->datagram.rra_101.tx_beam[i].heading * 0.01, d->datagram.rra_101.tx_beam[i].roll * 0.01, d->datagram.rra_101.tx_beam[i].pitch * 0.01,
                    d->datagram.rra_101.tx_beam[i].heave * 0.01);
            }

            if (output_level > 0) {

                for (i=0; i<d->datagram.rra_101.info->num_beams; i++) {

                    fprintf (fp, "\
---- RX_Beam [ %d ] -----\n\
range                 : %u\n\
quality_factor        : %u\n\
detect_window_length  : %u\n\
backscatter           : %0.1f dB\n\
beam_number           : %u\n\
rx_beam_angle         : %0.2f deg\n\
rx_heading            : %0.2f deg\n\
roll                  : %0.2f deg\n\
pitch                 : %0.2f deg\n\
heave                 : %0.2f m\n", i, d->datagram.rra_101.rx_beam[i].range, d->datagram.rra_101.rx_beam[i].quality_factor,
                        d->datagram.rra_101.rx_beam[i].detect_window_length, d->datagram.rra_101.rx_beam[i].backscatter * 0.5,
                        d->datagram.rra_101.rx_beam[i].beam_number, d->datagram.rra_101.rx_beam[i].rx_beam_angle * 0.01,
                        d->datagram.rra_101.rx_beam[i].rx_heading * 0.01, d->datagram.rra_101.rx_beam[i].roll * 0.01,
                        d->datagram.rra_101.rx_beam[i].pitch * 0.01, d->datagram.rra_101.rx_beam[i].heave * 0.01);
                }
            }

            break;

        case EMX_DATAGRAM_RRA_70 :

            fprintf (fp, "\
max_beams             : %u\n\
num_beams             : %u\n\
sound_speed           : %0.1f m/s\n", d->datagram.rra_70.info->max_beams, d->datagram.rra_70.info->num_beams, d->datagram.rra_70.info->sound_speed / 10.0);

            if (output_level > 0) {

                for (i=0; i<d->datagram.rra_70.info->num_beams; i++) {

                    fprintf (fp, "\
----- Beam [ %d ] ------\n\
beam_angle            : %0.2f deg\n\
tx_tilt_angle         : %0.2f deg\n\
range                 : %u\n\
backscatter           : %0.1f dB\n\
beam_number           : %u\n", i, d->datagram.rra_70.beam[i].beam_angle * 0.01, d->datagram.rra_70.beam[i].tx_tilt_angle * 0.01,
                        d->datagram.rra_70.beam[i].range, d->datagram.rra_70.beam[i].backscatter * 0.1, d->datagram.rra_70.beam[i].beam_number);
                }
            }

            break;

        case EMX_DATAGRAM_RRA_102 :

            fprintf (fp, "\
tx_sectors            : %u\n\
num_beams             : %u\n\
sample_rate           : %0.2f Hz\n\
rov_depth             : %0.2f m\n\
sound_speed           : %0.1f m/s\n\
max_beams             : %u\n", d->datagram.rra_102.info->tx_sectors, d->datagram.rra_102.info->num_beams,
                d->datagram.rra_102.info->sample_rate * 0.01, d->datagram.rra_102.info->rov_depth * 0.01,
                d->datagram.rra_102.info->sound_speed * 0.1, d->datagram.rra_102.info->max_beams);

            for (i=0; i<d->datagram.rra_102.info->tx_sectors; i++) {

                fprintf (fp, "\
---- Sector [ %d ] -----\n\
tx_tilt_angle         : %0.2f deg\n\
focus_range           : %0.1f m\n\
signal_length         : %u ms\n\
tx_offset             : %u ms\n\
center_freq           : %u Hz\n\
signal_bandwidth      : %0.0f Hz\n\
signal_waveform_id    : %u\n\
tx_sector             : %u\n", i, d->datagram.rra_102.tx_beam[i].tx_tilt_angle * 0.01, d->datagram.rra_102.tx_beam[i].focus_range * 0.1,
                    d->datagram.rra_102.tx_beam[i].signal_length, d->datagram.rra_102.tx_beam[i].tx_offset, d->datagram.rra_102.tx_beam[i].center_freq,
                    d->datagram.rra_102.tx_beam[i].signal_bandwidth * 10.0, d->datagram.rra_102.tx_beam[i].signal_waveform_id, d->datagram.rra_102.tx_beam[i].tx_sector);
            }

            if (output_level > 0) {

                for (i=0; i<d->datagram.rra_102.info->num_beams; i++) {

                    fprintf (fp, "\
---- RX_Beam [ %d ] -----\n\
rx_beam_angle         : %0.2f deg\n\
range                 : %0.2f\n\
tx_sector_number      : %u\n\
backscatter           : %0.1f dB\n\
quality_factor        : %u\n\
detect_window_length  : %u\n\
beam_number           : %d\n", i, d->datagram.rra_102.rx_beam[i].rx_beam_angle * 0.01, d->datagram.rra_102.rx_beam[i].range * 0.25,
                        d->datagram.rra_102.rx_beam[i].tx_sector_number, d->datagram.rra_102.rx_beam[i].backscatter * 0.5,
                        d->datagram.rra_102.rx_beam[i].quality_factor, d->datagram.rra_102.rx_beam[i].detect_window_length,
                        d->datagram.rra_102.rx_beam[i].beam_number);
                }
            }

            break;

        case EMX_DATAGRAM_RRA_78 :

            fprintf (fp, "\
sound_speed           : %0.1f m/s\n\
tx_sectors            : %u\n\
num_beams             : %u\n\
valid_beams           : %u\n\
sample_rate           : %0.2f Hz\n\
dscale                : %u\n", d->datagram.rra_78.info->sound_speed * 0.1, d->datagram.rra_78.info->tx_sectors,
                d->datagram.rra_78.info->num_beams, d->datagram.rra_78.info->valid_beams,
                d->datagram.rra_78.info->sample_rate, d->datagram.rra_78.info->dscale);

            for (i=0; i<d->datagram.rra_78.info->tx_sectors; i++) {

                fprintf (fp, "\
---- Sector [ %d ] -----\n\
tx_tilt_angle         : %0.2f deg\n\
focus_range           : %0.1f m\n\
signal_length         : %0.6f s\n\
sector_tx_delay       : %0.6f s\n\
center_freq           : %0.1f Hz\n\
mean_absorption       : %0.2f dB/km\n\
signal_waveform_id    : %u\n\
transmit_sector       : %u\n\
signal_bandwidth      : %0.0f Hz\n", i, d->datagram.rra_78.tx_beam[i].tx_tilt_angle * 0.01, d->datagram.rra_78.tx_beam[i].focus_range * 0.1,
                    d->datagram.rra_78.tx_beam[i].signal_length, d->datagram.rra_78.tx_beam[i].sector_tx_delay, d->datagram.rra_78.tx_beam[i].center_freq,
                    d->datagram.rra_78.tx_beam[i].mean_absorption * 0.01, d->datagram.rra_78.tx_beam[i].signal_waveform_id, d->datagram.rra_78.tx_beam[i].tx_sector,
                    d->datagram.rra_78.tx_beam[i].signal_bandwidth);
            }

            if (output_level > 0) {

                for (i=0; i<d->datagram.rra_78.info->num_beams; i++) {

                    fprintf (fp, "\
----- Beam [ %d ] -----\n\
rx_beam_angle         : %0.2f deg\n\
tx_sector_number      : %u\n\
detection_info        : %u\n\
detect_window_length  : %u\n\
quality_factor        : %u\n\
dcorr                 : %d\n\
two_way_travel_time   : %0.5f s\n\
backscatter           : %0.1f dB\n\
system_cleaning       : %d\n", i, d->datagram.rra_78.rx_beam[i].rx_beam_angle / 100.0, d->datagram.rra_78.rx_beam[i].tx_sector_number, d->datagram.rra_78.rx_beam[i].detection_info,
                        d->datagram.rra_78.rx_beam[i].detect_window_length, d->datagram.rra_78.rx_beam[i].quality_factor, d->datagram.rra_78.rx_beam[i].doppler_correction,
                        d->datagram.rra_78.rx_beam[i].two_way_travel_time, d->datagram.rra_78.rx_beam[i].backscatter / 10.0, d->datagram.rra_78.rx_beam[i].system_cleaning);
                }
            }

            break;

        case EMX_DATAGRAM_SEABED_IMAGE_83 :

            fprintf (fp, "\
mean_abs_coeff        : %0.2f dB/km\n\
pulse_length          : %u us\n\
range_norm            : %u\n\
start_range           : %u\n\
stop_range            : %u\n\
normal_incidence_bs   : %0.1f dB\n\
oblique_bs            : %0.1f dB\n\
tx_beamwidth          : %0.1f deg\n\
tvg_cross_over        : %0.1f deg\n\
num_beams             : %u beams\n", d->datagram.seabed_83.info->mean_abs_coef * 0.01, d->datagram.seabed_83.info->pulse_length,
                d->datagram.seabed_83.info->range_norm, d->datagram.seabed_83.info->start_range, d->datagram.seabed_83.info->stop_range,
                d->datagram.seabed_83.info->normal_incidence_bs * 0.1, d->datagram.seabed_83.info->oblique_bs * 0.1,
                d->datagram.seabed_83.info->tx_beamwidth * 0.1, d->datagram.seabed_83.info->tvg_cross_over * 0.1, d->datagram.seabed_83.info->num_beams);

            if (output_level > 0) {

                int num_samples = 0;

                for (i=0; i<d->datagram.seabed_83.info->num_beams; i++) {

                    fprintf (fp, "\
----- Beam [ %d ] -----\n\
beam_index            : %u\n\
sorting_direction     : %d\n\
num_samples           : %u\n\
detect_sample         : %u\n", i, d->datagram.seabed_83.beam[i].beam_index, d->datagram.seabed_83.beam[i].sorting_direction,
                        d->datagram.seabed_83.beam[i].num_samples, d->datagram.seabed_83.beam[i].detect_sample);

                    num_samples += d->datagram.seabed_83.beam[i].num_samples;
                }

                fprintf (fp, "\
-----------------------\n\
bytes_end             : %d\n", d->datagram.seabed_83.bytes_end);

                if (output_level > 1) {
                    fputs ("\
amplitude             : ", fp);

                    for (i=0; i<num_samples; i++) {

                        fprintf (fp, "%0.1f ", d->datagram.seabed_83.amplitude[i] * 0.5);
                    }

                    fputc ('\n', fp);
                }
            }

            break;

        case EMX_DATAGRAM_SEABED_IMAGE_89 :

            fprintf (fp, "\
sample_rate           : %0.2f Hz (%0.2f m)\n\
range_norm            : %u samples\n\
normal_incidence_bs   : %0.1f dB\n\
oblique_bs            : %0.1f dB\n\
tx_beamwidth          : %0.1f deg\n\
tvg_cross_over        : %0.1f deg\n\
num_beams             : %u beams\n", d->datagram.seabed_89.info->sample_rate, 750.0 / d->datagram.seabed_89.info->sample_rate,
                d->datagram.seabed_89.info->range_norm, 0.1 * d->datagram.seabed_89.info->normal_incidence_bs, 0.1 * d->datagram.seabed_89.info->oblique_bs,
                0.1 * d->datagram.seabed_89.info->tx_beamwidth, 0.1 * d->datagram.seabed_89.info->tvg_cross_over, d->datagram.seabed_89.info->num_beams);

            if (output_level > 0) {

                int num_samples = 0;

                for (i=0; i<d->datagram.seabed_89.info->num_beams; i++) {

                    fprintf (fp, "\
----- Beam [ %d ] -----\n\
sorting_direction     : %d\n\
detection_info        : %u\n\
num_samples           : %u\n\
detect_sample         : %u\n", i, d->datagram.seabed_89.beam[i].sorting_direction, d->datagram.seabed_89.beam[i].detection_info,
                        d->datagram.seabed_89.beam[i].num_samples, d->datagram.seabed_89.beam[i].detect_sample);

                    num_samples += d->datagram.seabed_89.beam[i].num_samples;
                }

                fprintf (fp, "\
-----------------------\n\
bytes_end             : %d\n", d->datagram.seabed_89.bytes_end);

                if (output_level > 1) {
                    fputs ("\
amplitude             : ", fp);

                    for (i=0; i<num_samples; i++) {

                        fprintf (fp, "%0.1f ", d->datagram.seabed_89.amplitude[i] * 0.1);
                    }

                    fputc ('\n', fp);
                }
            }

            break;

        case EMX_DATAGRAM_WATER_COLUMN :

            fprintf (fp, "\
num_datagrams         : %u\n\
datagram_number       : %u\n\
tx_sectors            : %u\n\
num_beams             : %u\n\
datagram_beams        : %u\n\
sound_speed           : %0.1f m/s\n\
sample_rate           : %0.2f Hz (%0.2f m)\n\
tx_heave              : %d cm\n\
tvg_function          : %u\n\
tvg_offset            : %d dB\n\
scanning_info         : %u\n", d->datagram.wc.info->num_datagrams, d->datagram.wc.info->datagram_number, d->datagram.wc.info->tx_sectors,
                d->datagram.wc.info->num_beams, d->datagram.wc.info->datagram_beams, d->datagram.wc.info->sound_speed * 0.1, d->datagram.wc.info->sample_rate * 0.01,
                750.0 / (0.01 * d->datagram.wc.info->sample_rate), d->datagram.wc.info->tx_heave, d->datagram.wc.info->tvg_function,
                d->datagram.wc.info->tvg_offset, d->datagram.wc.info->scanning_info);

            for (i=0; i<d->datagram.wc.info->tx_sectors; i++) {
                fprintf (fp, "\
---- Sector [ %d ] -----\n\
tx_tilt_angle         : %0.2f deg\n\
center_freq           : %0.0f Hz\n\
tx_sector             : %u\n", i, d->datagram.wc.txbeam[i].tx_tilt_angle / 100.0, d->datagram.wc.txbeam[i].center_freq * 10.0, d->datagram.wc.txbeam[i].tx_sector);
            }

            if (output_level > 0) {

                emx_datagram_wc_rx_beam wc_rx_beam;
                const uint8_t *p = d->datagram.wc.beamData;

                for (i=0; i<d->datagram.wc.info->datagram_beams; i++) {

                    p = emx_get_wc_rxbeam (&wc_rx_beam, p);

                    fprintf (fp, "\
----- Beam [ %d ] ------\n\
beam_angle            : %0.2f deg\n\
start_range           : %u\n\
num_samples           : %u\n\
detected_range        : %u\n\
tx_sector             : %u\n\
beam_index            : %u\n", i, wc_rx_beam.info->beam_angle / 100.0, wc_rx_beam.info->start_range, wc_rx_beam.info->num_samples,
                        wc_rx_beam.info->detected_range, wc_rx_beam.info->tx_sector, wc_rx_beam.info->beam_index);

                    if (output_level > 1) {
                        int j;

                        fputs ("\
amplitude             : ", fp);

                        for (j=0; j<wc_rx_beam.info->num_samples; j++) {
                            fprintf (fp, "%0.1f ", wc_rx_beam.amplitude[j] * 0.5);
                        }

                        fputc ('\n', fp);
                    }
                }
            }

            break;

        case EMX_DATAGRAM_QUALITY_FACTOR :

            fprintf (fp, "\
num_beams             : %u\n\
npar                  : %u\n", d->datagram.qf.info->num_beams, d->datagram.qf.info->npar);

            if (output_level > 0) {

                fputs ("\
quality_factor        : ", fp);

                for (i=0; i<d->datagram.qf.info->num_beams; i++) {
                    fprintf (fp, "%0.1f ", d->datagram.qf.data[i]);
                }
                fputc ('\n', fp);
            }

            break;

        case EMX_DATAGRAM_ATTITUDE :

            fprintf (fp, "\
num_entries           : %u\n\
sensor_sys_descriptor : %d\n", d->datagram.attitude.info->num_entries, d->datagram.attitude.sensor_system_descriptor);

            if (output_level > 0) {

                for (i=0; i<d->datagram.attitude.info->num_entries; i++) {

                    fprintf (fp, "\
----- Entry [ %d ] -----\n\
record_time           : %0.3f s\n\
status                : %u\n\
roll                  : %0.2f deg\n\
pitch                 : %0.2f deg\n\
heave                 : %d cm\n\
heading               : %0.2f deg\n", i, d->datagram.attitude.data[i].record_time / 1000.0, d->datagram.attitude.data[i].status, d->datagram.attitude.data[i].roll * 0.01,
                        d->datagram.attitude.data[i].pitch * 0.01, d->datagram.attitude.data[i].heave, d->datagram.attitude.data[i].heading * 0.01);
                }
            }

            break;

        case EMX_DATAGRAM_ATTITUDE_NETWORK :

            fprintf (fp, "\
num_entries           : %u\n\
sensor_sys_descriptor : %d\n", d->datagram.attitude_network.info->num_entries, d->datagram.attitude_network.info->sensor_system_descriptor);

            if (output_level > 0) {

                emx_datagram_attitude_network_data attitude_data;
                const uint8_t *p = d->datagram.attitude_network.data;

                for (i=0; i<d->datagram.attitude_network.info->num_entries; i++) {

                    p = emx_get_attitude_network_data (&attitude_data, p);

                    fprintf (fp, "\
----- Entry [ %d ] -----\n\
record_time           : %0.3f s\n\
roll                  : %0.2f deg\n\
pitch                 : %0.2f deg\n\
heave                 : %0.2f m\n\
heading               : %0.2f deg\n\
num_bytes_input       : %u Bytes\n", i, attitude_data.info->record_time / 1000.0, attitude_data.info->roll * 0.01, attitude_data.info->pitch * 0.01, attitude_data.info->heave * 0.01,
                        attitude_data.info->heading * 0.01, attitude_data.info->num_bytes_input);

                    if (output_level > 1) {
                        int j;

                        fputs ("\
message               : ", fp);

                        for (j=0; j<attitude_data.info->num_bytes_input; j++) {
                            fprintf (fp, "%d ", attitude_data.message[j]);
                        }

                        fputc ('\n', fp);
                    }
                }
            }

            break;

        case EMX_DATAGRAM_CLOCK :

            fprintf (fp, "\
date                  : %u\n\
time_ms               : %u ms\n\
PPS                   : %u\n", d->datagram.clock.info->date, d->datagram.clock.info->time_ms, d->datagram.clock.info->PPS);

            break;

        case EMX_DATAGRAM_HEIGHT :

            fprintf (fp, "\
height                : %d cm\n\
height_type           : %u\n", d->datagram.height.info->height, d->datagram.height.info->height_type);

            break;

        case EMX_DATAGRAM_HEADING :

            fprintf (fp, "\
num_entries           : %u\n\
heading_indicator     : %u\n", d->datagram.heading.info->num_entries, d->datagram.heading.heading_indicator);

            if (output_level > 0) {
                for (i=0; i<d->datagram.heading.info->num_entries; i++) {

                    fprintf (fp, "\
----- Entry [ %d ] -----\n\
record_time           : %u ms\n\
heading               : %0.2f deg\n", i, d->datagram.heading.data[i].record_time, d->datagram.heading.data[i].heading * 0.01);
                }
            }

            break;

        case EMX_DATAGRAM_POSITION :

            if (d->datagram.position.info->latitude == EMX_NULL_INT32) {
                fputs ("latitude              : INVALID\n", fp);
            } else {
                fprintf (fp, "latitude              : %0.5f deg\n", d->datagram.position.info->latitude / 20000000.0);
            }

            if (d->datagram.position.info->longitude == EMX_NULL_INT32) {
                fputs ("longitude             : INVALID\n", fp);
            } else {
                fprintf (fp, "longitude             : %0.5f deg\n", d->datagram.position.info->longitude / 10000000.0);
            }

            if (d->datagram.position.info->position_fix_quality == EMX_NULL_UINT16) {
                fputs ("position_fix_quality  : INVALID\n", fp);
            } else {
                fprintf (fp, "position_fix_quality  : %0.2f m\n", d->datagram.position.info->position_fix_quality / 100.0);
            }

            if (d->datagram.position.info->vessel_speed == EMX_NULL_UINT16) {
                fputs ("vessel_speed          : INVALID\n", fp);
            } else {
                fprintf (fp, "vessel_speed          : %0.2f m/s\n", d->datagram.position.info->vessel_speed / 100.0);
            }

            if (d->datagram.position.info->vessel_course == EMX_NULL_UINT16) {
                fputs ("vessel_course         : INVALID\n", fp);
            } else {
                fprintf (fp, "vessel_course         : %0.2f deg\n", 0.01 * d->datagram.position.info->vessel_course);
            }

            if (d->datagram.position.info->vessel_heading == EMX_NULL_UINT16) {
                fputs ("vessel_heading        : INVALID\n", fp);
            } else {
                fprintf (fp, "vessel_heading        : %0.2f deg\n", 0.01 * d->datagram.position.info->vessel_heading);
            }

            fprintf (fp, "position_system       : %u\n", d->datagram.position.info->position_system);

            if ((d->datagram.position.info->position_system & 3) == 1) {
                fprintf (fp, "position_system       : Positioning System 1\n");
            }

            if ((d->datagram.position.info->position_system & 3) == 2) {
                fprintf (fp, "position_system       : Positioning System 2\n");
            }

            if ((d->datagram.position.info->position_system & 3) == 3) {
                fprintf (fp, "position_system       : Positioning System 3\n");
            }

            if ((d->datagram.position.info->position_system & 192) == 128) {
                fprintf (fp, "position_system       : Positioning System Active, System Time Used\n");
            }

            if ((d->datagram.position.info->position_system & 192) == 192) {
                fprintf (fp, "position_system       : Positioning System Active, Input Datagram Used\n");
            }

            fprintf (fp, "bytes_in_input        : %u Bytes\n", d->datagram.position.info->bytes_in_input);

            fputs ("\
message               : ", fp);

            /* TODO: Check if message is binary and print in hex. */
            for (i=0; i<d->datagram.position.info->bytes_in_input; i++) {
                if (d->datagram.position.message[i] == '\0') break;
                if ((d->datagram.position.message[i] != '\n') && (d->datagram.position.message[i] != '\r')) {
                    if (CPL_ISALNUM (d->datagram.position.message[i])) {
                        fprintf (fp, "%c", d->datagram.position.message[i]);
                    }
                }
            }
            fputc ('\n', fp);

            break;

        case EMX_DATAGRAM_SINGLE_BEAM_DEPTH :

            fprintf (fp, "\
date                  : %u\n\
time_ms               : %u ms\n\
depth                 : %0.2f m\n\
source                : %c\n", d->datagram.sb_depth.info->date, d->datagram.sb_depth.info->time_ms, d->datagram.sb_depth.info->depth * 0.01,
                d->datagram.sb_depth.info->source);

            break;

        case EMX_DATAGRAM_TIDE :

            fprintf (fp, "\
date                  : %u\n\
time_ms               : %u ms\n\
tide_offset           : %0.2f m\n", d->datagram.tide.info->date, d->datagram.tide.info->time_ms, d->datagram.tide.info->tide_offset * 0.01);

            break;

        case EMX_DATAGRAM_SSSV :

            fprintf (fp, "\
num_samples           : %u\n", d->datagram.sssv.info->num_samples);

            if (output_level > 0) {

                for (i=0; i<d->datagram.sssv.info->num_samples; i++) {

                    fprintf (fp, "\
----- Sample [ %d ] ----\n\
record_time           : %u s\n\
sound_speed           : %0.1f m/s\n", i, d->datagram.sssv.data[i].record_time, d->datagram.sssv.data[i].sound_speed / 10.0);
                }
            }

            break;

        case EMX_DATAGRAM_SVP :

            fprintf (fp, "\
date                  : %u\n\
time_ms               : %u ms\n\
num_samples           : %u\n\
depth_resolution      : %u cm\n", d->datagram.svp.info->date, d->datagram.svp.info->time_ms, d->datagram.svp.info->num_samples,
                d->datagram.svp.info->depth_resolution);

            if (output_level > 0) {

                for (i=0; i<d->datagram.svp.info->num_samples; i++) {

                    fprintf (fp, "\
----- Sample [ %d ] ----\n\
depth                 : %0.2f m\n\
sound_speed           : %0.1f m/s\n", i, d->datagram.svp.data[i].depth * 0.01, d->datagram.svp.data[i].sound_speed * 0.1);
                }
            }

            break;

        case EMX_DATAGRAM_SVP_EM3000 :

            fprintf (fp, "\
date                  : %u\n\
time_ms               : %u ms\n\
num_samples           : %u\n\
depth_resolution      : %u cm\n", d->datagram.svp_em3000.info->date, d->datagram.svp_em3000.info->time_ms, d->datagram.svp_em3000.info->num_samples,
                d->datagram.svp_em3000.info->depth_resolution);

            if (output_level > 0) {

                for (i=0; i<d->datagram.svp_em3000.info->num_samples; i++) {

                    fprintf (fp, "\
----- Sample [ %d ] ----\n\
depth                 : %0.2f m\n\
sound_speed           : %0.1f m/s\n", i, d->datagram.svp_em3000.data[i].depth * 0.01, d->datagram.svp_em3000.data[i].sound_speed * 0.1);
                }
            }

            break;

        case EMX_DATAGRAM_KM_SSP_OUTPUT :

            fprintf (fp, "\
num_bytes             : %d\n", d->datagram.ssp_output.num_bytes);

            fputs ("\
data                  : ", fp);

            for (i=0; i<d->datagram.ssp_output.num_bytes; i++) {
                if (d->datagram.ssp_output.data[i] == '\0') break;
                fputc (d->datagram.ssp_output.data[i], fp);
            }
            fputc ('\n', fp);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS :

            fprintf (fp, "\
serial_number2        : %u\n\
text_length           : %d\n", d->datagram.install_params.info->serial_number2, d->datagram.install_params.text_length);

            fputs ("\
text                  : ", fp);

            for (i=0; i<d->datagram.install_params.text_length; i++) {
                if (d->datagram.install_params.text[i] == '\0') break;
                fputc (d->datagram.install_params.text[i], fp);
            }
            fputc ('\n', fp);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS_STOP :

            fprintf (fp, "\
serial_number2        : %u\n\
text_length           : %d\n", d->datagram.install_params_stop.info->serial_number2, d->datagram.install_params_stop.text_length);

            fputs ("\
text                  : ", fp);

            for (i=0; i<d->datagram.install_params_stop.text_length; i++) {
                if (d->datagram.install_params_stop.text[i] == '\0') break;
                fputc (d->datagram.install_params_stop.text[i], fp);
            }
            fputc ('\n', fp);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS_REMOTE :

            fprintf (fp, "\
serial_number2        : %u\n\
text_length           : %d\n", d->datagram.install_params_remote.info->serial_number2, d->datagram.install_params_remote.text_length);

            fputs ("\
text                  : ", fp);

            for (i=0; i<d->datagram.install_params_remote.text_length; i++) {
                if (d->datagram.install_params_remote.text[i] == '\0') break;
                fputc (d->datagram.install_params_remote.text[i], fp);
            }
            fputc ('\n', fp);

            break;

        case EMX_DATAGRAM_REMOTE_PARAMS_INFO :

            /* TODO: */

            break;

        case EMX_DATAGRAM_RUNTIME_PARAMS :

            fprintf (fp, "\
operator_station_stat : %u\n\
pu_status             : %u\n\
bsp_status            : %u\n\
head_or_tx_status     : %u\n\
mode                  : %u\n\
filter_id             : %u\n\
min_depth             : %u m\n\
max_depth             : %u m\n\
absorption            : %0.2f dB/km\n\
tx_pulse_length       : %u us\n", d->datagram.runtime_params.info->operator_station_status, d->datagram.runtime_params.info->pu_status,
                d->datagram.runtime_params.info->bsp_status, d->datagram.runtime_params.info->head_or_tx_status, d->datagram.runtime_params.info->mode,
                d->datagram.runtime_params.info->filter_id, d->datagram.runtime_params.info->min_depth, d->datagram.runtime_params.info->max_depth,
                d->datagram.runtime_params.info->absorption / 100.0, d->datagram.runtime_params.info->tx_pulse_length);

            fputs ("\
tx_beamwidth          : ", fp);

            if (d->datagram.runtime_params.info->tx_beamwidth == EMX_NULL_UINT16) {
                fputs ("INVALID\n", fp);
            }

            fprintf (fp, "%0.1f deg\n", d->datagram.runtime_params.info->tx_beamwidth / 10.0);

            fprintf (fp, "\
tx_power              : %d dB\n\
rx_beamwidth          : %0.1f deg\n\
rx_bandwidth          : %0.0f Hz\n\
rx_fixed_gain         : %u dB\n\
tvg_crossover         : %u deg\n\
sound_speed_source    : %u\n\
max_port_swath        : %u m\n", d->datagram.runtime_params.info->tx_power, d->datagram.runtime_params.info->rx_beamwidth / 10.0,
                d->datagram.runtime_params.info->rx_bandwidth * 50.0, d->datagram.runtime_params.info->rx_fixed_gain,
                d->datagram.runtime_params.info->tvg_crossover, d->datagram.runtime_params.info->sound_speed_source,
                d->datagram.runtime_params.info->max_port_swath);

            fputs ("\
beam_spacing          : ", fp);

            if ((d->datagram.runtime_params.info->beam_spacing & 128) == 0) {
                switch (d->datagram.runtime_params.info->beam_spacing) {
                    case 0 : fputs ("Determined by beamwidth\n", fp); break;
                    case 1 : fputs ("Equidistant\n", fp); break;
                    case 2 : fputs ("Equiangle\n", fp); break;
                    case 3 : fputs ("High density equidistant\n", fp); break;
                    default : fprintf (fp, "%u\n", d->datagram.runtime_params.info->beam_spacing); break;
                }
            } else {
                /* TODO: dual head. */
            }

            fprintf (fp, "\
max_port_coverage     : %u deg\n\
yaw_pitch_mode        : %u\n\
max_stbd_coverage     : %u deg\n\
max_stbd_swath        : %u m\n\
tx_along_tilt         : %d\n\
filter_id2            : %u\n", d->datagram.runtime_params.info->max_port_coverage, d->datagram.runtime_params.info->yaw_pitch_mode,
                d->datagram.runtime_params.info->max_stbd_coverage, d->datagram.runtime_params.info->max_stbd_swath,
                d->datagram.runtime_params.info->tx_along_tilt, d->datagram.runtime_params.info->filter_id2);

            break;

        case EMX_DATAGRAM_EXTRA_PARAMS :

            fputs ("\
content               : ", fp);

            switch (d->datagram.extra_params.info->content) {
                case 1 : fputs ("calib.txt\n", fp); break;
                case 2 : fputs ("Log all heights\n", fp); break;
                case 3 : fputs ("Sound velocity at transducer\n", fp); break;
                case 4 : fputs ("Sound velocity profile\n", fp); break;
                case 5 : fputs ("Multicast RX status\n", fp); break;
                case 6 : fputs ("bscorr.txt\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.extra_params.info->content); break;
            }

            switch (d->datagram.extra_params.info->content) {

                /* TODO: */

                case 6 :
                    fprintf (fp, "\
num_chars             : %u\n\
text                  : \n\
%s\n", d->datagram.extra_params.data.bs_corr.num_chars, d->datagram.extra_params.data.bs_corr.text);
                    break;

                default : break;
            }

            break;

        case EMX_DATAGRAM_PU_OUTPUT :

            fprintf (fp, "\
udp_port1             : %u\n\
udp_port2             : %u\n\
udp_port3             : %u\n\
udp_port4             : %u\n\
system_descriptor     : %u\n\
pu_software_version   : %s\n\
bsp_software_version  : %s\n\
transceiver1_version  : %s\n\
transceiver2_version  : %s\n\
host_ip_address       : %u\n\
tx_opening_angle      : %u\n\
rx_opening_angle      : %u\n", d->datagram.pu_output.info->udp_port1, d->datagram.pu_output.info->udp_port2,
                d->datagram.pu_output.info->udp_port3, d->datagram.pu_output.info->udp_port4, d->datagram.pu_output.info->system_descriptor,
                d->datagram.pu_output.info->pu_software_version, d->datagram.pu_output.info->bsp_software_version,
                d->datagram.pu_output.info->transceiver1_version, d->datagram.pu_output.info->transceiver2_version,
                d->datagram.pu_output.info->host_ip_address, d->datagram.pu_output.info->tx_opening_angle,
                d->datagram.pu_output.info->rx_opening_angle);

            break;

        case EMX_DATAGRAM_PU_STATUS :

            fprintf (fp, "\
ping_rate             : %0.2f Hz\n\
ping_counter          : %u\n\
swath_distance        : %u\n\
status_udp_port_2     : %u\n\
status_serial_port_1  : %u\n\
status_serial_port_2  : %u\n\
status_serial_port_3  : %u\n\
status_serial_port_4  : %u\n\
pps_status            : %d\n\
position_status       : %d\n\
attitude_status       : %d\n\
clock_status          : %d\n\
heading_status        : %d\n\
pu_status             : %u\n\
heading               : %0.2f deg\n\
roll                  : %0.2f deg\n\
pitch                 : %0.2f deg\n\
heave                 : %0.2f m\n\
sound_speed           : %0.1f m/s\n\
depth                 : %0.2f m\n\
velocity              : %0.2f m/s\n\
velocity_status       : %u\n\
ramp                  : %u\n\
bs_oblique            : %d dB\n\
bs_normal             : %d dB\n\
gain                  : %d dB\n\
depth_normal          : %u m\n\
range_normal          : %u m\n\
port_coverage         : %u deg\n\
stbd_coverage         : %u deg\n\
sound_speed_svp       : %0.1f m/s\n\
yaw_stabilization     : %d\n\
port_coverage2        : %d\n\
stbd_coverage2        : %d\n\
cpu_temp              : %d deg C\n", d->datagram.pu_status.info->ping_rate * 0.01, d->datagram.pu_status.info->ping_counter, d->datagram.pu_status.info->swath_distance,
                d->datagram.pu_status.info->status_udp_port_2, d->datagram.pu_status.info->status_serial_port_1, d->datagram.pu_status.info->status_serial_port_2,
                d->datagram.pu_status.info->status_serial_port_3, d->datagram.pu_status.info->status_serial_port_4, d->datagram.pu_status.info->pps_status,
                d->datagram.pu_status.info->position_status, d->datagram.pu_status.info->attitude_status, d->datagram.pu_status.info->clock_status,
                d->datagram.pu_status.info->heading_status, d->datagram.pu_status.info->pu_status, d->datagram.pu_status.info->heading * 0.01,
                d->datagram.pu_status.info->roll * 0.01, d->datagram.pu_status.info->pitch * 0.01, d->datagram.pu_status.info->heave * 0.01,
                d->datagram.pu_status.info->sound_speed * 0.1, d->datagram.pu_status.info->depth * 0.01, d->datagram.pu_status.info->velocity * 0.01,
                d->datagram.pu_status.info->velocity_status, d->datagram.pu_status.info->ramp, d->datagram.pu_status.info->bs_oblique, d->datagram.pu_status.info->bs_normal,
                d->datagram.pu_status.info->gain, d->datagram.pu_status.info->depth_normal, d->datagram.pu_status.info->range_normal, d->datagram.pu_status.info->port_coverage,
                d->datagram.pu_status.info->stbd_coverage, d->datagram.pu_status.info->sound_speed_svp * 0.1, d->datagram.pu_status.info->yaw_stabilization,
                d->datagram.pu_status.info->port_coverage2, d->datagram.pu_status.info->stbd_coverage2, d->datagram.pu_status.info->cpu_temp);

            break;

        case EMX_DATAGRAM_PU_BIST_RESULT :

            fprintf (fp, "\
test_number           : %u\n\
test_result_status    : %d\n\
text_length           : %d\n", d->datagram.pu_bist_result.info->test_number, d->datagram.pu_bist_result.info->test_result_status,
                d->datagram.pu_bist_result.text_length);

            fputs ("\
text                  : ", fp);

            for (i=0; i<d->datagram.pu_bist_result.text_length; i++) {
                if (d->datagram.pu_bist_result.text[i] == '\0') break;
                fputc (d->datagram.pu_bist_result.text[i], fp);
            }
            fputc ('\n', fp);

            break;

        case EMX_DATAGRAM_TRANSDUCER_TILT :

            fprintf (fp, "\
num_entries           : %u\n", d->datagram.tilt.info->num_entries);

            if (output_level > 0) {

                for (i=0; i<d->datagram.tilt.info->num_entries; i++) {

                    fprintf (fp, "\
----- Sample [ %d ] ----\n\
record_time           : %u\n\
tilt                  : %0.2f deg\n", i, d->datagram.tilt.data[i].record_time, d->datagram.tilt.data[i].tilt * 0.01);
                }
            }

            break;

        case EMX_DATAGRAM_HISAS_STATUS :

            fprintf (fp, "\
version_id            : %u\n\
ping_rate             : %0.2f Hz\n\
ping_counter          : %u\n\
pu_idle_count         : %u\n\
sensor_input_status   : %u\n\
pps_status            : %d\n\
clock_status          : %d\n\
attitude_status       : %d\n\
trigger_status        : %u\n\
pu_modes              : %u\n\
logger_status         : %u\n\
yaw                   : %0.2f deg\n\
roll                  : %0.2f deg\n\
pitch                 : %0.2f deg\n\
heave                 : %0.3f m\n\
sound_speed           : %0.2f m/s\n\
log_file_size_port    : %u Bytes\n\
log_file_size_stbd    : %u Bytes\n\
free_space_port       : %u MBytes\n\
free_space_stbd       : %u MBytes\n\
cbmf_1_status         : %u\n\
cbmf_2_status         : %u\n\
tru_board_status      : %u\n\
pu_status             : %u\n\
cpu_temp              : %d deg C\n\
lptx_temp             : %d deg C\n\
lprx_temp             : %d deg C\n\
hdd_temp              : %d deg C\n\
last_nav_depth_input  : %d\n\
last_nav_alt_input    : %d\n\
transmitters_passive  : %u\n\
ext_trigger_enabled   : %u\n\
sidescan_bathy_enabled: %u\n\
insas_enabled         : %u\n", d->datagram.hisas_status.info->version_id, d->datagram.hisas_status.info->ping_rate * 0.01,
                d->datagram.hisas_status.info->ping_counter, d->datagram.hisas_status.info->pu_idle_count,
                d->datagram.hisas_status.info->sensor_input_status, d->datagram.hisas_status.info->pps_status,
                d->datagram.hisas_status.info->clock_status, d->datagram.hisas_status.info->attitude_status,
                d->datagram.hisas_status.info->trigger_status, d->datagram.hisas_status.info->pu_modes,
                d->datagram.hisas_status.info->logger_status, d->datagram.hisas_status.info->yaw * 0.01,
                d->datagram.hisas_status.info->roll * 0.01, d->datagram.hisas_status.info->pitch * 0.01, d->datagram.hisas_status.info->heave * 0.01,
                d->datagram.hisas_status.info->sound_speed * 0.1, d->datagram.hisas_status.info->log_file_size_port,
                d->datagram.hisas_status.info->log_file_size_stbd, d->datagram.hisas_status.info->free_space_port,
                d->datagram.hisas_status.info->free_space_stbd, d->datagram.hisas_status.info->cbmf_1_status,
                d->datagram.hisas_status.info->cbmf_2_status, d->datagram.hisas_status.info->tru_board_status,
                d->datagram.hisas_status.info->pu_status, d->datagram.hisas_status.info->cpu_temp,
                d->datagram.hisas_status.info->lptx_temp, d->datagram.hisas_status.info->lprx_temp,
                d->datagram.hisas_status.info->hdd_temp, d->datagram.hisas_status.info->last_nav_depth_input,
                d->datagram.hisas_status.info->last_nav_altitude_input, d->datagram.hisas_status.info->transmitters_passive,
                d->datagram.hisas_status.info->external_trigger_enabled, d->datagram.hisas_status.info->sidescan_bathy_enabled,
                d->datagram.hisas_status.info->in_mission_sas_enabled);

            break;

        case EMX_DATAGRAM_SIDESCAN_STATUS :

            fprintf (fp, "\
file_format           : %u\n\
system_type           : %u\n\
sonar_type            : %u\n\
nav_units             : %u\n\
num_channels          : %u\n", d->datagram.sidescan_status.info->file_format, d->datagram.sidescan_status.info->system_type,
                d->datagram.sidescan_status.info->sonar_type, d->datagram.sidescan_status.info->nav_units,
                d->datagram.sidescan_status.info->num_channels);

            for (i=0; i<d->datagram.sidescan_status.info->num_channels; i++) {

                fprintf (fp, "\
----- Channel %d -------\n\
type_of_channel       : %u\n\
sub_channel_number    : %u\n\
correction_flags      : %u\n\
uni_polar             : %u\n\
bytes_per_sample      : %u\n\
channel_name          : %s\n\
frequency             : %0.1f Hz\n\
horiz_beam_angle      : %0.1f deg\n\
tilt_angle            : %0.1f deg\n\
beam_width            : %0.1f deg\n\
offset_x              : %0.3f m\n\
offset_y              : %0.3f m\n\
offset_z              : %0.3f m\n\
offset_yaw            : %0.1f deg\n\
offset_pitch          : %0.1f deg\n\
offset_roll           : %0.1f deg\n", i, d->datagram.sidescan_status.info->channel[i].type_of_channel,
                    d->datagram.sidescan_status.info->channel[i].sub_channel_number, d->datagram.sidescan_status.info->channel[i].correction_flags,
                    d->datagram.sidescan_status.info->channel[i].uni_polar, d->datagram.sidescan_status.info->channel[i].bytes_per_sample,
                    d->datagram.sidescan_status.info->channel[i].channel_name, d->datagram.sidescan_status.info->channel[i].frequency,
                    d->datagram.sidescan_status.info->channel[i].horiz_beam_angle, d->datagram.sidescan_status.info->channel[i].tilt_angle,
                    d->datagram.sidescan_status.info->channel[i].beam_width, d->datagram.sidescan_status.info->channel[i].offset_x,
                    d->datagram.sidescan_status.info->channel[i].offset_y, d->datagram.sidescan_status.info->channel[i].offset_z,
                    d->datagram.sidescan_status.info->channel[i].offset_yaw, d->datagram.sidescan_status.info->channel[i].offset_pitch,
                    d->datagram.sidescan_status.info->channel[i].offset_roll);
            }

            break;

        case EMX_DATAGRAM_HISAS_1032_SIDESCAN :

            fprintf (fp, "\
magic_number          : 0x%x\n\
header_type           : %u\n\
beam_number           : %u\n\
num_channels          : %u\n\
num_bytes_record      : %u\n\
year                  : %u\n\
month                 : %u\n\
day                   : %u\n\
hour                  : %u\n\
minute                : %u\n\
second                : %u\n\
hseconds              : %u\n\
ping_number           : %u\n\
sound_velocity_two_way: %0.1f m/s\n\
sound_velocity        : %0.1f m/s\n\
fix_time_hour         : %u\n\
fix_time_minute       : %u\n\
fix_time_second       : %u\n\
fix_time_hsecond      : %u\n\
sensor_speed          : %0.1f kts\n\
sensor_lat            : %0.8f deg\n\
sensor_lon            : %0.8f deg\n\
sensor_depth          : %0.2f m\n\
sensor_altitude       : %0.2f m\n\
sensor_aux_altitude   : %0.2f m\n\
sensor_pitch          : %0.1f deg\n\
sensor_roll           : %0.1f deg\n\
sensor_heading        : %0.1f deg\n", d->datagram.sidescan_data.info->magic_number, d->datagram.sidescan_data.info->header_type,
                d->datagram.sidescan_data.info->beam_number, d->datagram.sidescan_data.info->num_channels,
                d->datagram.sidescan_data.info->num_bytes_record, d->datagram.sidescan_data.info->year,
                d->datagram.sidescan_data.info->month, d->datagram.sidescan_data.info->day, d->datagram.sidescan_data.info->hour,
                d->datagram.sidescan_data.info->minute, d->datagram.sidescan_data.info->second, d->datagram.sidescan_data.info->hseconds,
                d->datagram.sidescan_data.info->ping_number, d->datagram.sidescan_data.info->sound_velocity_two_way,
                d->datagram.sidescan_data.info->sound_velocity, d->datagram.sidescan_data.info->fix_time_hour,
                d->datagram.sidescan_data.info->fix_time_minute, d->datagram.sidescan_data.info->fix_time_second,
                d->datagram.sidescan_data.info->fix_time_hsecond, d->datagram.sidescan_data.info->sensor_speed,
                d->datagram.sidescan_data.info->sensor_lat, d->datagram.sidescan_data.info->sensor_lon,
                d->datagram.sidescan_data.info->sensor_depth, d->datagram.sidescan_data.info->sensor_altitude,
                d->datagram.sidescan_data.info->sensor_aux_altitude, d->datagram.sidescan_data.info->sensor_pitch,
                d->datagram.sidescan_data.info->sensor_roll, d->datagram.sidescan_data.info->sensor_heading);

            for (i=0; i<d->datagram.sidescan_data.info->num_channels; i++) {

                fprintf (fp, "\
----- Channel %d -------\n\
channel_number        : %u\n\
slant_range           : %0.1f m\n\
time_duration         : %0.4f s\n\
seconds_per_ping      : %0.4f s\n\
num_samples           : %u\n\
weight                : %d\n", i, d->datagram.sidescan_data.channel[i].info->channel_number, d->datagram.sidescan_data.channel[i].info->slant_range,
                    d->datagram.sidescan_data.channel[i].info->time_duration, d->datagram.sidescan_data.channel[i].info->seconds_per_ping,
                    d->datagram.sidescan_data.channel[i].info->num_samples, d->datagram.sidescan_data.channel[i].info->weight);

                if (output_level > 0) {

                    int j;

                    fputs ("data                  : ", fp);

                    if (d->datagram.sidescan_data.channel[i].bytes_per_sample == 2) {
                        for (j=0; j<d->datagram.sidescan_data.channel[i].info->num_samples; j++) {
                            fprintf (fp, "%u ", d->datagram.sidescan_data.channel[i].data.u16[j]);
                        }
                    } else  {
                        for (j=0; j<d->datagram.sidescan_data.channel[i].info->num_samples; j++) {
                            fprintf (fp, "%0.1f ", d->datagram.sidescan_data.channel[i].data.f32[j]);
                        }
                    }

                    fputc ('\n', fp);
                }
            }

            break;

        case EMX_DATAGRAM_NAVIGATION_OUTPUT :

            fprintf (fp, "\
data_type             : %u\n\
bytes_per_element     : %u\n\
num_elements          : %u\n\
date                  : %u\n\
time_ms               : %u\n\
time_offset           : %u\n\
latitude              : %0.8f deg\n\
longitude             : %0.8f deg\n\
depth                 : %0.2f m\n\
heading               : %0.1f deg\n\
pitch                 : %0.1f deg\n\
roll                  : %0.1f deg\n\
velocity_forward      : %0.1f m/s\n\
velocity_stbd         : %0.1f m/s\n\
velocity_down         : %0.1f m/s\n\
horizontal_uncertainty: %0.2f m\n\
altitude              : %0.2f m\n\
sound_speed           : %0.1f m/s\n\
data_validity         : %u\n", d->datagram.navigation_output.info->data_type, d->datagram.navigation_output.info->bytes_per_element,
                d->datagram.navigation_output.info->num_elements, d->datagram.navigation_output.info->date,
                d->datagram.navigation_output.info->time_ms, d->datagram.navigation_output.info->time_offset,
                d->datagram.navigation_output.info->latitude, d->datagram.navigation_output.info->longitude,
                d->datagram.navigation_output.info->depth, d->datagram.navigation_output.info->heading,
                d->datagram.navigation_output.info->pitch, d->datagram.navigation_output.info->roll,
                d->datagram.navigation_output.info->velocity_forward, d->datagram.navigation_output.info->velocity_stbd,
                d->datagram.navigation_output.info->velocity_down, d->datagram.navigation_output.info->horizontal_uncertainty,
                d->datagram.navigation_output.info->altitude, d->datagram.navigation_output.info->sound_speed,
                d->datagram.navigation_output.info->data_validity);

            break;

        default : break;
    }
}


/******************************************************************************
*
* emx_set_ignore_wc - Set the boolean to ignore (or skip) watercolumn data
*   while reading data from the file handle H.
*
******************************************************************************/

void emx_set_ignore_wc (
    emx_handle *h,
    const int ignore_wc) {

    assert (h);
    h->ignore_wc = ignore_wc;
}


/******************************************************************************
*
* emx_set_ignore_checksum - Set the boolean to not verify the datagram checksum.
*
******************************************************************************/

void emx_set_ignore_checksum (
    emx_handle *h,
    const int ignore_checksum) {

    assert (h);
    h->ignore_checksum = ignore_checksum;
}


/******************************************************************************
*
* emx_get_errno - Return the error number from the last call.
*
******************************************************************************/

int emx_get_errno (
    const emx_handle *h) {

    assert (h);
    return h->emx_errno;
}


/******************************************************************************
*
* emx_set_debug - Set the debugging output level for the EMX reader.
*
******************************************************************************/

void emx_set_debug (
    const int debug) {

    emx_debug = debug;
}


/******************************************************************************
*
* emx_identify - Determine if the file given by FILE_NAME is a EMX file.
*
* Return: 1 if the file is a EMX file,
*         0 if the file is not a EMX file, or
*         error condition if an error occurred.
*
* Errors: CS_EOPEN
*         CS_EREAD
*
******************************************************************************/

int emx_identify (
    const char *file_name) {

    emx_datagram_header header;
    int swap = -1;
    int status;
    int fd;


    if (!file_name) return 0;

    /* Open the binary file for read-only access or return on failure. */
    fd = cpl_open (file_name, CPL_OPEN_RDONLY | CPL_OPEN_BINARY | CPL_OPEN_SEQUENTIAL);

    if (fd < 0) {
        cpl_debug (emx_debug, "Open failed for file '%s'\n", file_name);
        return CS_EOPEN;
    }

    /* TODO: Read several datagrams instead of just one. */

    /* Read the first datagram header and validate it, then close the file. */
    status = emx_get_header (&header, &swap, fd);

    cpl_close (fd);

    if (status > 0) return 1;
    if (status == CS_EREAD) return status;

    return 0;
}


/******************************************************************************
*
* emx_get_header - Read the datagram header from the open file descriptor FD at
*   the current file position to the object HEADER.  If the value of SWAP is
*   positive, then the header will be byte swapped.  If SWAP is zero, then no
*   byte swapping is done.  If SWAP is negative, the byte swapping will be
*   determined from the data and SWAP will be set accordingly.  The data in the
*   header will be validated.
*
* Return: 0 if the file is at EOF,
*        >0 if the datagram header is read and is valid, or
*        <0 if an error occurred reading the header or the header is invalid.
*
* Errors: CS_EREAD
*         CS_EBADDATA
*
******************************************************************************/

static int emx_get_header (
    emx_datagram_header *header,
    int *swap,
    const int fd) {

    size_t actual_read_size;
    size_t read_size;
    ssize_t result;


    assert (swap);
    assert (header);
    assert (fd != -1);

    /* Set the read size to the datagram header size. */
    read_size = sizeof (emx_datagram_header);

    /* Read the datagram header from the open file descriptor FD. */
    result = cpl_read (header, read_size, fd);
    if (result < 0) {
        cpl_debug (emx_debug, "Read error occurred\n");
        return CS_EREAD;
    }

    actual_read_size = (size_t) result;

    /* If the read amount is zero, then it is EOF, otherwise if the read
       amount is less than the datagram header size, then the file is corrupt. */
    if (actual_read_size != read_size) {
        if (actual_read_size != 0) {
            cpl_debug (emx_debug, "Unexpected end of file\n");
            return CS_EBADDATA;
        }
        return 0;
    }

    /* Check the date and model number fields to determine if the data is stored as big or little-endian.
       Set the swap value to 1 if byte swapping is needed.  This test is only done if the swap value is negative. */
    if (*swap < 0) {
        *swap = emx_byte_order (header->date, header->em_model_number);
        if (*swap < 0) {
            cpl_debug (emx_debug, "Invalid date or model number (%u, %u)\n", header->date, header->em_model_number);
            return CS_EBADDATA;
        }
        cpl_debug (emx_debug, "Byte swap set to %s\n", *swap == 0 ? "FALSE" : "TRUE");
    }

    /* Byte swap the header if necessary. */
    if (*swap) emx_swap_header (header);

    /* Validate the header and return the result. */
    if (emx_valid_header (header)) return 1;

    return CS_EBADDATA;
}


/******************************************************************************
*
* emx_byte_order - Determine the byte order given the date value in DATE and the
*   system model number MODEL from the datagram header.
*
* Return: 0 if the date value is valid and requires no byte swapping,
*         1 if the date value is valid and does require byte swapping, or
*        -1 if the date value is invalid.
*
******************************************************************************/

static int emx_byte_order (
    const uint32_t date,
    const uint16_t model) {

    /* We determine byte order by byte swapping the date field and testing if the
       result is a valid date.  Some data has been found with the date in the first
       datagram set to zero, so for this case we test the model number field instead. */

    if ((date != 0) && (date != 20001025) && (date != 20790529)) {

        /* This test should be sufficient for all dates between the years 1970 to 2100
           with the exception of 20001025 and 20790529.  When these dates are byte swapped
           they equal themselves.  For these cases, we test the model number instead. */

        if (emx_valid_date (date)) {
            return 0;
        }

        if (emx_valid_date (CPL_BSWAP (date))) {
            return 1;
        }

    } else {

        if (emx_get_model (model)) {
            return 0;
        }

        if (emx_get_model (CPL_BSWAP (model))) {
            return 1;
        }
    }

    return -1;
}


/******************************************************************************
*
* emx_valid_date - Return non-zero if the date is in the valid range.
*
******************************************************************************/

static int emx_valid_date (
    const uint32_t date) {

    uint32_t year, month, day;


    /* Make sure the date is within a reasonable time period, but be generous. */
    if (date < 19700000 || date > 21000000) return 0;

    /* Parse year, month, and day from date field. */
    year = date / 10000;
    month = (date / 100) % 100;
    day = date % 100;

    return cpl_valid_date (year, month, day, 0, 0, 0.0);
}


/******************************************************************************
*
* emx_valid_header - Return non-zero if the datagram header HEADER is valid.
*
* Return: 1 if the header is valid,
*         0 otherwise.
*
******************************************************************************/

static int emx_valid_header (
    const emx_datagram_header *header) {

    assert (header);

    /* Verify start byte of datagram. */
    if (header->start_identifier != EMX_START_BYTE) {
        cpl_debug (emx_debug, "Invalid start byte (%u)\n", header->start_identifier);
        return 0;
    }

    /* Verify datagram size is not too small. */
    if (header->bytes_in_datagram < 16) {
        cpl_debug (emx_debug, "Invalid datagram size (%u)\n", header->bytes_in_datagram);
        return 0;
    }

    /* Verify the datagram size is not unreasonably large, but be generous. */
    if (header->bytes_in_datagram > 1<<27) {
        cpl_debug (emx_debug, "Invalid datagram size (%u)\n", header->bytes_in_datagram);
        return 0;
    }

    /* Have found an unknown datagram with invalid millisecond field. */
    if (header->datagram_type != EMX_DATAGRAM_UNKNOWN2) {

        /* Verify milliseconds field. */
        if (header->time_ms > 86399999) {
            cpl_debug (emx_debug, "Invalid millisecond field (%u)\n", header->time_ms);
            return 0;
        }

        /* Verify data field, but only if non-zero. */
        if (header->date != 0) {
            if (emx_valid_date (header->date) == 0) {
                cpl_debug (emx_debug, "Invalid date (%u)\n", header->date);
                return 0;
            }
        }
    }

    return 1;
}


/******************************************************************************
*
* emx_swap_header - Byte swap the datagram header.
*
******************************************************************************/

static void emx_swap_header (
    emx_datagram_header *header) {

    assert (header);

    header->bytes_in_datagram = CPL_BSWAP (header->bytes_in_datagram);
    header->em_model_number = CPL_BSWAP (header->em_model_number);
    header->date = CPL_BSWAP (header->date);
    header->time_ms = CPL_BSWAP (header->time_ms);
    header->counter = CPL_BSWAP (header->counter);
    header->serial_number = CPL_BSWAP (header->serial_number);
}


/******************************************************************************
*
* emx_swap_datagram - Byte swap the datagram D.
*
******************************************************************************/

static void emx_swap_datagram (
    emx_datagram *d,
    const int datagram_type) {

    size_t num_samples;
    size_t i;


    assert (d);

    switch (datagram_type) {

        case EMX_DATAGRAM_DEPTH :

            d->depth.info->vessel_heading = CPL_BSWAP (d->depth.info->vessel_heading);
            d->depth.info->sound_speed = CPL_BSWAP (d->depth.info->sound_speed);
            d->depth.info->transducer_depth = CPL_BSWAP (d->depth.info->transducer_depth);
            d->depth.info->sample_rate = CPL_BSWAP (d->depth.info->sample_rate);

            for (i=0; i<d->depth.info->num_beams; i++) {
                d->depth.beam[i].depth = CPL_BSWAP (d->depth.beam[i].depth);
                d->depth.beam[i].across_track = CPL_BSWAP (d->depth.beam[i].across_track);
                d->depth.beam[i].along_track = CPL_BSWAP (d->depth.beam[i].along_track);
                d->depth.beam[i].beam_depression_angle = CPL_BSWAP (d->depth.beam[i].beam_depression_angle);
                d->depth.beam[i].beam_azimuth_angle = CPL_BSWAP (d->depth.beam[i].beam_azimuth_angle);
                d->depth.beam[i].range = CPL_BSWAP (d->depth.beam[i].range);
            }

            break;

        case EMX_DATAGRAM_DEPTH_NOMINAL :

            d->depth_nominal.info->transducer_depth = CPL_BSWAPF (d->depth_nominal.info->transducer_depth);
            d->depth_nominal.info->max_beams = CPL_BSWAP (d->depth_nominal.info->max_beams);
            d->depth_nominal.info->num_beams = CPL_BSWAP (d->depth_nominal.info->num_beams);

            for (i=0; i<d->depth_nominal.info->num_beams; i++) {
                d->depth_nominal.beam[i].depth = CPL_BSWAPF (d->depth_nominal.beam[i].depth);
                d->depth_nominal.beam[i].across_track = CPL_BSWAPF (d->depth_nominal.beam[i].across_track);
                d->depth_nominal.beam[i].along_track = CPL_BSWAPF (d->depth_nominal.beam[i].along_track);
            }

            break;

        case EMX_DATAGRAM_XYZ :

            d->xyz.info->vessel_heading = CPL_BSWAP (d->xyz.info->vessel_heading);
            d->xyz.info->sound_speed = CPL_BSWAP (d->xyz.info->sound_speed);
            d->xyz.info->transducer_depth = CPL_BSWAPF (d->xyz.info->transducer_depth);
            d->xyz.info->num_beams = CPL_BSWAP (d->xyz.info->num_beams);
            d->xyz.info->valid_beams = CPL_BSWAP (d->xyz.info->valid_beams);
            d->xyz.info->sample_rate = CPL_BSWAPF (d->xyz.info->sample_rate);

            for (i=0; i<d->xyz.info->num_beams; i++) {
                d->xyz.beam[i].depth = CPL_BSWAPF (d->xyz.beam[i].depth);
                d->xyz.beam[i].across_track = CPL_BSWAPF (d->xyz.beam[i].across_track);
                d->xyz.beam[i].along_track = CPL_BSWAPF (d->xyz.beam[i].along_track);
                d->xyz.beam[i].detect_window_length = CPL_BSWAP (d->xyz.beam[i].detect_window_length);
                d->xyz.beam[i].backscatter = CPL_BSWAP (d->xyz.beam[i].backscatter);
            }

            break;

        case EMX_DATAGRAM_EXTRA_DETECTIONS :

            d->extra_detect.info->datagram_counter = CPL_BSWAP (d->extra_detect.info->datagram_counter);
            d->extra_detect.info->datagram_version = CPL_BSWAP (d->extra_detect.info->datagram_version);
            d->extra_detect.info->swath_counter = CPL_BSWAP (d->extra_detect.info->swath_counter);
            d->extra_detect.info->swath_index = CPL_BSWAP (d->extra_detect.info->swath_index);
            d->extra_detect.info->vessel_heading = CPL_BSWAP (d->extra_detect.info->vessel_heading);
            d->extra_detect.info->sound_speed = CPL_BSWAP (d->extra_detect.info->sound_speed);
            d->extra_detect.info->reference_depth = CPL_BSWAPF (d->extra_detect.info->reference_depth);
            d->extra_detect.info->wc_sample_rate = CPL_BSWAPF (d->extra_detect.info->wc_sample_rate);
            d->extra_detect.info->raw_amplitude_sample_rate = CPL_BSWAPF (d->extra_detect.info->raw_amplitude_sample_rate);
            d->extra_detect.info->num_detects = CPL_BSWAP (d->extra_detect.info->num_detects);
            d->extra_detect.info->num_classes = CPL_BSWAP (d->extra_detect.info->num_classes);
            d->extra_detect.info->nbytes_class = CPL_BSWAP (d->extra_detect.info->nbytes_class);
            d->extra_detect.info->nalarm_flags = CPL_BSWAP (d->extra_detect.info->nalarm_flags);
            d->extra_detect.info->nbytes_detect = CPL_BSWAP (d->extra_detect.info->nbytes_detect);

            for (i=0; i<d->extra_detect.info->num_classes; i++) {
                d->extra_detect.classes[i].start_depth = CPL_BSWAP (d->extra_detect.classes[i].start_depth);
                d->extra_detect.classes[i].stop_depth = CPL_BSWAP (d->extra_detect.classes[i].stop_depth);
                d->extra_detect.classes[i].qf_threshold = CPL_BSWAP (d->extra_detect.classes[i].qf_threshold);
                d->extra_detect.classes[i].bs_threshold = CPL_BSWAP (d->extra_detect.classes[i].bs_threshold);
                d->extra_detect.classes[i].snr_threshold = CPL_BSWAP (d->extra_detect.classes[i].snr_threshold);
                d->extra_detect.classes[i].alarm_threshold = CPL_BSWAP (d->extra_detect.classes[i].alarm_threshold);
                d->extra_detect.classes[i].num_detects = CPL_BSWAP (d->extra_detect.classes[i].num_detects);
            }

            for (i=0; i<d->extra_detect.info->num_detects; i++) {
                d->extra_detect.data[i].depth = CPL_BSWAPF (d->extra_detect.data[i].depth);
                d->extra_detect.data[i].across_track = CPL_BSWAPF (d->extra_detect.data[i].across_track);
                d->extra_detect.data[i].along_track = CPL_BSWAPF (d->extra_detect.data[i].along_track);
                d->extra_detect.data[i].latitude_delta = CPL_BSWAPF (d->extra_detect.data[i].latitude_delta);
                d->extra_detect.data[i].longitud_delta = CPL_BSWAPF (d->extra_detect.data[i].longitud_delta);
                d->extra_detect.data[i].beam_angle = CPL_BSWAPF (d->extra_detect.data[i].beam_angle);
                d->extra_detect.data[i].angle_correction = CPL_BSWAPF (d->extra_detect.data[i].angle_correction);
                d->extra_detect.data[i].travel_time = CPL_BSWAPF (d->extra_detect.data[i].travel_time);
                d->extra_detect.data[i].travel_time_correction = CPL_BSWAPF (d->extra_detect.data[i].travel_time_correction);
                d->extra_detect.data[i].backscatter = CPL_BSWAP (d->extra_detect.data[i].backscatter);
                d->extra_detect.data[i].tx_sector = CPL_BSWAP (d->extra_detect.data[i].tx_sector);
                d->extra_detect.data[i].detection_window_length = CPL_BSWAP (d->extra_detect.data[i].detection_window_length);
                d->extra_detect.data[i].quality_factor = CPL_BSWAP (d->extra_detect.data[i].quality_factor);
                d->extra_detect.data[i].system_cleaning = CPL_BSWAP (d->extra_detect.data[i].system_cleaning);
                d->extra_detect.data[i].range_factor = CPL_BSWAP (d->extra_detect.data[i].range_factor);
                d->extra_detect.data[i].class_number = CPL_BSWAP (d->extra_detect.data[i].class_number);
                d->extra_detect.data[i].confidence = CPL_BSWAP (d->extra_detect.data[i].confidence);
                d->extra_detect.data[i].qf_ifremer = CPL_BSWAP (d->extra_detect.data[i].qf_ifremer);
                d->extra_detect.data[i].wc_beam_number = CPL_BSWAP (d->extra_detect.data[i].wc_beam_number);
                d->extra_detect.data[i].beam_angle_across = CPL_BSWAPF (d->extra_detect.data[i].beam_angle_across);
                d->extra_detect.data[i].detected_range = CPL_BSWAP (d->extra_detect.data[i].detected_range);
                d->extra_detect.data[i].raw_amplitude = CPL_BSWAP (d->extra_detect.data[i].raw_amplitude);
            }

            /* TODO: raw_amplitudes */

            break;

        case EMX_DATAGRAM_CENTRAL_BEAMS :

            d->central_beams.info->mean_abs_coef = CPL_BSWAP (d->central_beams.info->mean_abs_coef);
            d->central_beams.info->pulse_length = CPL_BSWAP (d->central_beams.info->pulse_length);
            d->central_beams.info->range_norm = CPL_BSWAP (d->central_beams.info->range_norm);
            d->central_beams.info->start_range = CPL_BSWAP (d->central_beams.info->start_range);
            d->central_beams.info->stop_range = CPL_BSWAP (d->central_beams.info->stop_range);
            d->central_beams.info->tx_beamwidth = CPL_BSWAP (d->central_beams.info->tx_beamwidth);

            for (i=0; i<d->central_beams.info->num_beams; i++) {
                d->central_beams.beam[i].num_samples = CPL_BSWAP (d->central_beams.beam[i].num_samples);
                d->central_beams.beam[i].start_range = CPL_BSWAP (d->central_beams.beam[i].start_range);
            }

            break;

        case EMX_DATAGRAM_RRA_101 :

            d->rra_101.info->vessel_heading = CPL_BSWAP (d->rra_101.info->vessel_heading);
            d->rra_101.info->sound_speed = CPL_BSWAP (d->rra_101.info->sound_speed);
            d->rra_101.info->transducer_depth = CPL_BSWAP (d->rra_101.info->transducer_depth);
            d->rra_101.info->sample_rate = CPL_BSWAP (d->rra_101.info->sample_rate);
            d->rra_101.info->status = CPL_BSWAP (d->rra_101.info->status);
            d->rra_101.info->range_norm = CPL_BSWAP (d->rra_101.info->range_norm);
            d->rra_101.info->yawstab_heading = CPL_BSWAP (d->rra_101.info->yawstab_heading);
            d->rra_101.info->tx_sectors = CPL_BSWAP (d->rra_101.info->tx_sectors);


            for (i=0; i<d->rra_101.info->tx_sectors; i++) {
                d->rra_101.tx_beam[i].last_beam = CPL_BSWAP (d->rra_101.tx_beam[i].last_beam);
                d->rra_101.tx_beam[i].tx_tilt_angle = CPL_BSWAP (d->rra_101.tx_beam[i].tx_tilt_angle);
                d->rra_101.tx_beam[i].heading = CPL_BSWAP (d->rra_101.tx_beam[i].heading);
                d->rra_101.tx_beam[i].roll = CPL_BSWAP (d->rra_101.tx_beam[i].roll);
                d->rra_101.tx_beam[i].pitch = CPL_BSWAP (d->rra_101.tx_beam[i].pitch);
                d->rra_101.tx_beam[i].heave = CPL_BSWAP (d->rra_101.tx_beam[i].heave);
            }

            for (i=0; i<d->rra_101.info->num_beams; i++) {
                d->rra_101.rx_beam[i].range = CPL_BSWAP (d->rra_101.rx_beam[i].range);
                d->rra_101.rx_beam[i].rx_beam_angle = CPL_BSWAP (d->rra_101.rx_beam[i].rx_beam_angle);
                d->rra_101.rx_beam[i].rx_heading = CPL_BSWAP (d->rra_101.rx_beam[i].rx_heading);
                d->rra_101.rx_beam[i].roll = CPL_BSWAP (d->rra_101.rx_beam[i].roll);
                d->rra_101.rx_beam[i].pitch = CPL_BSWAP (d->rra_101.rx_beam[i].pitch);
                d->rra_101.rx_beam[i].heave = CPL_BSWAP (d->rra_101.rx_beam[i].heave);
            }

            break;

        case EMX_DATAGRAM_RRA_70 :

            d->rra_70.info->sound_speed = CPL_BSWAP (d->rra_70.info->sound_speed);

            for (i=0; i<d->rra_70.info->num_beams; i++) {
                d->rra_70.beam[i].beam_angle = CPL_BSWAP (d->rra_70.beam[i].beam_angle);
                d->rra_70.beam[i].tx_tilt_angle = CPL_BSWAP (d->rra_70.beam[i].tx_tilt_angle);
                d->rra_70.beam[i].range = CPL_BSWAP (d->rra_70.beam[i].range);
            }

            break;

        case EMX_DATAGRAM_RRA_102 :

            d->rra_102.info->tx_sectors = CPL_BSWAP (d->rra_102.info->tx_sectors);
            d->rra_102.info->num_beams = CPL_BSWAP (d->rra_102.info->num_beams);
            d->rra_102.info->sample_rate = CPL_BSWAP (d->rra_102.info->sample_rate);
            d->rra_102.info->rov_depth = CPL_BSWAP (d->rra_102.info->rov_depth);
            d->rra_102.info->sound_speed = CPL_BSWAP (d->rra_102.info->sound_speed);
            d->rra_102.info->max_beams = CPL_BSWAP (d->rra_102.info->max_beams);

            for (i=0; i<d->rra_102.info->tx_sectors; i++) {
                d->rra_102.tx_beam[i].tx_tilt_angle = CPL_BSWAP (d->rra_102.tx_beam[i].tx_tilt_angle);
                d->rra_102.tx_beam[i].focus_range = CPL_BSWAP (d->rra_102.tx_beam[i].focus_range);
                d->rra_102.tx_beam[i].signal_length = CPL_BSWAP (d->rra_102.tx_beam[i].signal_length);
                d->rra_102.tx_beam[i].tx_offset = CPL_BSWAP (d->rra_102.tx_beam[i].tx_offset);
                d->rra_102.tx_beam[i].center_freq = CPL_BSWAP (d->rra_102.tx_beam[i].center_freq);
                d->rra_102.tx_beam[i].signal_bandwidth = CPL_BSWAP (d->rra_102.tx_beam[i].signal_bandwidth);
            }

            for (i=0; i<d->rra_102.info->num_beams; i++) {
                d->rra_102.rx_beam[i].rx_beam_angle = CPL_BSWAP (d->rra_102.rx_beam[i].rx_beam_angle);
                d->rra_102.rx_beam[i].range = CPL_BSWAP (d->rra_102.rx_beam[i].range);
                d->rra_102.rx_beam[i].beam_number = CPL_BSWAP (d->rra_102.rx_beam[i].beam_number);
            }

            break;

        case EMX_DATAGRAM_RRA_78 :

            d->rra_78.info->sound_speed = CPL_BSWAP (d->rra_78.info->sound_speed);
            d->rra_78.info->tx_sectors = CPL_BSWAP (d->rra_78.info->tx_sectors);
            d->rra_78.info->num_beams = CPL_BSWAP (d->rra_78.info->num_beams);
            d->rra_78.info->valid_beams = CPL_BSWAP (d->rra_78.info->valid_beams);
            d->rra_78.info->sample_rate = CPL_BSWAPF (d->rra_78.info->sample_rate);
            d->rra_78.info->dscale = CPL_BSWAP (d->rra_78.info->dscale);

            for (i=0; i<d->rra_78.info->tx_sectors; i++) {
                d->rra_78.tx_beam[i].tx_tilt_angle = CPL_BSWAP (d->rra_78.tx_beam[i].tx_tilt_angle);
                d->rra_78.tx_beam[i].focus_range = CPL_BSWAP (d->rra_78.tx_beam[i].focus_range);
                d->rra_78.tx_beam[i].signal_length = CPL_BSWAPF (d->rra_78.tx_beam[i].signal_length);
                d->rra_78.tx_beam[i].sector_tx_delay = CPL_BSWAPF (d->rra_78.tx_beam[i].sector_tx_delay);
                d->rra_78.tx_beam[i].center_freq = CPL_BSWAPF (d->rra_78.tx_beam[i].center_freq);
                d->rra_78.tx_beam[i].mean_absorption = CPL_BSWAP (d->rra_78.tx_beam[i].mean_absorption);
                d->rra_78.tx_beam[i].signal_bandwidth = CPL_BSWAPF (d->rra_78.tx_beam[i].signal_bandwidth);
            }

            for (i=0; i<d->rra_78.info->num_beams; i++) {
                d->rra_78.rx_beam[i].rx_beam_angle = CPL_BSWAP (d->rra_78.rx_beam[i].rx_beam_angle);
                d->rra_78.rx_beam[i].detect_window_length = CPL_BSWAP (d->rra_78.rx_beam[i].detect_window_length);
                d->rra_78.rx_beam[i].two_way_travel_time = CPL_BSWAPF (d->rra_78.rx_beam[i].two_way_travel_time);
                d->rra_78.rx_beam[i].backscatter = CPL_BSWAP (d->rra_78.rx_beam[i].backscatter);
            }

            break;

        case EMX_DATAGRAM_SEABED_IMAGE_83 :

            d->seabed_83.info->mean_abs_coef = CPL_BSWAP (d->seabed_83.info->mean_abs_coef);
            d->seabed_83.info->pulse_length = CPL_BSWAP (d->seabed_83.info->pulse_length);
            d->seabed_83.info->range_norm = CPL_BSWAP (d->seabed_83.info->range_norm);
            d->seabed_83.info->start_range = CPL_BSWAP (d->seabed_83.info->start_range);
            d->seabed_83.info->stop_range = CPL_BSWAP (d->seabed_83.info->stop_range);
            d->seabed_83.info->tx_beamwidth = CPL_BSWAP (d->seabed_83.info->tx_beamwidth);

            for (i=0; i<d->seabed_83.info->num_beams; i++) {
                d->seabed_83.beam[i].num_samples = CPL_BSWAP (d->seabed_83.beam[i].num_samples);
                d->seabed_83.beam[i].detect_sample = CPL_BSWAP (d->seabed_83.beam[i].detect_sample);
            }

            break;

        case EMX_DATAGRAM_SEABED_IMAGE_89 :

            d->seabed_89.info->sample_rate = CPL_BSWAPF (d->seabed_89.info->sample_rate);
            d->seabed_89.info->range_norm = CPL_BSWAP (d->seabed_89.info->range_norm);
            d->seabed_89.info->normal_incidence_bs = CPL_BSWAP (d->seabed_89.info->normal_incidence_bs);
            d->seabed_89.info->oblique_bs = CPL_BSWAP (d->seabed_89.info->oblique_bs);
            d->seabed_89.info->tx_beamwidth = CPL_BSWAP (d->seabed_89.info->tx_beamwidth);
            d->seabed_89.info->tvg_cross_over = CPL_BSWAP (d->seabed_89.info->tvg_cross_over);
            d->seabed_89.info->num_beams = CPL_BSWAP (d->seabed_89.info->num_beams);

            num_samples = 0;
            for (i=0; i<d->seabed_89.info->num_beams; i++) {
                d->seabed_89.beam[i].num_samples = CPL_BSWAP (d->seabed_89.beam[i].num_samples);
                d->seabed_89.beam[i].detect_sample = CPL_BSWAP (d->seabed_89.beam[i].detect_sample);
                num_samples += d->seabed_89.beam[i].num_samples;
            }

            for (i=0; i<num_samples; i++) {
                d->seabed_89.amplitude[i] = CPL_BSWAP (d->seabed_89.amplitude[i]);
            }

            break;

        case EMX_DATAGRAM_WATER_COLUMN :

            d->wc.info->num_datagrams = CPL_BSWAP (d->wc.info->num_datagrams);
            d->wc.info->datagram_number = CPL_BSWAP (d->wc.info->datagram_number);
            d->wc.info->tx_sectors = CPL_BSWAP (d->wc.info->tx_sectors);
            d->wc.info->num_beams = CPL_BSWAP (d->wc.info->num_beams);
            d->wc.info->datagram_beams = CPL_BSWAP (d->wc.info->datagram_beams);
            d->wc.info->sound_speed = CPL_BSWAP (d->wc.info->sound_speed);
            d->wc.info->sample_rate = CPL_BSWAP (d->wc.info->sample_rate);
            d->wc.info->tx_heave = CPL_BSWAP (d->wc.info->tx_heave);

            for (i=0; i<d->wc.info->tx_sectors; i++) {
                d->wc.txbeam[i].tx_tilt_angle = CPL_BSWAP (d->wc.txbeam[i].tx_tilt_angle);
                d->wc.txbeam[i].center_freq = CPL_BSWAP (d->wc.txbeam[i].center_freq);
            }

            {
                emx_datagram_wc_rx_beam wc_rx_beam;
                const uint8_t *p = d->wc.beamData;

                for (i=0; i<d->wc.info->datagram_beams; i++) {

                    emx_datagram_wc_rx_beam_info *wc_rx_info = (emx_datagram_wc_rx_beam_info *) p;

                    /* We arrange the code this way to swap num_samples before calling emx_get_wc_rxbeam. */
                    wc_rx_info->beam_angle = CPL_BSWAP (wc_rx_info->beam_angle);
                    wc_rx_info->detected_range = CPL_BSWAP (wc_rx_info->detected_range);
                    wc_rx_info->num_samples = CPL_BSWAP (wc_rx_info->num_samples);
                    wc_rx_info->start_range = CPL_BSWAP (wc_rx_info->start_range);

                    p = emx_get_wc_rxbeam (&wc_rx_beam, p);
                }
            }

            break;

        case EMX_DATAGRAM_QUALITY_FACTOR :

            d->qf.info->num_beams = CPL_BSWAP (d->qf.info->num_beams);

            for (i=0; i<d->qf.info->num_beams; i++) {
                d->qf.data[i] = CPL_BSWAPF (d->qf.data[i]);
            }

            break;

        case EMX_DATAGRAM_ATTITUDE :

            d->attitude.info->num_entries = CPL_BSWAP (d->attitude.info->num_entries);

            for (i=0; i<d->attitude.info->num_entries; i++) {
                d->attitude.data[i].record_time = CPL_BSWAP (d->attitude.data[i].record_time);
                d->attitude.data[i].status = CPL_BSWAP (d->attitude.data[i].status);
                d->attitude.data[i].roll = CPL_BSWAP (d->attitude.data[i].roll);
                d->attitude.data[i].pitch = CPL_BSWAP (d->attitude.data[i].pitch);
                d->attitude.data[i].heave = CPL_BSWAP (d->attitude.data[i].heave);
                d->attitude.data[i].heading = CPL_BSWAP (d->attitude.data[i].heading);
            }

            break;

        case EMX_DATAGRAM_ATTITUDE_NETWORK :

            d->attitude_network.info->num_entries = CPL_BSWAP (d->attitude_network.info->num_entries);

            {
                emx_datagram_attitude_network_data attitude_data;
                const uint8_t *p = d->attitude_network.data;

                for (i=0; i<d->attitude_network.info->num_entries; i++) {

                    p = emx_get_attitude_network_data (&attitude_data, p);

                    attitude_data.info->record_time = CPL_BSWAP (attitude_data.info->record_time);
                    attitude_data.info->roll = CPL_BSWAP (attitude_data.info->roll);
                    attitude_data.info->pitch = CPL_BSWAP (attitude_data.info->pitch);
                    attitude_data.info->heave = CPL_BSWAP (attitude_data.info->heave);
                    attitude_data.info->heading = CPL_BSWAP (attitude_data.info->heading);
                }
            }

            break;

        case EMX_DATAGRAM_CLOCK :

            d->clock.info->date = CPL_BSWAP (d->clock.info->date);
            d->clock.info->time_ms = CPL_BSWAP (d->clock.info->time_ms);

            break;

        case EMX_DATAGRAM_HEIGHT :

            d->height.info->height = CPL_BSWAP (d->height.info->height);

            break;

        case EMX_DATAGRAM_HEADING :

            d->heading.info->num_entries = CPL_BSWAP (d->heading.info->num_entries);

            for (i=0; i<d->heading.info->num_entries; i++) {
                d->heading.data[i].record_time = CPL_BSWAP (d->heading.data[i].record_time);
                d->heading.data[i].heading = CPL_BSWAP (d->heading.data[i].heading);
            }

            break;

        case EMX_DATAGRAM_POSITION :

            d->position.info->latitude = CPL_BSWAP (d->position.info->latitude);
            d->position.info->longitude = CPL_BSWAP (d->position.info->longitude);
            d->position.info->position_fix_quality = CPL_BSWAP (d->position.info->position_fix_quality);
            d->position.info->vessel_speed = CPL_BSWAP (d->position.info->vessel_speed);
            d->position.info->vessel_course = CPL_BSWAP (d->position.info->vessel_course);
            d->position.info->vessel_heading = CPL_BSWAP (d->position.info->vessel_heading);

            break;

        case EMX_DATAGRAM_SINGLE_BEAM_DEPTH :

            d->sb_depth.info->date = CPL_BSWAP (d->sb_depth.info->date);
            d->sb_depth.info->time_ms = CPL_BSWAP (d->sb_depth.info->time_ms);
            d->sb_depth.info->depth = CPL_BSWAP (d->sb_depth.info->depth);

            break;

        case EMX_DATAGRAM_TIDE :

            d->tide.info->date = CPL_BSWAP (d->tide.info->date);
            d->tide.info->time_ms = CPL_BSWAP (d->tide.info->time_ms);
            d->tide.info->tide_offset = CPL_BSWAP (d->tide.info->tide_offset);

            break;

        case EMX_DATAGRAM_SSSV :

            d->sssv.info->num_samples = CPL_BSWAP (d->sssv.info->num_samples);

            for (i=0; i<d->sssv.info->num_samples; i++) {
                d->sssv.data[i].record_time = CPL_BSWAP (d->sssv.data[i].record_time);
                d->sssv.data[i].sound_speed = CPL_BSWAP (d->sssv.data[i].sound_speed);
            }

            break;

        case EMX_DATAGRAM_SVP :

            d->svp.info->date = CPL_BSWAP (d->svp.info->date);
            d->svp.info->time_ms = CPL_BSWAP (d->svp.info->time_ms);
            d->svp.info->num_samples = CPL_BSWAP (d->svp.info->num_samples);
            d->svp.info->depth_resolution = CPL_BSWAP (d->svp.info->depth_resolution);

            for (i=0; i<d->svp.info->num_samples; i++) {
                d->svp.data[i].depth = CPL_BSWAP (d->svp.data[i].depth);
                d->svp.data[i].sound_speed = CPL_BSWAP (d->svp.data[i].sound_speed);
            }

            break;

        case EMX_DATAGRAM_SVP_EM3000 :

            d->svp_em3000.info->date = CPL_BSWAP (d->svp_em3000.info->date);
            d->svp_em3000.info->time_ms = CPL_BSWAP (d->svp_em3000.info->time_ms);
            d->svp_em3000.info->num_samples = CPL_BSWAP (d->svp_em3000.info->num_samples);
            d->svp_em3000.info->depth_resolution = CPL_BSWAP (d->svp_em3000.info->depth_resolution);

            for (i=0; i<d->svp_em3000.info->num_samples; i++) {
                d->svp_em3000.data[i].depth = CPL_BSWAP (d->svp_em3000.data[i].depth);
                d->svp_em3000.data[i].sound_speed = CPL_BSWAP (d->svp_em3000.data[i].sound_speed);
            }

            break;

        case EMX_DATAGRAM_KM_SSP_OUTPUT :

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS :

            d->install_params.info->serial_number2 = CPL_BSWAP (d->install_params.info->serial_number2);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS_STOP :

            d->install_params_stop.info->serial_number2 = CPL_BSWAP (d->install_params_stop.info->serial_number2);

            break;

        case EMX_DATAGRAM_INSTALL_PARAMS_REMOTE :

            d->install_params_remote.info->serial_number2 = CPL_BSWAP (d->install_params_remote.info->serial_number2);

            break;

        case EMX_DATAGRAM_REMOTE_PARAMS_INFO :

            break;

        case EMX_DATAGRAM_RUNTIME_PARAMS :

            d->runtime_params.info->min_depth = CPL_BSWAP (d->runtime_params.info->min_depth);
            d->runtime_params.info->max_depth = CPL_BSWAP (d->runtime_params.info->max_depth);
            d->runtime_params.info->absorption = CPL_BSWAP (d->runtime_params.info->absorption);
            d->runtime_params.info->tx_pulse_length = CPL_BSWAP (d->runtime_params.info->tx_pulse_length);
            d->runtime_params.info->tx_beamwidth = CPL_BSWAP (d->runtime_params.info->tx_beamwidth);
            d->runtime_params.info->max_port_swath = CPL_BSWAP (d->runtime_params.info->max_port_swath);
            d->runtime_params.info->max_stbd_swath = CPL_BSWAP (d->runtime_params.info->max_stbd_swath);
            d->runtime_params.info->tx_along_tilt = CPL_BSWAP (d->runtime_params.info->tx_along_tilt);

            break;

        case EMX_DATAGRAM_EXTRA_PARAMS :

            d->extra_params.info->content = CPL_BSWAP (d->extra_params.info->content);

            switch (d->extra_params.info->content) {

                /* TODO: Add content 1-5 here. */

                case 6 :
                    d->extra_params.data.bs_corr.num_chars = CPL_BSWAP (d->extra_params.data.bs_corr.num_chars);
                    break;

                default : break;
            }

            break;

        case EMX_DATAGRAM_PU_OUTPUT :

            d->pu_output.info->udp_port1 = CPL_BSWAP (d->pu_output.info->udp_port1);
            d->pu_output.info->udp_port2 = CPL_BSWAP (d->pu_output.info->udp_port2);
            d->pu_output.info->udp_port3 = CPL_BSWAP (d->pu_output.info->udp_port3);
            d->pu_output.info->udp_port4 = CPL_BSWAP (d->pu_output.info->udp_port4);
            d->pu_output.info->system_descriptor = CPL_BSWAP (d->pu_output.info->system_descriptor);
            d->pu_output.info->host_ip_address = CPL_BSWAP (d->pu_output.info->host_ip_address);

            break;

        case EMX_DATAGRAM_PU_STATUS :

            d->pu_status.info->ping_rate = CPL_BSWAP (d->pu_status.info->ping_rate);
            d->pu_status.info->ping_counter = CPL_BSWAP (d->pu_status.info->ping_counter);
            d->pu_status.info->swath_distance = CPL_BSWAP (d->pu_status.info->swath_distance);
            d->pu_status.info->status_udp_port_2 = CPL_BSWAP (d->pu_status.info->status_udp_port_2);
            d->pu_status.info->status_serial_port_1 = CPL_BSWAP (d->pu_status.info->status_serial_port_1);
            d->pu_status.info->status_serial_port_2 = CPL_BSWAP (d->pu_status.info->status_serial_port_2);
            d->pu_status.info->status_serial_port_3 = CPL_BSWAP (d->pu_status.info->status_serial_port_3);
            d->pu_status.info->status_serial_port_4 = CPL_BSWAP (d->pu_status.info->status_serial_port_4);
            d->pu_status.info->heading = CPL_BSWAP (d->pu_status.info->heading);
            d->pu_status.info->roll = CPL_BSWAP (d->pu_status.info->roll);
            d->pu_status.info->pitch = CPL_BSWAP (d->pu_status.info->pitch);
            d->pu_status.info->heave = CPL_BSWAP (d->pu_status.info->heave);
            d->pu_status.info->sound_speed = CPL_BSWAP (d->pu_status.info->sound_speed);
            d->pu_status.info->depth = CPL_BSWAP (d->pu_status.info->depth);
            d->pu_status.info->velocity = CPL_BSWAP (d->pu_status.info->velocity);
            d->pu_status.info->range_normal = CPL_BSWAP (d->pu_status.info->range_normal);
            d->pu_status.info->sound_speed_svp = CPL_BSWAP (d->pu_status.info->sound_speed_svp);
            d->pu_status.info->yaw_stabilization = CPL_BSWAP (d->pu_status.info->yaw_stabilization);
            d->pu_status.info->port_coverage2 = CPL_BSWAP (d->pu_status.info->port_coverage2);
            d->pu_status.info->stbd_coverage2 = CPL_BSWAP (d->pu_status.info->stbd_coverage2);

            break;

        case EMX_DATAGRAM_PU_BIST_RESULT :

            d->pu_bist_result.info->test_number = CPL_BSWAP (d->pu_bist_result.info->test_number);
            d->pu_bist_result.info->test_result_status = CPL_BSWAP (d->pu_bist_result.info->test_result_status);

            break;

        case EMX_DATAGRAM_TRANSDUCER_TILT :

            d->tilt.info->num_entries = CPL_BSWAP (d->tilt.info->num_entries);

            for (i=0; i<d->tilt.info->num_entries; i++) {
                d->tilt.data[i].record_time = CPL_BSWAP (d->tilt.data[i].record_time);
                d->tilt.data[i].tilt = CPL_BSWAP (d->tilt.data[i].tilt);
            }

            break;

        case EMX_DATAGRAM_SYSTEM_STATUS :

            /* TODO: */

            break;

        case EMX_DATAGRAM_HISAS_STATUS        : CPL_ATTRIBUTE_FALLTHROUGH;
        case EMX_DATAGRAM_NAVIGATION_OUTPUT   : CPL_ATTRIBUTE_FALLTHROUGH;
        case EMX_DATAGRAM_SIDESCAN_STATUS     : CPL_ATTRIBUTE_FALLTHROUGH;
        case EMX_DATAGRAM_HISAS_1032_SIDESCAN : CPL_ATTRIBUTE_FALLTHROUGH;
        case EMX_DATAGRAM_RRA_123             :  /* No byte swapping on HISAS datagrams. */
            break;

        default : break;
    }
}


/******************************************************************************
*
* emx_valid_checksum - Return non-zero if the datagram check sum is valid and
*   the end byte (ETX) is valid.  The checksum is a simple sum of the bytes
*   between the STX and ETX bytes.
*
******************************************************************************/

static int emx_valid_checksum (
    const emx_datagram_header *header,
    const char *buffer,
    const int swap) {

    const uint8_t *p;
    uint16_t chksum_file;
    uint16_t chksum;
    size_t i, n;


    assert (header);
    assert (buffer);

    /* Calculate the number of bytes between STX and the end of the datagram.
       emx_valid_header has already verified that bytes_in_datagram is >= 16. */
    n = header->bytes_in_datagram - 16;

    /* Make sure we have an End-of-Text (ETX) character. */
    if (n < 3) return 0;

    p = (const uint8_t *) buffer;

    /* Have found some data with end byte set to zero, so allow this also. */
    if ((p[n-3] != EMX_END_BYTE) && (p[n-3] != 0)) {
        cpl_debug (emx_debug, "Missing ETX end byte (%u, counter=%u, datagram_type=%u)\n", p[n-3], header->counter, header->datagram_type);
        return 0;
    }

    /* Extract the checksum from the datagram. */
    if (swap) {
        chksum_file = p[n-2];
        chksum_file = p[n-1] + 256 * chksum_file;
    } else {
        chksum_file = p[n-1];
        chksum_file = p[n-2] + 256 * chksum_file;
    }

    /* Calculate the checksum of the header. */
    p = &(header->datagram_type);

    chksum = 0;
    for (i=0; i<15; i++) {
        chksum += p[i];
    }

    /* Now calculate the checksum of the rest of the datagram. */
    p = (const uint8_t *) buffer;

    for (i=0; i<n-3; i++) {
        chksum += p[i];
    }

    /* If the checksum is zero, then skip the test. */
    if ((chksum_file == 0) && (chksum != 0)) {
        cpl_debug (emx_debug, "Missing or zero checksum (counter=%u, datagram_type=%u)\n", header->counter, header->datagram_type);
        return 1;
    }

    /* Compare the checksum values and return the result. */
    if (chksum != chksum_file) {
        cpl_debug (emx_debug, "Checksum failure (chksum=%u, chksum_file=%u, counter=%u, datagram_type=%u)\n", chksum, chksum_file, header->counter, header->datagram_type);
        return 0;
    }

    return 1;
}


/******************************************************************************
*
* set_buffer_size - Set the internal allocated read buffer size given by the
*   file handle H to at least SIZE bytes.
*
* Return: Non-zero if memory allocation failed.
*
* Errors: CS_ENOMEM
*
******************************************************************************/

static int set_buffer_size (
    emx_handle *h,
    const size_t size) {

    assert (h);

    if (size > h->buffer_size) {

        /* Allocate a new size of 1.5 * size bytes.  Use free/malloc instead of
           realloc to avoid copying an empty buffer. */
        h->buffer_size = size + (size + 1) / 2;
        if (h->buffer) cpl_free (h->buffer);

        h->buffer = (char *) cpl_malloc (h->buffer_size);
        if (!h->buffer) return CS_ENOMEM;
    }

    return 0;
}


/******************************************************************************
*
* emx_get_model - Return the system model number or return zero if the model
*   number is unrecognized.
*
******************************************************************************/

int emx_get_model (
    const uint16_t em_model_number) {

    int model;


    switch (em_model_number) {
        case  120 : model = EM_MODEL_120;          break;
        case  121 : model = EM_MODEL_121A;         break;
        case  122 : model = EM_MODEL_122;          break;
        case  124 : model = EM_MODEL_124;          break;
        case  300 : model = EM_MODEL_300;          break;
        case  302 : model = EM_MODEL_302;          break;
        case  710 : model = EM_MODEL_710;          break;
        case  712 : model = EM_MODEL_712;          break;
        case  850 : model = EM_MODEL_ME70BO;       break;
        case 1002 : model = EM_MODEL_1002;         break;
        case 2000 : model = EM_MODEL_2000;         break;
        case 2040 : model = EM_MODEL_2040;         break;
        case 2045 : model = EM_MODEL_2040C;        break;
        case 3000 : model = EM_MODEL_3000;         break;
        case 3002 : CPL_ATTRIBUTE_FALLTHROUGH;
        case 3003 : CPL_ATTRIBUTE_FALLTHROUGH;
        case 3004 : CPL_ATTRIBUTE_FALLTHROUGH;
        case 3005 : CPL_ATTRIBUTE_FALLTHROUGH;
        case 3006 : CPL_ATTRIBUTE_FALLTHROUGH;
        case 3007 : CPL_ATTRIBUTE_FALLTHROUGH;
        case 3008 : model = EM_MODEL_3000D;        break;
        case 3020 : model = EM_MODEL_3002;         break;
        case 10120 : model = 0;                    break;  /* TODO: */
        case 11032 : model = EM_MODEL_HISAS_1032;  break;
        case 11034 : model = EM_MODEL_HISAS_1032D; break;
        case 12040 : model = EM_MODEL_HISAS_2040;  break;
        default : model = 0; break;
    }

    return model;
}


/******************************************************************************
*
* emx_get_em3000d_sample_rate - Return the sample rate in Hertz of the EM3000D
*   given the EM_MODEL_NUMBER and SONAR_HEAD.  The sonar head is 1 for port
*   and 2 for starboard.
*
******************************************************************************/

unsigned int emx_get_em3000d_sample_rate (
    const uint16_t em_model_number,
    const int sonar_head) {
                                     /* 3002,  3003,  3004,  3005,  3006,  3007,  3008 */
    static const unsigned int s1[] = { 13956, 13956, 14293, 13956, 14621, 14293, 14621 };
    static const unsigned int s2[] = { 14621, 14621, 14621, 14293, 14293, 13956, 13956 };


    assert ((sonar_head == 1) || (sonar_head == 2));

    if ((em_model_number >= 3002) && (em_model_number <= 3008)) {
        if (sonar_head == 1) {
            return s1[em_model_number - 3002];
        }
        if (sonar_head == 2) {
            return s2[em_model_number - 3002];
        }
    }

    return 0;
}


/******************************************************************************
*
* emx_get_datagram_name - Return a character string of the name of the datagram
*   given by DATAGRAM_TYPE.  This string should not be modified by the caller.
*
******************************************************************************/

const char * emx_get_datagram_name (
    const uint8_t datagram_type) {

    const char *s;


    switch (datagram_type) {

        case EMX_DATAGRAM_DEPTH                 : s = "EMX_DATAGRAM_DEPTH";                 break;
        case EMX_DATAGRAM_DEPTH_NOMINAL         : s = "EMX_DATAGRAM_DEPTH_NOMINAL";         break;
        case EMX_DATAGRAM_XYZ                   : s = "EMX_DATAGRAM_XYZ";                   break;
        case EMX_DATAGRAM_EXTRA_DETECTIONS      : s = "EMX_DATAGRAM_EXTRA_DETECTIONS";      break;
        case EMX_DATAGRAM_CENTRAL_BEAMS         : s = "EMX_DATAGRAM_CENTRAL_BEAMS";         break;
        case EMX_DATAGRAM_RRA_101               : s = "EMX_DATAGRAM_RRA_101";               break;
        case EMX_DATAGRAM_RRA_70                : s = "EMX_DATAGRAM_RRA_70";                break;
        case EMX_DATAGRAM_RRA_102               : s = "EMX_DATAGRAM_RRA_102";               break;
        case EMX_DATAGRAM_RRA_78                : s = "EMX_DATAGRAM_RRA_78";                break;
        case EMX_DATAGRAM_SEABED_IMAGE_83       : s = "EMX_DATAGRAM_SEABED_IMAGE_83";       break;
        case EMX_DATAGRAM_SEABED_IMAGE_89       : s = "EMX_DATAGRAM_SEABED_IMAGE_89";       break;
        case EMX_DATAGRAM_WATER_COLUMN          : s = "EMX_DATAGRAM_WATER_COLUMN";          break;
        case EMX_DATAGRAM_QUALITY_FACTOR        : s = "EMX_DATAGRAM_QUALITY_FACTOR";        break;
        case EMX_DATAGRAM_ATTITUDE              : s = "EMX_DATAGRAM_ATTITUDE";              break;
        case EMX_DATAGRAM_ATTITUDE_NETWORK      : s = "EMX_DATAGRAM_ATTITUDE_NETWORK";      break;
        case EMX_DATAGRAM_CLOCK                 : s = "EMX_DATAGRAM_CLOCK";                 break;
        case EMX_DATAGRAM_HEIGHT                : s = "EMX_DATAGRAM_HEIGHT";                break;
        case EMX_DATAGRAM_HEADING               : s = "EMX_DATAGRAM_HEADING";               break;
        case EMX_DATAGRAM_POSITION              : s = "EMX_DATAGRAM_POSITION";              break;
        case EMX_DATAGRAM_SINGLE_BEAM_DEPTH     : s = "EMX_DATAGRAM_SINGLE_BEAM_DEPTH";     break;
        case EMX_DATAGRAM_TIDE                  : s = "EMX_DATAGRAM_TIDE";                  break;
        case EMX_DATAGRAM_SSSV                  : s = "EMX_DATAGRAM_SSSV";                  break;
        case EMX_DATAGRAM_SVP                   : s = "EMX_DATAGRAM_SVP";                   break;
        case EMX_DATAGRAM_SVP_EM3000            : s = "EMX_DATAGRAM_SVP_EM3000";            break;
        case EMX_DATAGRAM_KM_SSP_OUTPUT         : s = "EMX_DATAGRAM_KM_SSP_OUTPUT";         break;
        case EMX_DATAGRAM_INSTALL_PARAMS        : s = "EMX_DATAGRAM_INSTALL_PARAMS";        break;
        case EMX_DATAGRAM_INSTALL_PARAMS_STOP   : s = "EMX_DATAGRAM_INSTALL_PARAMS_STOP";   break;
        case EMX_DATAGRAM_INSTALL_PARAMS_REMOTE : s = "EMX_DATAGRAM_INSTALL_PARAMS_REMOTE"; break;
        case EMX_DATAGRAM_REMOTE_PARAMS_INFO    : s = "EMX_DATAGRAM_REMOTE_PARAMS_INFO";    break;
        case EMX_DATAGRAM_RUNTIME_PARAMS        : s = "EMX_DATAGRAM_RUNTIME_PARAMS";        break;
        case EMX_DATAGRAM_EXTRA_PARAMS          : s = "EMX_DATAGRAM_EXTRA_PARAMS";          break;
        case EMX_DATAGRAM_PU_OUTPUT             : s = "EMX_DATAGRAM_PU_OUTPUT";             break;
        case EMX_DATAGRAM_PU_STATUS             : s = "EMX_DATAGRAM_PU_STATUS";             break;
        case EMX_DATAGRAM_PU_BIST_RESULT        : s = "EMX_DATAGRAM_PU_BIST_RESULT";        break;
        case EMX_DATAGRAM_TRANSDUCER_TILT       : s = "EMX_DATAGRAM_TRANSDUCER_TILT";       break;
        case EMX_DATAGRAM_SYSTEM_STATUS         : s = "EMX_DATAGRAM_SYSTEM_STATUS";         break;
        case EMX_DATAGRAM_STAVE                 : s = "EMX_DATAGRAM_STAVE";                 break;
        case EMX_DATAGRAM_HISAS_STATUS          : s = "EMX_DATAGRAM_HISAS_STATUS";          break;
        case EMX_DATAGRAM_NAVIGATION_OUTPUT     : s = "EMX_DATAGRAM_NAVIGATION_OUTPUT";     break;
        case EMX_DATAGRAM_SIDESCAN_STATUS       : s = "EMX_DATAGRAM_SIDESCAN_STATUS";       break;
        case EMX_DATAGRAM_HISAS_1032_SIDESCAN   : s = "EMX_DATAGRAM_HISAS_1032_SIDESCAN";   break;
        case EMX_DATAGRAM_RRA_123               : s = "EMX_DATAGRAM_RRA_123";               break;
        default                                 : s = "UNKNOWN";                            break;
    }

    return s;
}
