/* kma_reader.c -- Read Kongsberg KMALL file format.

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

#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "kma_reader.h"
#include "cpl_alloc.h"
#include "cpl_error.h"
#include "cpl_debug.h"
#include "cpl_file.h"


/* KMA File Handle */
struct kma_handle_struct {
    char *buffer;            /* File I/O buffer.            */
    size_t buffer_size;      /* Allocated buffer size.      */
    kma_data d;              /* Static kma_data struct.     */
    int fd;                  /* File descriptor.            */
    int kma_errno;           /* Error condition code.       */
    int ignore_mwc;          /* Boolean to ignore MWC data. */
    int ignore_mrz;          /* Boolean to ignore MRZ data. */
};


/* Private Function Prototypes */
static int kma_valid_header (const kma_datagram_header *) CPL_ATTRIBUTE_NONNULL_ALL;
static int set_buffer_size (kma_handle *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;


/* Private Variables */
static int kma_debug = 0;


/******************************************************************************
*
* kma_open - Open the file given by FILE_NAME and return a handle to it.  The
*   caller must call kma_close() to close the file and free the memory after use.
*
* Return: A file handle pointer to the open file, or
*         NULL if the file can not be opened for reading.
*
******************************************************************************/

kma_handle * kma_open (
    const char *file_name) {

    kma_handle *h;
    int fd;


    /* Make sure there is no padding in the data structs. */
    assert (sizeof (kma_datagram_header) == 20);
    assert (sizeof (kma_datagram_iip_data) == 6);
    assert (sizeof (kma_datagram_iop_data) == 6);
    assert (sizeof (kma_datagram_m_partition) == 4);
    assert (sizeof (kma_datagram_m_common) == 12);
    assert (sizeof (kma_datagram_mrz_ping_info) == 152);
    assert (sizeof (kma_datagram_mrz_tx_sector_info_v0) == 36);
    assert (sizeof (kma_datagram_mrz_tx_sector_info_v1) == 48);
    assert (sizeof (kma_datagram_mrz_rx_info) == 32);
    assert (sizeof (kma_datagram_mrz_extra_det_info) == 4);
    assert (sizeof (kma_datagram_mrz_sounding) == 120);
    assert (sizeof (kma_datagram_mwc_tx_info) == 12);
    assert (sizeof (kma_datagram_mwc_tx_sector_info) == 16);
    assert (sizeof (kma_datagram_mwc_rx_info) == 16);
    assert (sizeof (kma_datagram_s_common) == 8);
    assert (sizeof (kma_datagram_spo_data) == 40);
    assert (sizeof (kma_datagram_skm_info) == 12);
    assert (sizeof (kma_datagram_skm_binary) == 120);
    assert (sizeof (kma_datagram_skm_delayed_heave) == 12);
    assert (sizeof (kma_datagram_skm_sample) == 132);
    assert (sizeof (kma_datagram_svp_info) == 28);
    assert (sizeof (kma_datagram_svp_sample) == 20);
    assert (sizeof (kma_datagram_svt_info) == 20);
    assert (sizeof (kma_datagram_svt_sample) == 24);
    assert (sizeof (kma_datagram_scl_data) == 8);
    assert (sizeof (kma_datagram_sde_data_v0) == 28);
    assert (sizeof (kma_datagram_sde_data_v1) == 32);
    assert (sizeof (kma_datagram_shi_data) == 6);
    assert (sizeof (kma_datagram_cpo_data) == 40);
    assert (sizeof (kma_datagram_che_data) == 4);
    assert (sizeof (kma_datagram_f_common) == 8);

    if (!file_name) return NULL;

    cpl_debug (kma_debug, "Opening file '%s'\n", file_name);

    /* Open the binary file for read-only access and save the file descriptor. */
    fd = cpl_open (file_name, CPL_OPEN_RDONLY | CPL_OPEN_BINARY | CPL_OPEN_SEQUENTIAL);

    if (fd < 0) {
        cpl_debug (kma_debug, "Failed to open file '%s'\n", file_name);
        return NULL;
    }

    /* Allocate memory storage for the handle.  This must be free'd later. */
    h = (kma_handle *) cpl_malloc (sizeof (kma_handle));
    if (!h) {
        cpl_close (fd);
        return NULL;
    }

    /* Initialize the file buffer. */
    h->buffer_size = 0;
    h->buffer = NULL;

    /* Initialize the file handle. */
    h->kma_errno = CS_ENONE;
    h->fd = fd;

    /* Set boolean to ignore watercolumn data to false by default. */
    h->ignore_mwc = 0;

    /* Set boolean to ignore multibeam sounding data to false by default. */
    h->ignore_mrz = 0;

    return h;
}


/******************************************************************************
*
* kma_close - Close the file and free allocated memory given by the handle H.
*
* Return: 0 if the file is successfully closed, or
*         error condition if an error occurred.
*
* Errors: CS_ECLOSE
*
******************************************************************************/

int kma_close (
    kma_handle *h) {

    int status = CS_ENONE;


    if (h) {
        cpl_debug (kma_debug, "Closing file\n");

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
* kma_read - Read the datagram and channel data from the file given by the
*   handle H.  Data is read at the current file position.  When finished the
*   file pointer will be at the start of the next datagram.  The memory
*   storage for the datagram will be overwritten in subsequent calls to this
*   function or if the file is closed.  If they are to be preserved, then they
*   should be copied to a user-defined buffer location.
*
* Return: A pointer to the data struct if the data was read successfully,
*         NULL if no valid data was found or EOF was reached.
*
* Errors: CS_ENOMEM
*         CS_EREAD
*         CS_ESEEK
*         CS_EBADDATA
*
******************************************************************************/

kma_data * kma_read (
    kma_handle *h) {

    uint32_t datagram_size;
    size_t actual_read_size;
    size_t read_size;
    ssize_t result;
    char *p;


    assert (h);

    /* Set the read size to the datagram header size. */
L1: read_size = sizeof (kma_datagram_header);

    /* Read the datagram header from the file descriptor. */
    result = cpl_read (&(h->d.header), read_size, h->fd);
    if (result < 0) {
        cpl_debug (kma_debug, "Read error occurred\n");
        h->kma_errno = CS_EREAD;
        return NULL;
    }

    actual_read_size = (size_t) result;

    /* If the read amount is zero, then it is EOF, otherwise if the read
       amount is less than the datagram size, then the file is corrupt. */
    if (actual_read_size != read_size) {
        if (actual_read_size != 0) {
            cpl_debug (kma_debug, "Unexpected end of file\n");
            h->kma_errno = CS_EBADDATA;
        }
        return NULL;
    }

    /* Verify that the datagram header is valid. */
    if (kma_valid_header (&(h->d.header)) == 0) {
        cpl_debug (kma_debug, "Invalid header\n");
        h->kma_errno = CS_EBADDATA;
        return NULL;
    }

    cpl_debug (kma_debug >= 2, "numBytesDgm=%4u, systemID=%u, echoSoundID=%u, time=%u:%u, dgmType=%u (%s)\n",
        h->d.header.numBytesDgm, h->d.header.systemID, h->d.header.echoSounderID, h->d.header.time_sec, h->d.header.time_nanosec,
        h->d.header.dgmType, kma_get_datagram_name (h->d.header.dgmType));

    /* Now read the rest of the datagram.  The read size is the datagram size minus the header we
       just read.  The kma_valid_header function above should verify that this is a positive value. */
    read_size = h->d.header.numBytesDgm - sizeof (kma_datagram_header);

    /* Skip any water column datagrams if requested.  These datagrams are large, so if
       the read time is greater than the seek time on the storage device, then seeking
       past this data is advantageous if this data is not desired. */
    if (h->ignore_mwc && (h->d.header.dgmType == KMA_DATAGRAM_MWC)) {
        if (cpl_seek (h->fd, read_size, SEEK_CUR) < 0) {
            cpl_debug (kma_debug, "Seek failed\n");
            h->kma_errno = CS_ESEEK;
            return NULL;
        }

        /* Now go back and read another datagram. */
        goto L1;
    }

    /* Skip any multibeam datagrams if requested. */
    if (h->ignore_mrz && (h->d.header.dgmType == KMA_DATAGRAM_MRZ)) {
        if (cpl_seek (h->fd, read_size, SEEK_CUR) < 0) {
            cpl_debug (kma_debug, "Seek failed\n");
            h->kma_errno = CS_ESEEK;
            return NULL;
        }

        /* Now go back and read another datagram. */
        goto L1;
    }

    /* Resize the buffer as needed to fit the datagram size. */
    if (set_buffer_size (h, read_size) != 0) {
        h->kma_errno = CS_ENOMEM;
        return NULL;
    }

    /* Read the rest of the datagram from the file descriptor to the buffer. */
    result = cpl_read (h->buffer, read_size, h->fd);
    if (result < 0) {
        cpl_debug (kma_debug, "Read error occurred\n");
        h->kma_errno = CS_EREAD;
        return NULL;
    }

    actual_read_size = (size_t) result;

    /* Make sure we read at least the minimum size. */
    if (actual_read_size != read_size) {
        /* If the expected amount was not read, then the file is corrupt. */
        cpl_debug (kma_debug, "Unexpected end of file\n");
        h->kma_errno = CS_EBADDATA;
        return NULL;
    }

    /* Set the pointer p to the buffer, which is the start of the datagram (after the header). */
    p = h->buffer;

    /* Set the pointers of the datagram array (channel) data into the correct places in the buffer. */
    switch (h->d.header.dgmType) {

        case KMA_DATAGRAM_IIP :

            h->d.datagram.iip.data = (kma_datagram_iip_data *) p;
            p += sizeof (kma_datagram_iip_data);
            h->d.datagram.iip.install_text = (uint8_t *) p;

            datagram_size = h->d.datagram.iip.data->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected IIP datagram size (%u,%u)\n", h->d.datagram.iip.data->numBytesCmnPart, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_IOP :

            h->d.datagram.iop.data = (kma_datagram_iop_data *) p;
            p += sizeof (kma_datagram_iop_data);
            h->d.datagram.iop.runtime_text = (uint8_t *) p;

            datagram_size = h->d.datagram.iop.data->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected IOP datagram size (%u,%u)\n", h->d.datagram.iop.data->numBytesCmnPart, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_IBE :

            h->d.datagram.ibe.data = (kma_datagram_b_data *) p;
            p += sizeof (kma_datagram_b_data);
            h->d.datagram.ibe.bist_text = (uint8_t *) p;

            datagram_size = h->d.datagram.ibe.data->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected IBE datagram size (%u,%u)\n", h->d.datagram.ibe.data->numBytesCmnPart, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_IBR :

            h->d.datagram.ibr.data = (kma_datagram_b_data *) p;
            p += sizeof (kma_datagram_b_data);
            h->d.datagram.ibr.bist_text = (uint8_t *) p;

            datagram_size = h->d.datagram.ibr.data->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected IBR datagram size (%u,%u)\n", h->d.datagram.ibr.data->numBytesCmnPart, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_IBS :

            h->d.datagram.ibs.data = (kma_datagram_b_data *) p;
            p += sizeof (kma_datagram_b_data);
            h->d.datagram.ibs.bist_text = (uint8_t *) p;

            datagram_size = h->d.datagram.ibs.data->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected IBS datagram size (%u,%u)\n", h->d.datagram.ibs.data->numBytesCmnPart, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_MRZ :

            h->d.datagram.mrz.partition = (kma_datagram_m_partition *) p;
            p += sizeof (kma_datagram_m_partition);

            if ((h->d.datagram.mrz.partition->dgmNum != 1) || (h->d.datagram.mrz.partition->numOfDgms != 1)) {
                cpl_debug (kma_debug, "Unexpected datagram partition (%u,%u)\n", h->d.datagram.mrz.partition->dgmNum, h->d.datagram.mrz.partition->numOfDgms);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            /* TODO: Sanity check that all pointers are contained within the datagram buffer. */
            h->d.datagram.mrz.common = (kma_datagram_m_common *) p;
            p += h->d.datagram.mrz.common->numBytesCmnPart;
            h->d.datagram.mrz.pingInfo = (kma_datagram_mrz_ping_info *) p;
            p += h->d.datagram.mrz.pingInfo->numBytesInfoData;
            if (h->d.header.dgmVersion == 0) {
                h->d.datagram.mrz.txSectorInfo.v0 = (kma_datagram_mrz_tx_sector_info_v0 *) p;
                h->d.datagram.mrz.txSectorInfo.v1 = NULL;
            } else {
                h->d.datagram.mrz.txSectorInfo.v0 = NULL;
                h->d.datagram.mrz.txSectorInfo.v1 = (kma_datagram_mrz_tx_sector_info_v1 *) p;
            }
            p += h->d.datagram.mrz.pingInfo->numTxSectors * h->d.datagram.mrz.pingInfo->numBytesPerTxSector;
            h->d.datagram.mrz.rxInfo = (kma_datagram_mrz_rx_info *) p;
            p += h->d.datagram.mrz.rxInfo->numBytesRxInfo;
            h->d.datagram.mrz.extraDetInfo = (kma_datagram_mrz_extra_det_info *) p;
            p += h->d.datagram.mrz.rxInfo->numExtraDetectionClasses * h->d.datagram.mrz.rxInfo->numBytesPerClass;
            h->d.datagram.mrz.sounding = (kma_datagram_mrz_sounding *) p;
            p += (h->d.datagram.mrz.rxInfo->numSoundingsMaxMain + h->d.datagram.mrz.rxInfo->numExtraDetections) * h->d.datagram.mrz.rxInfo->numBytesPerSounding;
            h->d.datagram.mrz.sample = (int16_t *) p;

            if (h->d.datagram.mrz.pingInfo->numTxSectors == 0) {
                h->d.datagram.mrz.txSectorInfo.v0 = NULL;
                h->d.datagram.mrz.txSectorInfo.v1 = NULL;
            }

            if (h->d.datagram.mrz.rxInfo->numExtraDetectionClasses == 0) {
                h->d.datagram.mrz.extraDetInfo = NULL;
            }

            if (h->d.datagram.mrz.rxInfo->numSoundingsMaxMain + h->d.datagram.mrz.rxInfo->numExtraDetections == 0) {
                h->d.datagram.mrz.sounding = NULL;
            }

            break;

        case KMA_DATAGRAM_MWC :

            h->d.datagram.mwc.partition = (kma_datagram_m_partition *) p;
            p += sizeof (kma_datagram_m_partition);

            if ((h->d.datagram.mwc.partition->dgmNum != 1) || (h->d.datagram.mwc.partition->numOfDgms != 1)) {
                cpl_debug (kma_debug, "Unexpected datagram partition (%u,%u)\n", h->d.datagram.mwc.partition->dgmNum, h->d.datagram.mwc.partition->numOfDgms);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            /* TODO: Sanity check that all pointers are contained within the datagram buffer. */
            h->d.datagram.mwc.common = (kma_datagram_m_common *) p;
            p += h->d.datagram.mwc.common->numBytesCmnPart;
            h->d.datagram.mwc.txInfo = (kma_datagram_mwc_tx_info *) p;
            p += h->d.datagram.mwc.txInfo->numBytesTxInfo;
            h->d.datagram.mwc.txSectorInfo = (kma_datagram_mwc_tx_sector_info *) p;
            p += h->d.datagram.mwc.txInfo->numTxSectors * h->d.datagram.mwc.txInfo->numBytesPerTxSector;
            h->d.datagram.mwc.rxInfo = (kma_datagram_mwc_rx_info *) p;
            p += h->d.datagram.mwc.rxInfo->numBytesRxInfo;
            h->d.datagram.mwc.beamData = (uint8_t *) p;

            if (h->d.datagram.mwc.rxInfo->numBeams == 0) {
                h->d.datagram.mwc.beamData = NULL;
            }

            break;

        case KMA_DATAGRAM_SPO :

            h->d.datagram.spo.common = (kma_datagram_s_common *) p;
            p += h->d.datagram.spo.common->numBytesCmnPart;
            h->d.datagram.spo.data = (kma_datagram_spo_data *) p;
            p += sizeof (kma_datagram_spo_data);
            h->d.datagram.spo.dataFromSensor = (uint8_t *) p;

            datagram_size = h->d.datagram.spo.common->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SPO datagram size (%u,%u)\n", h->d.datagram.spo.common->numBytesCmnPart, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_SKM :

            h->d.datagram.skm.info = (kma_datagram_skm_info *) p;
            p += h->d.datagram.skm.info->numBytesInfoPart;
            h->d.datagram.skm.data = (kma_datagram_skm_sample *) p;

            datagram_size = h->d.datagram.skm.info->numBytesInfoPart + ((uint32_t) h->d.datagram.skm.info->numBytesPerSample) * h->d.datagram.skm.info->numSamples +
                sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SKM datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            if (h->d.datagram.skm.info->numSamples == 0) {
                h->d.datagram.skm.data = NULL;
            }

            break;

        case KMA_DATAGRAM_SVP :

            h->d.datagram.svp.info = (kma_datagram_svp_info *) p;
            p += h->d.datagram.svp.info->numBytesInfoPart;
            h->d.datagram.svp.data = (kma_datagram_svp_sample *) p;

            datagram_size = h->d.datagram.svp.info->numBytesInfoPart + h->d.datagram.svp.info->numSamples * sizeof (kma_datagram_svp_sample) + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SVP datagram size (%u,%u)\n", h->d.datagram.svp.info->numSamples, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            if (h->d.datagram.svp.info->numSamples == 0) {
                h->d.datagram.svp.data = NULL;
            }

            break;

        case KMA_DATAGRAM_SVT :

            h->d.datagram.svt.info = (kma_datagram_svt_info *) p;
            p += h->d.datagram.svt.info->numBytesInfoPart;
            h->d.datagram.svt.data = (kma_datagram_svt_sample *) p;

            datagram_size = h->d.datagram.svt.info->numBytesInfoPart + ((uint32_t) h->d.datagram.svt.info->numBytesPerSample) * h->d.datagram.svt.info->numSamples + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SVT datagram size (%u,%u)\n", h->d.datagram.svt.info->numSamples, h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            if (h->d.datagram.svp.info->numSamples == 0) {
                h->d.datagram.svp.data = NULL;
            }

            break;

        case KMA_DATAGRAM_SCL :

            h->d.datagram.scl.common = (kma_datagram_s_common *) p;
            p += h->d.datagram.scl.common->numBytesCmnPart;
            h->d.datagram.scl.data = (kma_datagram_scl_data *) p;
            p += sizeof (kma_datagram_scl_data);
            h->d.datagram.scl.dataFromSensor = (uint8_t *) p;

            datagram_size = h->d.datagram.scl.common->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SCL datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_SDE :

            h->d.datagram.sde.common = (kma_datagram_s_common *) p;
            p += h->d.datagram.sde.common->numBytesCmnPart;
            if (h->d.header.dgmVersion == 0) {
                h->d.datagram.sde.data.v0 = (kma_datagram_sde_data_v0 *) p;
                h->d.datagram.sde.data.v1 = NULL;
                p += sizeof (kma_datagram_sde_data_v0);
            } else if (h->d.header.dgmVersion >= 1) {
                h->d.datagram.sde.data.v0 = NULL;
                h->d.datagram.sde.data.v1 = (kma_datagram_sde_data_v1 *) p;
                p += sizeof (kma_datagram_sde_data_v1);
            }
            h->d.datagram.sde.dataFromSensor = (uint8_t *) p;

            datagram_size = h->d.datagram.sde.common->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SDE datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_SHI :

            h->d.datagram.shi.common = (kma_datagram_s_common *) p;
            p += h->d.datagram.shi.common->numBytesCmnPart;
            h->d.datagram.shi.data = (kma_datagram_shi_data *) p;
            p += sizeof (kma_datagram_shi_data);
            h->d.datagram.shi.dataFromSensor = (uint8_t *) p;

            datagram_size = h->d.datagram.shi.common->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected SHI datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_CPO :

            h->d.datagram.cpo.common = (kma_datagram_s_common *) p;
            p += h->d.datagram.cpo.common->numBytesCmnPart;
            h->d.datagram.cpo.data = (kma_datagram_cpo_data *) p;
            p += sizeof (kma_datagram_cpo_data);
            h->d.datagram.cpo.dataFromSensor = (uint8_t *) p;

            datagram_size = h->d.datagram.cpo.common->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected CPO datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_CHE :

            h->d.datagram.che.common = (kma_datagram_m_common *) p;
            p += h->d.datagram.che.common->numBytesCmnPart;
            h->d.datagram.che.data = (kma_datagram_che_data *) p;

            datagram_size = h->d.datagram.che.common->numBytesCmnPart + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected CHE datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        case KMA_DATAGRAM_FCF :

            h->d.datagram.fcf.partition = (kma_datagram_m_partition *) p;
            p += sizeof (kma_datagram_m_partition);
            h->d.datagram.fcf.common = (kma_datagram_f_common *) p;
            h->d.datagram.fcf.fileName = (char *) (p + sizeof (kma_datagram_f_common));
            p += h->d.datagram.fcf.common->numBytesCmnPart;
            h->d.datagram.fcf.bsCalibrationFile = (uint8_t *) p;

            datagram_size = h->d.datagram.fcf.common->numBytesCmnPart + sizeof (kma_datagram_m_partition) + sizeof (kma_datagram_header) + sizeof (uint32_t);

            if (datagram_size > h->d.header.numBytesDgm) {
                cpl_debug (kma_debug, "Unexpected FCF datagram size (%u)\n", h->d.header.numBytesDgm);
                h->kma_errno = CS_EBADDATA;
                return NULL;
            }

            break;

        default :

            cpl_debug (kma_debug, "Unknown message type (%u) of size %u bytes\n",
                h->d.header.dgmType, h->d.header.numBytesDgm);

            break;
    }

    return &(h->d);
}


/******************************************************************************
*
* kma_print - Print the data in D to the file stream FP in a human-readable format.
*
******************************************************************************/

void kma_print (
    FILE *fp,
    const kma_data *d,
    const int output_level) {

    size_t num_bytes;
    size_t i;


    assert (fp);

    if (!d) return;

    fprintf (fp, "\
====== DATAGRAM =======\n\
numBytesDgm           : %u Bytes\n\
dgmType               : %s\n\
dgmVersion            : %u\n\
systemID              : %u\n\
echoSounderID         : %u\n\
time_sec              : %u\n\
time_nanosec          : %u\n\
-----------------------\n", d->header.numBytesDgm, kma_get_datagram_name (d->header.dgmType),
        d->header.dgmVersion, d->header.systemID, d->header.echoSounderID, d->header.time_sec,
        d->header.time_nanosec);

    switch (d->header.dgmType) {

        case KMA_DATAGRAM_IIP :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
info                  : %u\n\
status                : %u\n", d->datagram.iip.data->numBytesCmnPart, d->datagram.iip.data->info, d->datagram.iip.data->status);

            fputs ("\
install_txt           : ", fp);

            num_bytes = d->datagram.iip.data->numBytesCmnPart - sizeof (kma_datagram_iip_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.iip.install_text[i] == '\0') break;
                if (d->datagram.iip.install_text[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.iip.install_text[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.iip.install_text[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_IOP :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
info                  : %u\n\
status                : %u\n", d->datagram.iop.data->numBytesCmnPart, d->datagram.iop.data->info, d->datagram.iop.data->status);

            fputs ("\
runtime_txt           : ", fp);

            num_bytes = d->datagram.iop.data->numBytesCmnPart - sizeof (kma_datagram_iop_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.iop.runtime_text[i] == '\0') break;
                if (d->datagram.iop.runtime_text[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.iop.runtime_text[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.iop.runtime_text[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_IBE :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
BISTInfo              : %u\n\
BISTStyle             : %u\n\
BISTNumber            : %u\n\
BISTStatus            : %d\n", d->datagram.ibe.data->numBytesCmnPart, d->datagram.ibe.data->BISTInfo,
                d->datagram.ibe.data->BISTStyle, d->datagram.ibe.data->BISTNumber, d->datagram.ibe.data->BISTStatus);

            fputs ("\
bist_text             : ", fp);

            num_bytes = d->datagram.ibe.data->numBytesCmnPart - sizeof (kma_datagram_b_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.ibe.bist_text[i] == '\0') break;
                if (d->datagram.ibe.bist_text[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.ibe.bist_text[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.ibe.bist_text[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_IBR :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
BISTInfo              : %u\n\
BISTStyle             : %u\n\
BISTNumber            : %u\n\
BISTStatus            : %d\n", d->datagram.ibr.data->numBytesCmnPart, d->datagram.ibr.data->BISTInfo,
                d->datagram.ibr.data->BISTStyle, d->datagram.ibr.data->BISTNumber, d->datagram.ibr.data->BISTStatus);

            fputs ("\
bist_text             : ", fp);

            num_bytes = d->datagram.ibr.data->numBytesCmnPart - sizeof (kma_datagram_b_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.ibr.bist_text[i] == '\0') break;
                if (d->datagram.ibr.bist_text[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.ibr.bist_text[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.ibr.bist_text[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_IBS :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
BISTInfo              : %u\n\
BISTStyle             : %u\n\
BISTNumber            : %u\n\
BISTStatus            : %d\n", d->datagram.ibs.data->numBytesCmnPart, d->datagram.ibs.data->BISTInfo,
                d->datagram.ibs.data->BISTStyle, d->datagram.ibs.data->BISTNumber, d->datagram.ibs.data->BISTStatus);

            fputs ("\
bist_text             : ", fp);

            num_bytes = d->datagram.ibs.data->numBytesCmnPart - sizeof (kma_datagram_b_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.ibs.bist_text[i] == '\0') break;
                if (d->datagram.ibs.bist_text[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.ibs.bist_text[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.ibs.bist_text[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_MRZ :

            fprintf (fp, "\
numOfDgms             : %u\n\
dgmNum                : %u\n", d->datagram.mrz.partition->numOfDgms, d->datagram.mrz.partition->dgmNum);

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
pingCnt               : %u\n\
rxFansPerPing         : %u\n\
rxFanIndex            : %u\n\
swathsPerPing         : %u\n\
swathAlongPosition    : %u\n\
txTransducerInd       : %u\n\
rxTransducerInd       : %u\n\
numRxTransducers      : %u\n\
algorithmType         : %u\n", d->datagram.mrz.common->numBytesCmnPart, d->datagram.mrz.common->pingCnt,
                d->datagram.mrz.common->rxFansPerPing, d->datagram.mrz.common->rxFanIndex, d->datagram.mrz.common->swathsPerPing,
                d->datagram.mrz.common->swathAlongPosition, d->datagram.mrz.common->txTransducerInd, d->datagram.mrz.common->rxTransducerInd,
                d->datagram.mrz.common->numRxTransducers, d->datagram.mrz.common->algorithmType);

            fprintf (fp, "\
------ Ping Info ------\n\
numBytesInfoData      : %u Bytes\n\
pingRate              : %0.5f Hz\n", d->datagram.mrz.pingInfo->numBytesInfoData, d->datagram.mrz.pingInfo->pingRate_Hz);

            fputs ("\
beamSpacing           : ", fp);

            switch (d->datagram.mrz.pingInfo->beamSpacing) {
                case 0 : fputs ("Equidistance\n", fp); break;
                case 1 : fputs ("Equiangle\n", fp); break;
                case 2 : fputs ("High density\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->beamSpacing); break;
            }

            fputs ("\
depthMode             : ", fp);

            switch (d->datagram.mrz.pingInfo->depthMode) {
                case   0 : fputs ("Very Shallow\n", fp); break;
                case   1 : fputs ("Shallow\n", fp); break;
                case   2 : fputs ("Medium\n", fp); break;
                case   3 : fputs ("Deep\n", fp); break;
                case   4 : fputs ("Deeper\n", fp); break;
                case   5 : fputs ("Very Deep\n", fp); break;
                case   6 : fputs ("Extra Deep\n", fp); break;
                case   7 : fputs ("Extreme Deep\n", fp); break;
                case 100 : fputs ("Very Shallow (Manual)\n", fp); break;
                case 101 : fputs ("Shallow (Manual)\n", fp); break;
                case 102 : fputs ("Medium (Manual)\n", fp); break;
                case 103 : fputs ("Deep (Manual)\n", fp); break;
                case 104 : fputs ("Deeper (Manual)\n", fp); break;
                case 105 : fputs ("Very Deep (Manual)\n", fp); break;
                case 106 : fputs ("Extra Deep (Manual)\n", fp); break;
                case 107 : fputs ("Extreme Deep (Manual)\n", fp); break;
                default :  fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->depthMode); break;
            }

            fprintf (fp, "\
subDepthMode          : %u\n\
distanceBtwSwath      : %u %%\n", d->datagram.mrz.pingInfo->subDepthMode, d->datagram.mrz.pingInfo->distanceBtwSwath);

            fputs ("\
detectionMode         : ", fp);

            switch (d->datagram.mrz.pingInfo->detectionMode) {
                case 0 : fputs ("Normal\n", fp); break;
                case 1 : fputs ("Waterway\n", fp); break;
                case 2 : fputs ("Tracking\n", fp); break;
                case 3 : fputs ("Minimum Depth\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->detectionMode); break;
            }

            fputs ("\
pulseForm             : ", fp);

            switch (d->datagram.mrz.pingInfo->pulseForm) {
                case 0 : fputs ("CW\n", fp); break;
                case 1 : fputs ("Mix\n", fp); break;
                case 2 : fputs ("FM\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->pulseForm); break;
            }

            fputs ("\
frequencyMode         : ", fp);

            if (d->datagram.mrz.pingInfo->frequencyMode_Hz == -1.0) {
                fputs ("Not Used\n", fp);
            } else if (d->datagram.mrz.pingInfo->frequencyMode_Hz == 0.0) {
                fputs ("40-100 kHz\n", fp);
            } else if (d->datagram.mrz.pingInfo->frequencyMode_Hz == 1.0) {
                fputs ("50-100 kHz\n", fp);
            } else if (d->datagram.mrz.pingInfo->frequencyMode_Hz == 2.0) {
                fputs ("70-100 kHz\n", fp);
            } else if (d->datagram.mrz.pingInfo->frequencyMode_Hz == 3.0) {
                fputs ("50 kHz\n", fp);
            } else if (d->datagram.mrz.pingInfo->frequencyMode_Hz == 4.0) {
                fputs ("40 kHz\n", fp);
            } else {
                fprintf (fp, "%0.2f Hz\n", d->datagram.mrz.pingInfo->frequencyMode_Hz);
            }

            fprintf (fp, "\
freqRangeLowLim       : %0.2f Hz\n\
freqRangeHighLim      : %0.2f Hz\n\
maxTotalTxPulseLength : %0.5f s\n\
maxEffTxPulseLength   : %0.5f s\n\
maxEffTxBandwidth     : %0.2f Hz\n\
absCoeff              : %0.3f dB/km\n\
portSectorEdge        : %0.2f deg\n\
starbSectorEdge       : %0.2f deg\n\
portMeanCov           : %0.2f deg\n\
starbMeanCov          : %0.2f deg\n\
portMeanCov           : %d m\n\
starbMeanCov          : %d m\n", d->datagram.mrz.pingInfo->freqRangeLowLim_Hz, d->datagram.mrz.pingInfo->freqRangeHighLim_Hz,
                d->datagram.mrz.pingInfo->maxTotalTxPulseLength_sec, d->datagram.mrz.pingInfo->maxEffTxPulseLength_sec,
                d->datagram.mrz.pingInfo->maxEffTxBandwidth_Hz, d->datagram.mrz.pingInfo->absCoeff_dBPerkm, d->datagram.mrz.pingInfo->portSectorEdge_deg,
                d->datagram.mrz.pingInfo->starbSectorEdge_deg, d->datagram.mrz.pingInfo->portMeanCov_deg, d->datagram.mrz.pingInfo->starbMeanCov_deg,
                d->datagram.mrz.pingInfo->portMeanCov_m, d->datagram.mrz.pingInfo->starbMeanCov_m);

            fprintf (fp, "\
modeAndStabilization  : %u\n", d->datagram.mrz.pingInfo->modeAndStabilization);

            if ((d->datagram.mrz.pingInfo->modeAndStabilization & 0x01) != 0) {
                fputs ("modeAndStabilization  : Pitch Stabilization On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->modeAndStabilization & 0x02) != 0) {
                fputs ("modeAndStabilization  : Yaw Stabilization On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->modeAndStabilization & 0x03) != 0) {
                fputs ("modeAndStabilization  : Sonar Mode On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->modeAndStabilization & 0x04) != 0) {
                fputs ("modeAndStabilization  : Angular Coverage Mode On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->modeAndStabilization & 0x05) != 0) {
                fputs ("modeAndStabilization  : Sector Mode On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->modeAndStabilization & 0x06) != 0) {
                fputs ("modeAndStabilization  : Swath Along Position Dynamic\n", fp);
            }

            fprintf (fp, "\
runtimeFilter1        : %u\n", d->datagram.mrz.pingInfo->runtimeFilter1);

            if ((d->datagram.mrz.pingInfo->runtimeFilter1 & 0x01) != 0) {
                fputs ("runtimeFilter1        : Slope Filter On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->runtimeFilter1 & 0x02) != 0) {
                fputs ("runtimeFilter1        : Aeration Filter On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->runtimeFilter1 & 0x03) != 0) {
                fputs ("runtimeFilter1        : Sector Filter On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->runtimeFilter1 & 0x04) != 0) {
                fputs ("runtimeFilter1        : Interference Filter On\n", fp);
            }
            if ((d->datagram.mrz.pingInfo->runtimeFilter1 & 0x05) != 0) {
                fputs ("runtimeFilter1        : Special Amplitude Detect On\n", fp);
            }

            fprintf (fp, "\
runtimeFilter2        : %u\n", d->datagram.mrz.pingInfo->runtimeFilter2);

            if (d->datagram.mrz.pingInfo->runtimeFilter2 != 0) {

                uint16_t range_gate_size = d->datagram.mrz.pingInfo->runtimeFilter2 & 0x000F;
                uint16_t spike_filter_strength = (d->datagram.mrz.pingInfo->runtimeFilter2 >> 4) & 0x000F;
                uint16_t penetration_filter = (d->datagram.mrz.pingInfo->runtimeFilter2 >> 8) & 0x000F;
                uint16_t phase_ramp = (d->datagram.mrz.pingInfo->runtimeFilter2 >> 12) & 0x000F;

                fputs ("\
runtimeFilter2        : Range Gate Size : ", fp);

                switch (range_gate_size) {
                    case 0 : fputs ("Small\n", fp); break;
                    case 1 : fputs ("Normal\n", fp); break;
                    case 2 : fputs ("Large\n", fp); break;
                    default : fprintf (fp, "%u\n", range_gate_size); break;
                }

                fputs ("\
runtimeFilter2        : Spike Filter Strength : ", fp);

                switch (spike_filter_strength) {
                    case 0 : fputs ("Off\n", fp); break;
                    case 1 : fputs ("Weak\n", fp); break;
                    case 2 : fputs ("Medium\n", fp); break;
                    case 3 : fputs ("Strong\n", fp); break;
                    default : fprintf (fp, "%u\n", spike_filter_strength); break;
                }

                fputs ("\
runtimeFilter2        : Penetration Filter : ", fp);

                switch (penetration_filter) {
                    case 0 : fputs ("Off\n", fp); break;
                    case 1 : fputs ("Weak\n", fp); break;
                    case 2 : fputs ("Medium\n", fp); break;
                    case 3 : fputs ("Strong\n", fp); break;
                    default : fprintf (fp, "%u\n", penetration_filter); break;
                }

                fputs ("\
runtimeFilter2        : Phase Ramp : ", fp);

                switch (phase_ramp) {
                    case 0 : fputs ("Short\n", fp); break;
                    case 1 : fputs ("Normal\n", fp); break;
                    case 2 : fputs ("Long\n", fp); break;
                    default : fprintf (fp, "%u\n", phase_ramp); break;
                }
            }

            fprintf (fp, "\
pipeTrackingStatus    : %u\n\
transmitArraySizeUsed : %0.2f deg\n\
receiveArraySizeUsed  : %0.2f deg\n\
transmitPower         : %0.2f dB\n\
SLrampUpTimeRemaining : %u %%\n\
yawAngle              : %0.2f deg\n\
numTxSectors          : %u\n\
numBytesPerTxSector   : %u Bytes\n\
headingVessel         : %0.2f deg\n\
soundSpeedAtTxDepth   : %0.2f m/s\n\
txTransducerDepth     : %0.3f m\n\
z_waterLevelReRefPoint: %0.3f m\n\
x_kmallToall          : %0.3f m\n\
y_kmallToall          : %0.3f m\n", d->datagram.mrz.pingInfo->pipeTrackingStatus,
                d->datagram.mrz.pingInfo->transmitArraySizeUsed_deg, d->datagram.mrz.pingInfo->receiveArraySizeUsed_deg,
                d->datagram.mrz.pingInfo->transmitPower_dB, d->datagram.mrz.pingInfo->SLrampUpTimeRemaining, d->datagram.mrz.pingInfo->yawAngle_deg,
                d->datagram.mrz.pingInfo->numTxSectors, d->datagram.mrz.pingInfo->numBytesPerTxSector, d->datagram.mrz.pingInfo->headingVessel_deg,
                d->datagram.mrz.pingInfo->soundSpeedAtTxDepth_mPerSec, d->datagram.mrz.pingInfo->txTransducerDepth_m, d->datagram.mrz.pingInfo->z_waterLevelReRefPoint_m,
                d->datagram.mrz.pingInfo->x_kmallToall_m, d->datagram.mrz.pingInfo->y_kmallToall_m);

            fputs ("\
latLongInfo           : ", fp);

            switch (d->datagram.mrz.pingInfo->latLongInfo) {
                case 0 : fputs ("Last Position Received\n", fp); break;
                case 1 : fputs ("Interpolated\n", fp); break;
                case 2 : fputs ("Processed\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->latLongInfo); break;
            }

            fputs ("\
posSensorStatus       : ", fp);

            switch (d->datagram.mrz.pingInfo->posSensorStatus) {
                case 0 : fputs ("Valid Data\n", fp); break;
                case 1 : fputs ("Invalid Data\n", fp); break;
                case 2 : fputs ("Reduced Performance\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->posSensorStatus); break;
            }

            fputs ("\
attitudeSensorStatus  : ", fp);

            switch (d->datagram.mrz.pingInfo->attitudeSensorStatus) {
                case 0 : fputs ("Valid Data\n", fp); break;
                case 1 : fputs ("Invalid Data\n", fp); break;
                case 2 : fputs ("Reduced Performance\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mrz.pingInfo->attitudeSensorStatus); break;
            }

            fprintf (fp, "\
latitude              : %0.8f deg\n\
longitude             : %0.8f deg\n\
ellipsoidHeightReRefPt: %0.3f m\n", d->datagram.mrz.pingInfo->latitude_deg,
                d->datagram.mrz.pingInfo->longitude_deg, d->datagram.mrz.pingInfo->ellipsoidHeightReRefPoint_m);

            if (d->header.dgmVersion >= 1) {
                fprintf (fp, "\
bsCorrectionOffset    : %0.1f dB\n\
lambertsLawApplied    : %u\n\
iceWindow             : %u\n\
activeModes           : %u\n", d->datagram.mrz.pingInfo->bsCorrectionOffset_dB, d->datagram.mrz.pingInfo->lambertsLawApplied,
                    d->datagram.mrz.pingInfo->iceWindow, d->datagram.mrz.pingInfo->activeModes);
            }

            if (d->datagram.mrz.txSectorInfo.v0) {

                for (i=0; i<d->datagram.mrz.pingInfo->numTxSectors; i++) {

                    fprintf (fp, "\
--- TX Sector [ %lu ] ---\n\
txSectorNum           : %u\n\
txArrNum              : %u\n", (unsigned long) i, d->datagram.mrz.txSectorInfo.v0[i].txSectorNum, d->datagram.mrz.txSectorInfo.v0[i].txArrNum);

                    fputs ("\
txSubArray            : ", fp);

                    switch (d->datagram.mrz.txSectorInfo.v0[i].txSubArray) {
                        case 0 : fputs ("Port\n", fp); break;
                        case 1 : fputs ("Middle\n", fp); break;
                        case 2 : fputs ("Starboard\n", fp); break;
                        default : fprintf (fp, "%u\n", d->datagram.mrz.txSectorInfo.v0[i].txSubArray); break;
                    }

                    fprintf (fp, "\
sectorTransmitDelay   : %0.5f s\n\
tiltAngleReTx         : %0.2f deg\n\
txNominalSourceLevel  : %0.2f dB\n\
txFocusRange          : %0.2f m\n\
centreFreq            : %0.3f Hz\n\
signalBandWidth       : %0.3f Hz\n\
totalSignalLength     : %0.5f s\n\
pulseShading          : %u %%\n", d->datagram.mrz.txSectorInfo.v0[i].sectorTransmitDelay_sec, d->datagram.mrz.txSectorInfo.v0[i].tiltAngleReTx_deg,
                        d->datagram.mrz.txSectorInfo.v0[i].txNominalSourceLevel_dB, d->datagram.mrz.txSectorInfo.v0[i].txFocusRange_m,
                        d->datagram.mrz.txSectorInfo.v0[i].centreFreq_Hz, d->datagram.mrz.txSectorInfo.v0[i].signalBandWidth_Hz,
                        d->datagram.mrz.txSectorInfo.v0[i].totalSignalLength_sec, d->datagram.mrz.txSectorInfo.v0[i].pulseShading);

                    fputs ("\
signalWaveForm        : ", fp);

                    switch (d->datagram.mrz.txSectorInfo.v0[i].signalWaveForm) {
                        case KMA_PULSE_TYPE_CW      : fputs ("CW\n", fp); break;
                        case KMA_PULSE_TYPE_FM_UP   : fputs ("FM Upsweep\n", fp); break;
                        case KMA_PULSE_TYPE_FM_DOWN : fputs ("FM Downsweep\n", fp); break;
                        default : fprintf (fp, "%u\n", d->datagram.mrz.txSectorInfo.v0[i].signalWaveForm); break;
                    }
                }

            } else if (d->datagram.mrz.txSectorInfo.v1) {

                for (i=0; i<d->datagram.mrz.pingInfo->numTxSectors; i++) {

                    fprintf (fp, "\
--- TX Sector [ %lu ] ---\n\
txSectorNum           : %u\n\
txArrNum              : %u\n", (unsigned long) i, d->datagram.mrz.txSectorInfo.v1[i].txSectorNum, d->datagram.mrz.txSectorInfo.v1[i].txArrNum);

                    fputs ("\
txSubArray            : ", fp);

                    switch (d->datagram.mrz.txSectorInfo.v1[i].txSubArray) {
                        case 0 : fputs ("Port\n", fp); break;
                        case 1 : fputs ("Middle\n", fp); break;
                        case 2 : fputs ("Starboard\n", fp); break;
                        default : fprintf (fp, "%u\n", d->datagram.mrz.txSectorInfo.v1[i].txSubArray); break;
                    }

                    fprintf (fp, "\
sectorTransmitDelay   : %0.5f s\n\
tiltAngleReTx         : %0.2f deg\n\
txNominalSourceLevel  : %0.2f dB\n\
txFocusRange          : %0.2f m\n\
centreFreq            : %0.1f Hz\n\
signalBandWidth       : %0.1f Hz\n\
totalSignalLength     : %0.5f s\n\
pulseShading          : %u %%\n", d->datagram.mrz.txSectorInfo.v1[i].sectorTransmitDelay_sec, d->datagram.mrz.txSectorInfo.v1[i].tiltAngleReTx_deg,
                        d->datagram.mrz.txSectorInfo.v1[i].txNominalSourceLevel_dB, d->datagram.mrz.txSectorInfo.v1[i].txFocusRange_m,
                        d->datagram.mrz.txSectorInfo.v1[i].centreFreq_Hz, d->datagram.mrz.txSectorInfo.v1[i].signalBandWidth_Hz,
                        d->datagram.mrz.txSectorInfo.v1[i].totalSignalLength_sec, d->datagram.mrz.txSectorInfo.v1[i].pulseShading);

                    fputs ("\
signalWaveForm        : ", fp);

                    switch (d->datagram.mrz.txSectorInfo.v1[i].signalWaveForm) {
                        case KMA_PULSE_TYPE_CW      : fputs ("CW\n", fp); break;
                        case KMA_PULSE_TYPE_FM_UP   : fputs ("FM Upsweep\n", fp); break;
                        case KMA_PULSE_TYPE_FM_DOWN : fputs ("FM Downsweep\n", fp); break;
                        default : fprintf (fp, "%u\n", d->datagram.mrz.txSectorInfo.v1[i].signalWaveForm); break;
                    }

                    /* The following was added in Version 1 (Rev. G). */
                    fprintf (fp, "\
highVoltageLevel      : %0.1f dB\n\
sectorTrackingCorr    : %0.1f dB\n\
effectiveSignalLength : %0.4f s\n", d->datagram.mrz.txSectorInfo.v1[i].highVoltageLevel_dB, d->datagram.mrz.txSectorInfo.v1[i].sectorTrackingCorr_dB,
                        d->datagram.mrz.txSectorInfo.v1[i].effectiveSignalLength_sec);
                }
            }

            fprintf (fp, "\
------- RX Info -------\n\
numBytesRxInfo        : %u Bytes\n\
numSoundingsMaxMain   : %u\n\
numSoundingsValidMain : %u\n\
numBytesPerSounding   : %u Bytes\n\
WCSampleRate          : %0.4f Hz\n\
seabedImageSampleRate : %0.4f Hz\n\
BSnormal              : %0.2f dB\n\
BSoblique             : %0.2f dB\n\
extraDetectAlarmFlag  : %u\n\
numExtraDetections    : %u\n\
numExtraDetectClasses : %u\n\
numBytesPerClass      : %u Bytes\n", d->datagram.mrz.rxInfo->numBytesRxInfo, d->datagram.mrz.rxInfo->numSoundingsMaxMain,
                d->datagram.mrz.rxInfo->numSoundingsValidMain, d->datagram.mrz.rxInfo->numBytesPerSounding,
                d->datagram.mrz.rxInfo->WCSampleRate_Hz, d->datagram.mrz.rxInfo->seabedImageSampleRate_Hz,
                d->datagram.mrz.rxInfo->BSnormal_dB, d->datagram.mrz.rxInfo->BSoblique_dB, d->datagram.mrz.rxInfo->extraDetectionAlarmFlag,
                d->datagram.mrz.rxInfo->numExtraDetections, d->datagram.mrz.rxInfo->numExtraDetectionClasses,
                d->datagram.mrz.rxInfo->numBytesPerClass);

            if (d->datagram.mrz.extraDetInfo) {

                for (i=0; i<d->datagram.mrz.rxInfo->numExtraDetectionClasses; i++) {

                    fprintf (fp, "\
----- Class [ %lu ] -----\n\
numExtraDetInClass    : %u\n\
alarmFlag             : %u\n", (unsigned long) i, d->datagram.mrz.extraDetInfo[i].numExtraDetInClass, d->datagram.mrz.extraDetInfo[i].alarmFlag);
                }
            }

            if ((output_level > 0) && d->datagram.mrz.sounding) {

                for (i=0; i<d->datagram.mrz.rxInfo->numSoundingsMaxMain + d->datagram.mrz.rxInfo->numExtraDetections; i++) {

                    fprintf (fp, "\
--- Sounding [ %lu ] ----\n\
soundingIndex         : %u\n\
txSectorNum           : %u\n", (unsigned long) i, d->datagram.mrz.sounding[i].soundingIndex, d->datagram.mrz.sounding[i].txSectorNum);

                    fputs ("\
detectionType         : ", fp);

                    switch (d->datagram.mrz.sounding[i].detectionType) {
                        case KMA_DETECT_TYPE_NORMAL   : fputs ("Normal\n", fp); break;
                        case KMA_DETECT_TYPE_EXTRA    : fputs ("Extra\n", fp); break;
                        case KMA_DETECT_TYPE_REJECTED : fputs ("Rejected\n", fp); break;
                        default : fprintf (fp, "%u\n", d->datagram.mrz.sounding[i].detectionType); break;
                    }

                    fputs ("\
detectionMethod       : ", fp);

                    switch (d->datagram.mrz.sounding[i].detectionMethod) {
                        case KMA_DETECT_METHOD_NONE      : fputs ("No Valid Detection\n", fp); break;
                        case KMA_DETECT_METHOD_AMPLITUDE : fputs ("Amplitude\n", fp); break;
                        case KMA_DETECT_METHOD_PHASE     : fputs ("Phase\n", fp); break;
                        default : fprintf (fp, "%u\n", d->datagram.mrz.sounding[i].detectionMethod); break;
                    }

                    fprintf (fp, "\
rejectionInfo1        : %u\n\
rejectionInfo2        : %u\n\
postProcessingInfo    : %u\n\
detectionClass        : %u\n\
detectionConfidenceLvl: %u\n\
rangeFactor           : %0.1f %%\n\
qualityFactor         : %0.2f %%\n\
detectUncertaintyVer  : %0.3f m\n\
detectUncertaintyHor  : %0.3f m\n\
detectWindowLength    : %0.5f s\n\
echoLength            : %0.5f s\n\
WCBeamNum             : %u\n\
WCrange               : %u\n\
WCNomBeamAngleAcross  : %0.3f deg\n\
meanAbsCoeff          : %0.3f dB/km\n\
reflectivity1         : %0.3f dB\n\
reflectivity2         : %0.3f dB\n\
rxSensitivityApplied  : %0.3f dB\n\
sourceLevelApplied    : %0.3f dB\n\
BScalibration         : %0.3f dB\n\
TVG                   : %0.3f dB\n\
beamAngleReRx         : %0.3f deg\n\
beamAngleCorrection   : %0.3f deg\n\
twoWayTravelTime      : %0.5f s\n\
twoWayTravelTimeCorr  : %0.5f s\n\
deltaLatitude         : %0.8f deg\n\
deltaLongitude        : %0.8f deg\n\
z_reRefPoint          : %0.3f m\n\
y_reRefPoint          : %0.3f m\n\
x_reRefPoint          : %0.3f m\n\
beamIncAngleAdj       : %0.3f deg\n\
realTimeCleanInfo     : %u\n\
SIstartRange          : %u\n\
SIcentreSample        : %u\n\
SInumSamples          : %u\n", d->datagram.mrz.sounding[i].rejectionInfo1, d->datagram.mrz.sounding[i].rejectionInfo2,
                        d->datagram.mrz.sounding[i].postProcessingInfo, d->datagram.mrz.sounding[i].detectionClass,
                        d->datagram.mrz.sounding[i].detectionConfidenceLevel, d->datagram.mrz.sounding[i].rangeFactor, d->datagram.mrz.sounding[i].qualityFactor,
                        d->datagram.mrz.sounding[i].detectionUncertaintyVer_m, d->datagram.mrz.sounding[i].detectionUncertaintyHor_m,
                        d->datagram.mrz.sounding[i].detectionWindowLength_sec, d->datagram.mrz.sounding[i].echoLength_sec, d->datagram.mrz.sounding[i].WCBeamNum,
                        d->datagram.mrz.sounding[i].WCrange_samples, d->datagram.mrz.sounding[i].WCNomBeamAngleAcross_deg, d->datagram.mrz.sounding[i].meanAbsCoeff_dBPerkm,
                        d->datagram.mrz.sounding[i].reflectivity1_dB, d->datagram.mrz.sounding[i].reflectivity2_dB, d->datagram.mrz.sounding[i].receiverSensitivityApplied_dB,
                        d->datagram.mrz.sounding[i].sourceLevelApplied_dB, d->datagram.mrz.sounding[i].BScalibration_dB, d->datagram.mrz.sounding[i].TVG_dB,
                        d->datagram.mrz.sounding[i].beamAngleReRx_deg, d->datagram.mrz.sounding[i].beamAngleCorrection_deg, d->datagram.mrz.sounding[i].twoWayTravelTime_sec,
                        d->datagram.mrz.sounding[i].twoWayTravelTimeCorrection_sec, d->datagram.mrz.sounding[i].deltaLatitude_deg, d->datagram.mrz.sounding[i].deltaLongitude_deg,
                        d->datagram.mrz.sounding[i].z_reRefPoint_m, d->datagram.mrz.sounding[i].y_reRefPoint_m, d->datagram.mrz.sounding[i].x_reRefPoint_m,
                        d->datagram.mrz.sounding[i].beamIncAngleAdj_deg, d->datagram.mrz.sounding[i].realTimeCleanInfo, d->datagram.mrz.sounding[i].SIstartRange_samples,
                        d->datagram.mrz.sounding[i].SIcentreSample, d->datagram.mrz.sounding[i].SInumSamples);
                }
            }

            if ((output_level > 1) && d->datagram.mrz.sounding) {

                size_t k = 0;

                fputs ("\
-----------------------\n\
sample (dB)           :\n", fp);

                for (i=0; i<d->datagram.mrz.rxInfo->numSoundingsMaxMain + d->datagram.mrz.rxInfo->numExtraDetections; i++) {

                    size_t j;

                    for (j=0; j<d->datagram.mrz.sounding[i].SInumSamples; j++) {

                        fprintf (fp, "%0.1f ", d->datagram.mrz.sample[k] / 10.0);

                        k++;
                    }

                    fputc ('\n', fp);
                }
            }

            break;

        case KMA_DATAGRAM_MWC :

            fprintf (fp, "\
numOfDgms             : %u\n\
dgmNum                : %u\n", d->datagram.mwc.partition->numOfDgms, d->datagram.mwc.partition->dgmNum);

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
pingCnt               : %u\n\
rxFansPerPing         : %u\n\
rxFanIndex            : %u\n\
swathsPerPing         : %u\n\
swathAlongPosition    : %u\n\
txTransducerInd       : %u\n\
rxTransducerInd       : %u\n\
numRxTransducers      : %u\n\
algorithmType         : %u\n", d->datagram.mwc.common->numBytesCmnPart, d->datagram.mwc.common->pingCnt,
                d->datagram.mwc.common->rxFansPerPing, d->datagram.mwc.common->rxFanIndex, d->datagram.mwc.common->swathsPerPing,
                d->datagram.mwc.common->swathAlongPosition, d->datagram.mwc.common->txTransducerInd, d->datagram.mwc.common->rxTransducerInd,
                d->datagram.mwc.common->numRxTransducers, d->datagram.mwc.common->algorithmType);

            fprintf (fp, "\
numBytesTxInfo        : %u Bytes\n\
numTxSectors          : %u\n\
numBytesPerTxSector   : %u Bytes\n\
heave                 : %0.2f m\n", d->datagram.mwc.txInfo->numBytesTxInfo, d->datagram.mwc.txInfo->numTxSectors,
                d->datagram.mwc.txInfo->numBytesPerTxSector, d->datagram.mwc.txInfo->heave_m);

            for (i=0; i<d->datagram.mwc.txInfo->numTxSectors; i++) {

                fprintf (fp, "\
--- TX Sector [ %lu ] ---\n\
tiltAngleReTx         : %0.2f deg\n\
centreFreq            : %0.2f Hz\n\
txBeamWidthAlong      : %0.2f deg\n\
txSectorNum           : %u\n", (unsigned long) i, d->datagram.mwc.txSectorInfo[i].tiltAngleReTx_deg, d->datagram.mwc.txSectorInfo[i].centreFreq_Hz,
                    d->datagram.mwc.txSectorInfo[i].txBeamWidthAlong_deg, d->datagram.mwc.txSectorInfo[i].txSectorNum);
            }

            fprintf (fp, "\
-----------------------\n\
numBytesRxInfo        : %u Bytes\n\
numBeams              : %u\n\
numBytesPerBeamEntry  : %u Bytes\n", d->datagram.mwc.rxInfo->numBytesRxInfo, d->datagram.mwc.rxInfo->numBeams, d->datagram.mwc.rxInfo->numBytesPerBeamEntry);

            fputs ("\
phaseFlag             : ", fp);

            switch (d->datagram.mwc.rxInfo->phaseFlag) {
                case KMA_MWC_RX_PHASE_OFF  : fputs ("Off\n", fp); break;
                case KMA_MWC_RX_PHASE_LOW  : fputs ("Low Resolution\n", fp); break;
                case KMA_MWC_RX_PHASE_HIGH : fputs ("High Resolution\n", fp); break;
                default : fprintf (fp, "%u\n", d->datagram.mwc.rxInfo->phaseFlag); break;
            }

            fprintf (fp, "\
TVGfunctionApplied    : %u\n\
TVGoffset             : %d dB\n\
sampleFreq            : %0.4f Hz\n\
soundVelocity         : %0.2f m/s\n", d->datagram.mwc.rxInfo->TVGfunctionApplied, d->datagram.mwc.rxInfo->TVGoffset_dB,
                d->datagram.mwc.rxInfo->sampleFreq_Hz, d->datagram.mwc.rxInfo->soundVelocity_mPerSec);

            if (output_level > 0) {

                const uint8_t *p = d->datagram.mwc.beamData;
                kma_datagram_mwc_rx_beam mwc_rx_beam;

                for (i=0; i<d->datagram.mwc.rxInfo->numBeams; i++) {

                    p = kma_get_mwc_rx_beam_data (&mwc_rx_beam, p, d->datagram.mwc.rxInfo->phaseFlag, d->datagram.mwc.rxInfo->numBytesPerBeamEntry);
                    if (!p) break;

                    fprintf (fp, "\
----- Beam [ %lu ] ------\n\
beamPointAngReVertical: %0.2f deg\n\
startRangeSampleNum   : %u\n\
detectedRangeInSamples: %u\n\
beamTxSectorNum       : %u\n\
numSamples            : %u\n", (unsigned long) i, mwc_rx_beam.data->beamPointAngReVertical_deg, mwc_rx_beam.data->startRangeSampleNum,
                        mwc_rx_beam.data->detectedRangeInSamples, mwc_rx_beam.data->beamTxSectorNum, mwc_rx_beam.data->numSamples);

                    if (d->header.dgmVersion >= 1) {
                        fprintf (fp, "\
detectedRangeHiRes    : %0.2f\n", mwc_rx_beam.data->detectedRangeInSamplesHighResolution);
                    }

                    if (output_level > 1) {
                        size_t j;

                        if (mwc_rx_beam.sampleAmplitude_05dB) {
                            fputs ("\
sampleAmplitude_05dB  : ", fp);

                            for (j=0; j<mwc_rx_beam.data->numSamples; j++) {
                                fprintf (fp, "%d ", mwc_rx_beam.sampleAmplitude_05dB[j]);
                            }

                            fputc ('\n', fp);
                        }

                        if (mwc_rx_beam.rxBeamPhase1) {
                            fputs ("\
rxBeamPhase1          : ", fp);

                            for (j=0; j<mwc_rx_beam.data->numSamples; j++) {
                                fprintf (fp, "%0.2f ", mwc_rx_beam.rxBeamPhase1[j] * 180.0 / 128.0);
                            }

                            fputc ('\n', fp);
                        }

                        if (mwc_rx_beam.rxBeamPhase2) {
                            fputs ("\
rxBeamPhase2          : ", fp);

                            for (j=0; j<mwc_rx_beam.data->numSamples; j++) {
                                fprintf (fp, "%0.2f ", mwc_rx_beam.rxBeamPhase2[j] * 0.01);
                            }

                            fputc ('\n', fp);
                        }
                    }
                }
            }

            break;

        case KMA_DATAGRAM_SPO :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
sensorSystem          : %u\n\
sensorStatus          : %u\n", d->datagram.spo.common->numBytesCmnPart, d->datagram.spo.common->sensorSystem, d->datagram.spo.common->sensorStatus);

            fprintf (fp, "\
timeFromSensor_sec    : %u\n\
timeFromSensor_nanosec: %u\n\
posFixQuality         : %0.3f m\n\
correctedLat          : %0.8f deg\n\
correctedLong         : %0.8f deg\n\
speedOverGround       : %0.2f m/s\n\
courseOverGround      : %0.2f deg\n\
ellipsoidHeightReRefPt: %0.3f m\n", d->datagram.spo.data->timeFromSensor_sec, d->datagram.spo.data->timeFromSensor_nanosec,
                d->datagram.spo.data->posFixQuality_m, d->datagram.spo.data->correctedLat_deg, d->datagram.spo.data->correctedLong_deg, d->datagram.spo.data->speedOverGround_mPerSec,
                d->datagram.spo.data->courseOverGround_deg, d->datagram.spo.data->ellipsoidHeightReRefPoint_m);

            fputs ("\
dataFromSensor        : ", fp);

            num_bytes = d->header.numBytesDgm - sizeof (kma_datagram_header) - sizeof (uint32_t) - d->datagram.spo.common->numBytesCmnPart - sizeof (kma_datagram_spo_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.spo.dataFromSensor[i] == '\0') break;
                if (d->datagram.spo.dataFromSensor[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.spo.dataFromSensor[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.spo.dataFromSensor[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_SKM :

            fprintf (fp, "\
numBytesInfoPart      : %u Bytes\n\
sensorSystem          : %u\n", d->datagram.skm.info->numBytesInfoPart, d->datagram.skm.info->sensorSystem);

            fprintf (fp, "\
sensorStatus          : %u\n", d->datagram.skm.info->sensorStatus);

            if ((d->datagram.skm.info->sensorStatus & 0x01) != 0) fputs ("sensorStatus          : Active\n", fp);
            if ((d->datagram.skm.info->sensorStatus & 0x03) != 0) fputs ("sensorStatus          : Reduced Performance\n", fp);
            if ((d->datagram.skm.info->sensorStatus & 0x05) != 0) fputs ("sensorStatus          : Invalid Data\n", fp);
            if ((d->datagram.skm.info->sensorStatus & 0x07) != 0) fputs ("sensorStatus          : Velocity calculated by PU\n", fp);

            fputs ("\
sensorInputFormat     : ", fp);

            switch (d->datagram.skm.info->sensorInputFormat) {
                case KMA_SENSOR_KM_BINARY         : fputs ("KM binary\n", fp); break;
                case KMA_SENSOR_EM3000            : fputs ("EM 3000 data\n", fp); break;
                case KMA_SENSOR_SAGEM             : fputs ("Sagem\n", fp); break;
                case KMA_SENSOR_SEAPATH_11        : fputs ("Seapath binary 11\n", fp); break;
                case KMA_SENSOR_SEAPATH_23        : fputs ("Seapath binary 23\n", fp); break;
                case KMA_SENSOR_SEAPATH_26        : fputs ("Seapath binary 26\n", fp); break;
                case KMA_SENSOR_POSMV_GRP102_103  : fputs ("POS M/V GRP 102/103\n", fp); break;
                default: fprintf (fp, "%u\n", d->datagram.skm.info->sensorInputFormat); break;
            }

            fprintf (fp, "\
numSamples            : %u\n\
numBytesPerSample     : %u Bytes\n", d->datagram.skm.info->numSamples, d->datagram.skm.info->numBytesPerSample);

            fprintf (fp, "\
sensorDataContents    : %u\n", d->datagram.skm.info->sensorDataContents);

            if ((d->datagram.skm.info->sensorDataContents & 0x01) != 0) fputs ("sensorDataContents    : Horizontal position and velocity\n", fp);
            if ((d->datagram.skm.info->sensorDataContents & 0x02) != 0) fputs ("sensorDataContents    : Roll and pitch\n", fp);
            if ((d->datagram.skm.info->sensorDataContents & 0x03) != 0) fputs ("sensorDataContents    : Heading\n", fp);
            if ((d->datagram.skm.info->sensorDataContents & 0x04) != 0) fputs ("sensorDataContents    : Heave\n", fp);
            if ((d->datagram.skm.info->sensorDataContents & 0x05) != 0) fputs ("sensorDataContents    : Acceleration\n", fp);
            if ((d->datagram.skm.info->sensorDataContents & 0x06) != 0) fputs ("sensorDataContents    : Delayed heave\n", fp);
            if ((d->datagram.skm.info->sensorDataContents & 0x07) != 0) fputs ("sensorDataContents    : Delayed heave\n", fp);

            if (output_level > 0) {
                for (i=0; i<d->datagram.skm.info->numSamples; i++) {

                    fprintf (fp, "\
--- Sample [ %lu ] ----\n\
dgmType               : %c%c%c%c\n\
numBytesDgm           : %u Bytes\n\
dgmVersion            : %u\n\
time_sec              : %u\n\
time_nanosec          : %u\n\
status                : %u\n\
latitude              : %0.8f deg\n\
longitude             : %0.8f deg\n\
ellipsoidHeight       : %0.3f m\n\
roll                  : %0.2f deg\n\
pitch                 : %0.2f deg\n\
heading               : %0.2f deg\n\
heave                 : %0.2f m\n\
rollRate              : %0.2f deg/s\n\
pitchRate             : %0.2f deg/s\n\
yawRate               : %0.2f deg/s\n\
velNorth              : %0.2f m/s\n\
velEast               : %0.2f m/s\n\
velDown               : %0.2f m/s\n\
latitudeError         : %0.3f m\n\
longitudeError        : %0.3f m\n\
ellipsoidHeightError  : %0.3f m\n\
rollError             : %0.3f deg\n\
pitchError            : %0.3f deg\n\
headingError          : %0.3f deg\n\
heaveError            : %0.3f m\n\
northAcceleration     : %0.3f m/s^2\n\
eastAcceleration      : %0.3f m/s^2\n\
downAcceleration      : %0.3f m/s^2\n", (unsigned long) i, d->datagram.skm.data[i].KMdefault.dgmType[0], d->datagram.skm.data[i].KMdefault.dgmType[1],
                        d->datagram.skm.data[i].KMdefault.dgmType[2], d->datagram.skm.data[i].KMdefault.dgmType[3], d->datagram.skm.data[i].KMdefault.numBytesDgm,
                        d->datagram.skm.data[i].KMdefault.dgmVersion, d->datagram.skm.data[i].KMdefault.time_sec, d->datagram.skm.data[i].KMdefault.time_nanosec,
                        d->datagram.skm.data[i].KMdefault.status, d->datagram.skm.data[i].KMdefault.latitude_deg, d->datagram.skm.data[i].KMdefault.longitude_deg,
                        d->datagram.skm.data[i].KMdefault.ellipsoidHeight_m, d->datagram.skm.data[i].KMdefault.roll_deg, d->datagram.skm.data[i].KMdefault.pitch_deg,
                        d->datagram.skm.data[i].KMdefault.heading_deg, d->datagram.skm.data[i].KMdefault.heave_m, d->datagram.skm.data[i].KMdefault.rollRate_degPerSec,
                        d->datagram.skm.data[i].KMdefault.pitchRate_degPerSec, d->datagram.skm.data[i].KMdefault.yawRate_degPerSec, d->datagram.skm.data[i].KMdefault.velNorth_mPerSec,
                        d->datagram.skm.data[i].KMdefault.velEast_mPerSec, d->datagram.skm.data[i].KMdefault.velDown_mPerSec, d->datagram.skm.data[i].KMdefault.latitudeError_m,
                        d->datagram.skm.data[i].KMdefault.longitudeError_m, d->datagram.skm.data[i].KMdefault.ellipsoidHeightError_m, d->datagram.skm.data[i].KMdefault.rollError_deg,
                        d->datagram.skm.data[i].KMdefault.pitchError_deg, d->datagram.skm.data[i].KMdefault.headingError_deg, d->datagram.skm.data[i].KMdefault.heaveError_m,
                        d->datagram.skm.data[i].KMdefault.northAcceleration_mPerSecSec, d->datagram.skm.data[i].KMdefault.eastAcceleration_mPerSecSec,
                        d->datagram.skm.data[i].KMdefault.downAcceleration_mPerSecSec);

                    fprintf (fp, "\
time_sec              : %u\n\
time_nanosec          : %u\n\
delayedHeave_m        : %0.2f m\n", d->datagram.skm.data[i].delayedHeave.time_sec, d->datagram.skm.data[i].delayedHeave.time_nanosec,
                        d->datagram.skm.data[i].delayedHeave.delayedHeave_m);
                }
            }

            break;

        case KMA_DATAGRAM_SVP :

            fprintf (fp, "\
numBytesInfoPart      : %u Bytes\n\
numSamples            : %u\n\
sensorFormat          : %c%c%c\n\
time_sec              : %u\n\
latitude              : %0.8f deg\n\
longitude             : %0.8f deg\n", d->datagram.svp.info->numBytesInfoPart, d->datagram.svp.info->numSamples,
                d->datagram.svp.info->sensorFormat[0], d->datagram.svp.info->sensorFormat[1],
                d->datagram.svp.info->sensorFormat[2], d->datagram.svp.info->time_sec,
                d->datagram.svp.info->latitude_deg, d->datagram.svp.info->longitude_deg);

            if (output_level > 0) {
                for (i=0; i<d->datagram.svp.info->numSamples; i++) {

                    fprintf (fp, "\
--- SVP Point [ %lu ] ---\n\
depth                 : %0.2f m\n\
soundVelocity         : %0.2f m/s\n\
temp                  : %0.2f deg C\n\
salinity              : %0.2f\n", (unsigned long) i, d->datagram.svp.data[i].depth_m, d->datagram.svp.data[i].soundVelocity_mPerSec,
                        d->datagram.svp.data[i].temp_C, d->datagram.svp.data[i].salinity);
                }
            }

            break;

        case KMA_DATAGRAM_SVT :

            fprintf (fp, "\
numBytesInfoPart      : %u Bytes\n\
sensorStatus          : %u\n\
sensorInputFormat     : %u\n\
numSamples            : %u\n\
numBytesPerSample     : %u Bytes\n\
sensorDataContents    : %u\n\
filterTime            : %0.2f s\n\
soundVelocity_offset  : %0.2f m/s\n", d->datagram.svt.info->numBytesInfoPart, d->datagram.svt.info->sensorStatus,
                d->datagram.svt.info->sensorInputFormat, d->datagram.svt.info->numSamples, d->datagram.svt.info->numBytesPerSample,
                d->datagram.svt.info->sensorDataContents, d->datagram.svt.info->filterTime_sec, d->datagram.svt.info->soundVelocity_offset_mPerSec);

            for (i=0; i<d->datagram.svt.info->numSamples; i++) {

                fprintf (fp, "\
--- SVT Point [ %lu ] ---\n\
time_sec              : %u\n\
time_nanosec          : %u\n\
soundVelocity         : %0.2f m/s\n\
temp                  : %0.2f deg C\n\
pressure              : %0.2f Pa\n\
salinity              : %0.2f g/kg\n", (unsigned long) i, d->datagram.svt.data[i].time_sec, d->datagram.svt.data[i].time_nanosec, d->datagram.svt.data[i].soundVelocity_mPerSec,
                    d->datagram.svt.data[i].temp_C, d->datagram.svt.data[i].pressure_Pa, d->datagram.svt.data[i].salinity);
            }

            break;

        case KMA_DATAGRAM_SCL :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
sensorSystem          : %u\n\
sensorStatus          : %u\n", d->datagram.scl.common->numBytesCmnPart, d->datagram.scl.common->sensorSystem, d->datagram.scl.common->sensorStatus);

            fprintf (fp, "\
offset                : %0.5f s\n\
clockDevPU            : %d ns\n", d->datagram.scl.data->offset_sec, d->datagram.scl.data->clockDevPU_nanosec);

            fputs ("\
dataFromSensor        : ", fp);

            num_bytes = d->header.numBytesDgm - sizeof (kma_datagram_header) - sizeof (uint32_t) - d->datagram.scl.common->numBytesCmnPart - sizeof (kma_datagram_scl_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.scl.dataFromSensor[i] == '\0') break;
                if (d->datagram.scl.dataFromSensor[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.scl.dataFromSensor[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.scl.dataFromSensor[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_SDE :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
sensorSystem          : %u\n\
sensorStatus          : %u\n", d->datagram.sde.common->numBytesCmnPart, d->datagram.sde.common->sensorSystem, d->datagram.sde.common->sensorStatus);

            if (d->header.dgmVersion == 0) {
                fprintf (fp, "\
depthUsed             : %0.2f m\n\
offset                : %0.2f\n\
scale                 : %0.2f\n\
latitude              : %0.8f deg\n\
longitude             : %0.8f deg\n", d->datagram.sde.data.v0->depthUsed_m, d->datagram.sde.data.v0->offset, d->datagram.sde.data.v0->scale,
                    d->datagram.sde.data.v0->latitude_deg, d->datagram.sde.data.v0->longitude_deg);
            } else if (d->header.dgmVersion == 1) {
                fprintf (fp, "\
depthUsed             : %0.2f m\n\
depthRaw              : %0.2f m\n\
offset                : %0.2f\n\
scale                 : %0.2f\n\
latitude              : %0.8f deg\n\
longitude             : %0.8f deg\n", d->datagram.sde.data.v1->depthUsed_m, d->datagram.sde.data.v1->depthRaw_m, d->datagram.sde.data.v1->offset,
                    d->datagram.sde.data.v1->scale, d->datagram.sde.data.v1->latitude_deg, d->datagram.sde.data.v1->longitude_deg);
            }

            fputs ("\
dataFromSensor        : ", fp);

            if (d->header.dgmVersion == 0) {
                num_bytes = d->header.numBytesDgm - sizeof (kma_datagram_header) - sizeof (uint32_t) - d->datagram.sde.common->numBytesCmnPart - sizeof (kma_datagram_sde_data_v0);
            } else {
                num_bytes = d->header.numBytesDgm - sizeof (kma_datagram_header) - sizeof (uint32_t) - d->datagram.sde.common->numBytesCmnPart - sizeof (kma_datagram_sde_data_v1);
            }

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.sde.dataFromSensor[i] == '\0') break;
                if (d->datagram.sde.dataFromSensor[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.sde.dataFromSensor[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.sde.dataFromSensor[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_SHI :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
sensorSystem          : %u\n\
sensorStatus          : %u\n", d->datagram.shi.common->numBytesCmnPart, d->datagram.shi.common->sensorSystem, d->datagram.shi.common->sensorStatus);

            fprintf (fp, "\
sensorType            : %u\n\
heigthUsed            : %0.2f m\n", d->datagram.shi.data->sensorType, d->datagram.shi.data->heigthUsed_m);

            fputs ("\
dataFromSensor        : ", fp);

            num_bytes = d->header.numBytesDgm - sizeof (kma_datagram_header) - sizeof (uint32_t) - d->datagram.shi.common->numBytesCmnPart - sizeof (kma_datagram_shi_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.shi.dataFromSensor[i] == '\0') break;
                if (d->datagram.shi.dataFromSensor[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.shi.dataFromSensor[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.shi.dataFromSensor[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_CPO :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
sensorSystem          : %u\n\
sensorStatus          : %u\n", d->datagram.cpo.common->numBytesCmnPart, d->datagram.cpo.common->sensorSystem, d->datagram.cpo.common->sensorStatus);

            fprintf (fp, "\
timeFromSensor_sec    : %u\n\
timeFromSensor_nanosec: %u\n\
posFixQuality         : %0.3f m\n\
correctedLat          : %0.8f deg\n\
correctedLong         : %0.8f deg\n\
speedOverGround       : %0.2f m/s\n\
courseOverGround      : %0.2f deg\n\
ellipsoidHeightReRefPt: %0.3f m\n", d->datagram.cpo.data->timeFromSensor_sec, d->datagram.cpo.data->timeFromSensor_nanosec,
                d->datagram.cpo.data->posFixQuality_m, d->datagram.cpo.data->correctedLat_deg, d->datagram.cpo.data->correctedLong_deg, d->datagram.cpo.data->speedOverGround_mPerSec,
                d->datagram.cpo.data->courseOverGround_deg, d->datagram.cpo.data->ellipsoidHeightReRefPoint_m);

            fputs ("\
dataFromSensor        : ", fp);

            num_bytes = d->header.numBytesDgm - sizeof (kma_datagram_header) - sizeof (uint32_t) - d->datagram.cpo.common->numBytesCmnPart - sizeof (kma_datagram_cpo_data);

            for (i=0; i<num_bytes; i++) {
                if (d->datagram.cpo.dataFromSensor[i] == '\0') break;
                if (d->datagram.cpo.dataFromSensor[i] == '\n') {
                    fputs ("\\n", fp);
                } else if (d->datagram.cpo.dataFromSensor[i] == '\r') {
                    fputs ("\\r", fp);
                } else {
                    fputc (d->datagram.cpo.dataFromSensor[i], fp);
                }
            }
            fputc ('\n', fp);

            break;

        case KMA_DATAGRAM_CHE :

            fprintf (fp, "\
numBytesCmnPart       : %u Bytes\n\
pingCnt               : %u\n\
rxFansPerPing         : %u\n\
rxFanIndex            : %u\n\
swathsPerPing         : %u\n\
swathAlongPosition    : %u\n\
txTransducerInd       : %u\n\
rxTransducerInd       : %u\n\
numRxTransducers      : %u\n\
algorithmType         : %u\n", d->datagram.che.common->numBytesCmnPart, d->datagram.che.common->pingCnt,
                d->datagram.che.common->rxFansPerPing, d->datagram.che.common->rxFanIndex, d->datagram.che.common->swathsPerPing,
                d->datagram.che.common->swathAlongPosition, d->datagram.che.common->txTransducerInd, d->datagram.che.common->rxTransducerInd,
                d->datagram.che.common->numRxTransducers, d->datagram.che.common->algorithmType);

            fprintf (fp, "\
heave                 : %0.2f m\n", d->datagram.che.data->heave_m);

            break;

        case KMA_DATAGRAM_FCF :

            fprintf (fp, "\
numOfDgms             : %u\n\
dgmNum                : %u\n\
numBytesCmnPart       : %u Bytes\n\
fileStatus            : %d\n\
numBytesFile          : %u Bytes\n\
fileName              : %s\n\
bsCalibrationFile     : \n\
%s\n", d->datagram.fcf.partition->numOfDgms, d->datagram.fcf.partition->dgmNum,
                d->datagram.fcf.common->numBytesCmnPart, d->datagram.fcf.common->fileStatus, d->datagram.fcf.common->numBytesFile,
                d->datagram.fcf.fileName, d->datagram.fcf.bsCalibrationFile);

            break;

        default : break;
    }
}


/******************************************************************************
*
* kma_set_ignore_mwc - Set the boolean to ignore (or skip) watercolumn data
*   while reading data from the file handle H.
*
******************************************************************************/

void kma_set_ignore_mwc (
    kma_handle *h,
    const int ignore_mwc) {

    assert (h);
    h->ignore_mwc = ignore_mwc;
}


/******************************************************************************
*
* kma_set_ignore_mrz - Set the boolean to ignore (or skip) multibeam sounding
*   data while reading data from the file handle H.
*
******************************************************************************/

void kma_set_ignore_mrz (
    kma_handle *h,
    const int ignore_mrz) {

    assert (h);
    h->ignore_mrz = ignore_mrz;
}


/******************************************************************************
*
* kma_get_mwc_rx_beam_data - The MWC RX beam data stored in the .kmall format
*   consists of the kma_datagram_mwc_rx_beam_data struct followed by numSamples
*   of amplitude data and optionally 8-bit or 16-bit phase data.  This function
*   will set the pointers in the Rx beam data object D for the data stream
*   starting at P with PHASE_FLAG.  The start of the next RX beam data stream
*   will be returned.  Note: Check sample and beamphase arrays are not NULL
*   when function returns.
*
* Return: A pointer to the next RX beam data, or
*         NULL if numBytesPerBeamEntry is invalid.
*
******************************************************************************/

const uint8_t * kma_get_mwc_rx_beam_data (
    kma_datagram_mwc_rx_beam *d,
    const uint8_t *p,
    const uint8_t phase_flag,
    const uint8_t numBytesPerBeamEntry) {

    assert (d);
    assert (p);

    d->data = NULL;
    d->sampleAmplitude_05dB = NULL;
    d->rxBeamPhase1 = NULL;
    d->rxBeamPhase2 = NULL;

    if (numBytesPerBeamEntry == 0) {
        cpl_debug (kma_debug, "Invalid numBytesPerBeamEntry\n");
        return NULL;
    }

    d->data = (kma_datagram_mwc_rx_beam_data *) p;
    p = p + numBytesPerBeamEntry;

    if (d->data->numSamples > 0) {
        d->sampleAmplitude_05dB = (int8_t *) p;
        p = p + d->data->numSamples * sizeof (int8_t);

        if (phase_flag == KMA_MWC_RX_PHASE_LOW) {
            d->rxBeamPhase1 = (int8_t *) p;
            p = p + d->data->numSamples * sizeof (int8_t);
        } else if (phase_flag == KMA_MWC_RX_PHASE_HIGH) {
            d->rxBeamPhase2 = (int16_t *) p;
            p = p + d->data->numSamples * sizeof (int16_t);
        }
    }

    return p;
}


/******************************************************************************
*
* kma_get_errno - Return the error number from the last call.
*
******************************************************************************/

int kma_get_errno (
    const kma_handle *h) {

    assert (h);
    return h->kma_errno;
}


/******************************************************************************
*
* kma_identify - Determine if the file given by FILE_NAME is a KMA file.
*
* Return: 1 if the file is a KMA file,
*         0 if the file is not a KMA file, or
*         error condition if an error occurred.
*
* Errors: CS_EOPEN
*         CS_EREAD
*
******************************************************************************/

int kma_identify (
    const char *file_name) {

    kma_datagram_header header;
    size_t actual_read_size;
    size_t read_size;
    ssize_t result;
    int fd;


    if (!file_name) return 0;

    /* TODO: Read and validate several datagram headers instead of just one.  This will
       increase accuracy of id. */

    /* Open the binary file for read-only access or return on failure. */
    fd = cpl_open (file_name, CPL_OPEN_RDONLY | CPL_OPEN_BINARY | CPL_OPEN_SEQUENTIAL);

    if (fd < 0) {
        cpl_debug (kma_debug, "Open failed for file '%s'\n", file_name);
        return CS_EOPEN;
    }

    read_size = sizeof (kma_datagram_header);

    /* Read the datagram header and close the file. */
    result = cpl_read (&header, read_size, fd);
    cpl_close (fd);

    if (result < 0) {
        cpl_debug (kma_debug, "Read error occurred for file '%s'\n", file_name);
        return CS_EREAD;
    }

    actual_read_size = (size_t) result;

    /* Make sure we read at least the minimum size. */
    if (actual_read_size != read_size) {
        cpl_debug (kma_debug, "Unexpected end of file for file '%s'\n", file_name);
        return 0;
    }

    /* Validate the header and return the result. */
    return kma_valid_header (&header);
}


/******************************************************************************
*
* kma_valid_header - Return non-zero if the header HEADER appears to be valid.
*
* Return: 1 if the header is valid,
*         0 otherwise.
*
******************************************************************************/

static int kma_valid_header (
    const kma_datagram_header *header) {

    assert (header);

    /* Datagram size must be at least as large as the header plus 4-byte length field. */
    if (header->numBytesDgm < sizeof (kma_datagram_header) + 4) {
        cpl_debug (kma_debug, "Invalid datagram size (%u)\n", header->numBytesDgm);
        return 0;
    }

    /* Make sure the datagram size is not unreasonably large, but be generous. */
    if (header->numBytesDgm > 1<<30) {
        cpl_debug (kma_debug, "Invalid datagram size (%u)\n", header->numBytesDgm);
        return 0;
    }

    /* Verify that the datagram type field starts with '#'. */
    if ((header->dgmType & 0x000000FF) != 0x00000023) {
        cpl_debug (kma_debug, "Invalid datagram magic byte (%u)\n", header->dgmType);
        return 0;
    }

    /* Verify nanosecond field is not out of range. */
    if (header->time_nanosec > 1e9) {
        cpl_debug (kma_debug, "Invalid time field (%u)\n", header->time_nanosec);
        return 0;
    }

    /* We could check the datagram type field against the known datagrams, but could
       never guarantee we don't encounter a new or undocumented datagram type. */

    return 1;
}


/******************************************************************************
*
* kma_set_debug - Set the debugging output level for the KMA reader.
*
******************************************************************************/

void kma_set_debug (
    const int debug) {

    kma_debug = debug;
}


/******************************************************************************
*
* kma_get_datagram_name - Return a character string of the name of the datagram
*   given by DGMTYPE.  This string should not be modified by the caller.
*
******************************************************************************/

const char * kma_get_datagram_name (
    const uint32_t dgmType) {

    const char *s;


    switch (dgmType) {

        case KMA_DATAGRAM_IIP : s = "IIP"; break;
        case KMA_DATAGRAM_IOP : s = "IOP"; break;
        case KMA_DATAGRAM_IBE : s = "IBE"; break;
        case KMA_DATAGRAM_IBR : s = "IBR"; break;
        case KMA_DATAGRAM_IBS : s = "IBS"; break;
        case KMA_DATAGRAM_MRZ : s = "MRZ"; break;
        case KMA_DATAGRAM_MWC : s = "MWC"; break;
        case KMA_DATAGRAM_SPO : s = "SPO"; break;
        case KMA_DATAGRAM_SKM : s = "SKM"; break;
        case KMA_DATAGRAM_SVP : s = "SVP"; break;
        case KMA_DATAGRAM_SVT : s = "SVT"; break;
        case KMA_DATAGRAM_SCL : s = "SCL"; break;
        case KMA_DATAGRAM_SDE : s = "SDE"; break;
        case KMA_DATAGRAM_SHI : s = "SHI"; break;
        case KMA_DATAGRAM_CPO : s = "CPO"; break;
        case KMA_DATAGRAM_CHE : s = "CHE"; break;
        case KMA_DATAGRAM_FCF : s = "FCF"; break;
        default               : s = "UNKNOWN"; break;
    }

    return s;
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
    kma_handle *h,
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
