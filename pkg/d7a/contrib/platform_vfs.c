/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifdef MODULE_VFS
/**
 * @{
 * @ingroup     pkg_d7a
 * @file
 * @brief       Implementation of FS platform abstraction
 *
 * @author      Cortus <software@cortus.com>
 * @}
 */
#include <string.h>

#include "framework/hal/inc/hwsystem.h"

#include "framework/inc/debug.h"
#include "framework/inc/d7ap.h"
#include "framework/inc/fs.h"
#include "framework/inc/ng.h"
#include "framework/inc/version.h"
#include "framework/inc/key.h"

///////////////////////////////////////
#include "vfs.h"
#include "errors.h"
#include "board.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#include "log.h"

///////////////////////////////////////
// The d7a file header is stored in file.
// filename is $mount/d7f.$id
// filebody is [$header][$data]
// where $mount=/|/tmp
//       length(header) = 12
//       length($data) == $header.length == $header.allocated_length
///////////////////////////////////////
static const vfs_mount_t* const d7a_fs[] = {
        [FS_STORAGE_TRANSIENT]  = &ARCH_VFS_STORAGE_TRANSIENT,
        [FS_STORAGE_VOLATILE]   = &ARCH_VFS_STORAGE_VOLATILE,
        [FS_STORAGE_RESTORABLE] = &ARCH_VFS_STORAGE_RESTORABLE,
        [FS_STORAGE_PERMANENT]  = &ARCH_VFS_STORAGE_PERMANENT
};
#define FS_STORAGE_CLASS_NUMOF       (FS_STORAGE_PERMANENT+1)
#define FS_STORAGE_CLASS_NUMOF_CHECK (sizeof(d7a_fs)/sizeof(d7a_fs[0]))

///////////////////////////////////////

/* The magic file allows to check filesystem integrity.
 * FIXME: for now it is just a version string, but it should be crc/sig based.
 * Note: to force fs reinitialization, just do 'vfs rm <magic file>'.
 */
#define D7A_FS_MAGIC        RIOT_VERSION "magic"
#define D7A_FS_MAGIC_LEN    sizeof RIOT_VERSION

#define MAX_FILE_NAME           32          /* mount name included */

/* ALLOW_FORMAT:
 *  0: the fs must be mountable and valid, else error
 *     it reduces the code size by 3,2Kbytes, but might fail on boot
 *  1: the fs is recreated when not moutable or not valid.
 *
 * FORCE_REFORMAT: if (ALLOW_FORMAT==1)
 *  when the fs is moutable but the magic nok,
 *  0: just overwrite the files
 *  1: erase and reformat the whole fs.
 *     cleaner but old uninited files will be lost.
 */
#define ALLOW_FORMAT          1
#define FORCE_REFORMAT        1 && ALLOW_FORMAT

#define FIXME 1
#ifdef  FIXME
/* firmware version custom field
 *  strings are defined in riotbuild.h by pkg/Makefile.include
 *  and padded with default values to fit the static sizes defined in d7ap.h.
 * FIXME: sizes should be dynamic accross the whole d7a pkg (for now used only here).
 */
#define _GIT_SHA1     D7A_GIT_SHA1 "0000001" //"${GIT_SHA1}";
#define _APP_NAME     D7A_APP_NAME "APPD7A"  //"${__APP_BUILD_NAME}";
#define _GIT_SHA1_SZ  D7A_FILE_FIRMWARE_VERSION_GIT_SHA1_SIZE  //sizeof D7A_GIT_SHA1
#define _APP_NAME_SZ  D7A_FILE_FIRMWARE_VERSION_APP_NAME_SIZE  //sizeof D7A_APP_NAME

#ifdef APS_MINICLIB
#warning "FIXME: snprintf NOK on miniclib"
#define snprintf(fn, sz, ...)   sprintf(fn, __VA_ARGS__)
#endif

#define WAIT_is_fs_init_completed()     assert(is_fs_init_completed)    //FIXME: blocking mutex???
#endif /*FIXME*/

#if   defined(MODULE_TMPFS)
#define FS_DESC_STR_TMP  "tmpfs"
#else
#define FS_DESC_STR_TMP  "???"
#endif
#if   defined(MODULE_SPIFFS)
#define FS_DESC_STR_PER  "spiffs"
#elif defined (MODULE_LITTLEFS)
#define FS_DESC_STR_PER  "littlefs"
#else
#define FS_DESC_STR_PER  "???"
#endif
#if   defined(MODULE_MTD_SPI_NOR)
#define FS_DESC_STR_MTD     "mtd_spi_nor"
#else
#define FS_DESC_STR_MTD     "mtd_dummy"
#endif
#define FS_DESC_STR     FS_DESC_STR_TMP "+" FS_DESC_STR_PER " on " FS_DESC_STR_MTD

typedef struct  __attribute__((__packed__))
{
    uint8_t length;
    fs_storage_class_t storage : 2;
    uint8_t _rfu : 6; //FIXME: 'valid' field or invalid storage qualifier?
} file_stat_t;

///////////////////////////////////////

#define D7A_PROTOCOL_VERSION_MAJOR 1
#define D7A_PROTOCOL_VERSION_MINOR 1

#define FS_ENABLE_SSR_FILTER 0 // TODO always enabled? cmake param?

static file_stat_t NGDEF(_file_stat)[FRAMEWORK_FS_FILE_COUNT] = { 0 };
#define file_stat NG(_file_stat)    //TODO replace with vfs_stat (optional) if available.

static bool NGDEF(_is_fs_init_completed) = false;
#define is_fs_init_completed NG(_is_fs_init_completed)      //set in _d7a_verify_magic()

static fs_modified_file_callback_t file_modified_callbacks[FRAMEWORK_FS_FILE_COUNT] = { NULL };

static fs_d7aactp_callback_t d7aactp_callback = NULL;

/* forward internal declarations */
static const char * _file_mount_point(uint8_t file_id, const fs_file_header_t* file_header);
static int _file_name(char* file_name, uint8_t file_id, const fs_file_header_t* file_header);
static int _d7a_init_transient_system_files(fs_init_args_t* init_args);
static int _d7a_init_persistent_system_files(fs_init_args_t* init_args);
static int _d7a_create_magic(void);
static int _d7a_verify_magic(void);
static int _fs_create_file(uint8_t file_id, const fs_file_header_t* file_header, const uint8_t* initial_data);
static alp_status_codes_t _fs_read_file(uint8_t file_id, fs_file_header_t* pfile_header_out, uint8_t offset, uint8_t* buffer, uint8_t length);
static alp_status_codes_t _fs_write_file(uint8_t file_id,fs_file_header_t* pfile_header_out, fs_file_header_t* pfile_header_in, uint8_t offset, const uint8_t* buffer, uint8_t length);
static void _fs_exec_on_write(uint8_t file_id, fs_file_header_t* file_header);
static void _execute_d7a_action_protocol(uint8_t command_file_id, uint8_t interface_file_id);
static void _fs_write_access_class(uint8_t access_class_index, dae_access_profile_t* access_class, bool _exec);



static inline bool is_file_defined(uint8_t file_id)
{
    //return file_stat[file_id].storage == FS_STORAGE_INVALID;
    return file_stat[file_id].length != 0;
}

void fs_init(fs_init_args_t* init_args)
{
    // TODO store as big endian!
    assert(init_args != NULL);
    assert(init_args->access_profiles_count > 0); // there should be at least one access profile defined
    assert(FS_STORAGE_CLASS_NUMOF == FS_STORAGE_CLASS_NUMOF_CHECK);  //try to ensure that fs_storage_class_t enums are mapped to incremental zero based array indexes

    if (is_fs_init_completed)
        return /*0*/;

    //reset the cache (all length to 0)
    memset(file_stat,0,sizeof(file_stat));

    //mount permanent and transient storages
    int res=0;
    for (int i=0; i<FS_STORAGE_CLASS_NUMOF; i++) {
        res = vfs_mount((vfs_mount_t*)d7a_fs[i]);
        if (!(res == 0 || res==-EBUSY)) {
#if (ALLOW_FORMAT==1)
            DEBUG("Error: fs[%d] mount failed (%d). trying format\n",i,res);
            if ((res=vfs_format((vfs_mount_t*)d7a_fs[i]))!=0)
            {
                DEBUG("Error: fs[%d] format failed (%d)\n",i,res);
                /* no return. (re)try mount before failing */
            }
#endif
            if ((res=vfs_mount((vfs_mount_t*)d7a_fs[i])) != 0)
            {
                LOG_ERROR("D7A fs_init ERROR: fs[%d] mount failed (%d)\n",i,res);
                return /*-1*/;
            }
        }
        DEBUG("D7A fs[%d] mounted\n",i);
    }
    _d7a_init_persistent_system_files(init_args);
    _d7a_init_transient_system_files(init_args);
    if (is_fs_init_completed) {
        LOG_INFO("D7A fs_init OK (" FS_DESC_STR ")\n");
    }else{
        LOG_ERROR("D7A fs_init ERROR (" FS_DESC_STR ")\n");
    }
    assert(is_fs_init_completed);
    /*return 0*/;
}

int _d7a_init_transient_system_files(fs_init_args_t* init_args)
{
    //noop
    (void)init_args;
    return 0;
}

int _d7a_init_persistent_system_files(fs_init_args_t* init_args)
{
    _d7a_verify_magic();

    if (is_fs_init_completed)
        return 0;

    LOG_INFO("D7A fs_init: no valid magic, recreating fs...\n");

#if (ALLOW_FORMAT==1)

#if (FORCE_REFORMAT == 1)
    DEBUG("init_persistent_system_files: force format\n");
    int res;
    if ((res=vfs_umount((vfs_mount_t*)d7a_fs[FS_STORAGE_PERMANENT])) != 0)
    {
        DEBUG("init_persistent_system_files: Oops, umount failed (%d)\n", res);
    }
    if ((res=vfs_format((vfs_mount_t*)d7a_fs[FS_STORAGE_PERMANENT])) != 0)
    {
        DEBUG("init_persistent_system_files: Oops, format failed (%d)\n", res);
        return -1;
    }
    if ((res=vfs_mount((vfs_mount_t*)d7a_fs[FS_STORAGE_PERMANENT]))!=0)
    {
        DEBUG("init_persistent_system_files: Oops, remount failed (%d)\n",res);
        return -1;
    }
    DEBUG("init_persistent_system_files: force format complete\n");
#endif

    uint16_t current_data_offset;

#define FRAMEWORK_FS_MAX_FILE_SIZE 256
    uint8_t  data[FRAMEWORK_FS_MAX_FILE_SIZE] = { 0 };
#define RESET_DATABUF(file_sz)  {                           \
        assert(file_sz < FRAMEWORK_FS_MAX_FILE_SIZE);       \
        current_data_offset=0;                              \
        memset(data,0,file_sz);                             \
    }

    fs_file_header_t fh;
#define SET_FILE_HEADER(storage, size)  {                       \
        fh.file_properties.action_protocol_enabled = 0;         \
        fh.file_properties.storage_class = storage;             \
        fh.file_permissions = 0; /*TODO*/                       \
        fh.length           = size;                             \
        fh.allocated_length = size;                             \
    }

    // 0x00 - UID
    uint64_t id    = hw_get_unique_id();
    uint64_t id_be = __builtin_bswap64(id);
    RESET_DATABUF(D7A_FILE_UID_SIZE);
    data[0]=id_be;
    SET_FILE_HEADER(FS_STORAGE_PERMANENT, D7A_FILE_UID_SIZE);
    _fs_create_file(D7A_FILE_UID_FILE_ID, &fh, data);

    // 0x02 - Firmware version
    RESET_DATABUF(D7A_FILE_FIRMWARE_VERSION_SIZE);
    memset(data + current_data_offset, D7A_PROTOCOL_VERSION_MAJOR, 1); current_data_offset++;
    memset(data + current_data_offset, D7A_PROTOCOL_VERSION_MINOR, 1); current_data_offset++;
    memcpy(data + current_data_offset, _APP_NAME, D7A_FILE_FIRMWARE_VERSION_APP_NAME_SIZE);
    current_data_offset += D7A_FILE_FIRMWARE_VERSION_APP_NAME_SIZE;
    memcpy(data + current_data_offset, _GIT_SHA1, D7A_FILE_FIRMWARE_VERSION_GIT_SHA1_SIZE);
    current_data_offset += D7A_FILE_FIRMWARE_VERSION_GIT_SHA1_SIZE;

    SET_FILE_HEADER(FS_STORAGE_PERMANENT, D7A_FILE_FIRMWARE_VERSION_SIZE);
    _fs_create_file(D7A_FILE_FIRMWARE_VERSION_FILE_ID, &fh, data);

    // 0x0A - DLL Configuration
    RESET_DATABUF(D7A_FILE_DLL_CONF_SIZE);
    data[0] = init_args->access_class;      // active access class
    memset(data + 1, 0xFF, 2);              // VID; 0xFFFF means not valid
    SET_FILE_HEADER(FS_STORAGE_RESTORABLE, D7A_FILE_DLL_CONF_SIZE);
    _fs_create_file(D7A_FILE_DLL_CONF_FILE_ID, &fh, data);

    // 0x20-0x2E - Access Profiles
    // must be created before DLL conf ?
#if defined(MODULE_LITTLEFS) && defined(USE_PERIPH_CORTUS_MTD_FPGA_M25P32)
#if ((APS_FPGA_M25P32_SECTOR_COUNT-APS_FPGA_M25P32_BASE_SECTOR) < 22)
    //FIXME: dirty hack...
#warning "***D7A on LITTLEFS***: not enough sector counts for littlefs (1 file/sector+3). Reducing number of ACCESS_PROFILE_COUNT"
#define D7A_FILE_ACCESS_PROFILE_COUNT  4
    LOG_ERROR("***D7A on LITTLEFS***: WARNING: not enough sector counts (%d) (1 file/sector+3). Reducing number of ACCESS_PROFILE_COUNT to %d\n",
              (APS_FPGA_M25P32_SECTOR_COUNT-APS_FPGA_M25P32_BASE_SECTOR),
              D7A_FILE_ACCESS_PROFILE_COUNT);
#endif
#endif
    RESET_DATABUF(D7A_FILE_ACCESS_PROFILE_SIZE);
    for(uint8_t i = 0; i < D7A_FILE_ACCESS_PROFILE_COUNT; i++)
    {
        dae_access_profile_t* access_class;

        // make sure we fill all 15 AP files, either with the supplied AP or by repeating the last one
        if(i < init_args->access_profiles_count)
          access_class = &(init_args->access_profiles[i]);
        else
          access_class = &(init_args->access_profiles[init_args->access_profiles_count - 1]);

        SET_FILE_HEADER(FS_STORAGE_PERMANENT, D7A_FILE_ACCESS_PROFILE_SIZE);
        _fs_create_file(D7A_FILE_ACCESS_PROFILE_ID + i, &fh, NULL);
        _fs_write_access_class(i, access_class, false);
    }

    // 0x0D- Network security
    RESET_DATABUF(D7A_FILE_NWL_SECURITY_SIZE);
    data[0] = PROVISIONED_KEY_COUNTER;

    SET_FILE_HEADER(FS_STORAGE_PERMANENT, D7A_FILE_NWL_SECURITY_SIZE);
    _fs_create_file(D7A_FILE_NWL_SECURITY, &fh, data);

    // 0x0E - Network security key
    RESET_DATABUF(D7A_FILE_NWL_SECURITY_KEY_SIZE);
    memcpy(data, AES128_key, D7A_FILE_NWL_SECURITY_KEY_SIZE);

    SET_FILE_HEADER(FS_STORAGE_PERMANENT, D7A_FILE_NWL_SECURITY_KEY_SIZE);
    _fs_create_file(D7A_FILE_NWL_SECURITY_KEY, &fh, data);

    // 0x0F - Network security state register
    RESET_DATABUF(D7A_FILE_NWL_SECURITY_STATE_REG_SIZE);
    data[0] = init_args->ssr_filter_mode;

    SET_FILE_HEADER(FS_STORAGE_PERMANENT,
                    init_args->ssr_filter_mode & FS_ENABLE_SSR_FILTER ? D7A_FILE_NWL_SECURITY_STATE_REG_SIZE : 1);
    _fs_create_file(D7A_FILE_NWL_SECURITY_STATE_REG, &fh, data);

    // init user files
    if(init_args->fs_user_files_init_cb)
        init_args->fs_user_files_init_cb();
    
    //assert(current_data_offset <= FRAMEWORK_FS_FILESYSTEM_SIZE);
    d7aactp_callback = init_args->fs_d7aactp_cb;

    _d7a_create_magic();
    return 0;

#undef RESET_DATABUF
#undef SET_FILE_HEADER

#else
    return -1;
#endif /*ALLOW_FORMAT*/
}


//TODO: CRC MAGIC
static int _d7a_create_magic(void)
{
    assert(!is_fs_init_completed); // initing files not allowed after fs_init() completed (for now?)

    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    snprintf( fn, MAX_FILE_NAME,   "%s/d7f.magic",
            (d7a_fs[FS_STORAGE_PERMANENT])->mount_point);

    if ((fd = vfs_open(fn, O_CREAT | O_RDWR, 0)) < 0)
    {
        DEBUG("Error opening file magic for create (%s)\n",fn);
        return ALP_STATUS_FILE_ID_NOT_EXISTS;
    }

    /* write the magic */
    rtc = vfs_write(fd, D7A_FS_MAGIC, D7A_FS_MAGIC_LEN);
    if ( (rtc < 0) || ((unsigned)rtc < D7A_FS_MAGIC_LEN) )
    {
        vfs_close(fd);
        DEBUG("Error writing file magic (%d)\n",rtc);
        return ALP_STATUS_UNKNOWN_ERROR;
    }
    vfs_close(fd);

    /* verify */
    return _d7a_verify_magic();
}

static int _d7a_verify_magic(void)
{
    is_fs_init_completed=false;

    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    snprintf( fn, MAX_FILE_NAME,   "%s/d7f.magic",
            (d7a_fs[FS_STORAGE_PERMANENT])->mount_point);

    if ((fd = vfs_open(fn, O_RDONLY, 0)) < 0)
    {
        DEBUG("Error opening file magic for reading (%s)\n",fn);
        return ALP_STATUS_FILE_ID_NOT_EXISTS;
    }

    /* read the magic */
    char magic[D7A_FS_MAGIC_LEN];
    memset(magic,0,D7A_FS_MAGIC_LEN);
    rtc = vfs_read(fd, magic, D7A_FS_MAGIC_LEN);
    if ( (rtc < 0) || ((unsigned)rtc < D7A_FS_MAGIC_LEN) )
    {
        vfs_close(fd);
        DEBUG("Error reading file magic (%d) exp:%ld\n",rtc, D7A_FS_MAGIC_LEN);
        return ALP_STATUS_UNKNOWN_ERROR;
    }
    vfs_close(fd);

    /* compare */
    for (int i=0; i< (int)D7A_FS_MAGIC_LEN; i++) {
        if (magic[i] != D7A_FS_MAGIC[i]) {
            DEBUG("Error magic[%d] incorrect (%d)\n", i, magic[i]);
            return ALP_STATUS_UNKNOWN_ERROR;
        }
    }

    is_fs_init_completed=true;
    return ALP_STATUS_OK;
}


static const char * _file_mount_point(uint8_t file_id, const fs_file_header_t* file_header)
{
    if (file_header) {
        //CREATE
        if (is_file_defined(file_id)) {
            if (file_stat[file_id].storage != file_header->file_properties.storage_class) {
                //FIXME: this should not happen.
                DEBUG("Oops: somebody's trying to change the storage class.... mv m1 m2");
                assert(false);
            }
        } else {
            file_stat[file_id].storage = file_header->file_properties.storage_class;
        }
    } else {
        //READ/WRITE
        //For create, the file_header is known, hence the storage class.
        //For read/write, if the cache entry does not exist, we assume that we
        //try to read/write a persistent file.
        //                       Because after a reboot, all transient files
        //are erased. Hence a read/write of a transient file without a first
        //create (which updates the cache) is an error.
        if (!is_file_defined(file_id)) {
            file_stat[file_id].storage = FS_STORAGE_PERMANENT;
        }
    }
    return (d7a_fs[file_stat[file_id].storage])->mount_point;
}

static int _file_name(char* file_name, uint8_t file_id, const fs_file_header_t* file_header)
{
    memset(file_name,0,MAX_FILE_NAME);

    return snprintf( file_name, MAX_FILE_NAME,   "%s/d7f.%u",
                     _file_mount_point(file_id, file_header),
                     (unsigned)file_id );
}


int _fs_create_file(uint8_t file_id, const fs_file_header_t* file_header, const uint8_t* initial_data)
{
    assert(!is_fs_init_completed); // initing files not allowed after fs_init() completed (for now?)
    assert(file_id < FRAMEWORK_FS_FILE_COUNT);
    assert(file_header != NULL);
    //length do not account sizeof(fs_file_header_t) embedded in file.
    assert(file_header->allocated_length >= file_header->length );
    //assert(current_data_offset + file_header->length <= FRAMEWORK_FS_FILESYSTEM_SIZE);

    file_stat[file_id].length=0;

    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;
    if ((rtc = _file_name(fn, file_id, file_header)) <=0) {
        DEBUG("Error creating fileid=%d name (%d)\n",file_id, rtc);
        return -1;
    }

    if ((fd = vfs_open(fn, O_CREAT | O_RDWR, 0)) < 0) {
        DEBUG("Error creating fileid=%d (%s)\n",file_id, fn);
        return -1;
    }

    if ( (rtc = vfs_write(fd, file_header, sizeof(fs_file_header_t))) != sizeof(fs_file_header_t) ) {
        vfs_close(fd);
        DEBUG("Error writing fileid=%d header (%d)\n",file_id, rtc);
        return -1;
    }

    vfs_close(fd);

    file_stat[file_id].length = file_header->allocated_length;

    if(initial_data != NULL) {
        //write without callback.
        return _fs_write_file(file_id, NULL, NULL, 0, initial_data, file_header->length);
    }
    return 0;
}

void fs_init_file(uint8_t file_id, const fs_file_header_t* file_header, const uint8_t* initial_data)
{
    assert(!is_fs_init_completed); // initing files not allowed after fs_init() completed (for now?)
    assert(file_id < FRAMEWORK_FS_FILE_COUNT);
    assert(file_id >= 0x40); // system files may not be inited
    //assert(current_data_offset + file_header->length <= FRAMEWORK_FS_FILESYSTEM_SIZE);

    _fs_create_file(file_id, file_header, initial_data);
}


static alp_status_codes_t _fs_read_file(uint8_t file_id,
                                 fs_file_header_t* pfile_header_out,
                                 uint8_t offset, uint8_t* buffer, uint8_t length)
{
    //if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS;
    //if(file_stat[file_id].length < offset + length) return ALP_STATUS_UNKNOWN_ERROR;
    // TODO more specific error (wait for spec discussion)

    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    if ((rtc = _file_name(fn, file_id, NULL)) <=0)
    {
        DEBUG("Error creating fileid=%d name (%d)\n",file_id, rtc);
        return ALP_STATUS_UNKNOWN_ERROR;
    }
    
    if ((fd = vfs_open(fn, O_RDONLY, 0)) < 0)
    {
        DEBUG("Error opening fileid=%d for reading (%s)\n",file_id, fn);
        return ALP_STATUS_FILE_ID_NOT_EXISTS;
    }

    /*always read the header and update the cache entry*/
    fs_file_header_t fh;
    if (!pfile_header_out) pfile_header_out = &fh;
    memset(pfile_header_out,0,sizeof(fs_file_header_t));
    rtc = vfs_read(fd, pfile_header_out, sizeof(fs_file_header_t));
    if ( (rtc < 0) || ((unsigned)rtc < sizeof(fs_file_header_t)) )
    {
        vfs_close(fd);
        DEBUG("Error reading fileid=%d header (%d)\n",file_id, rtc);
        return ALP_STATUS_UNKNOWN_ERROR;
    }
    file_stat[file_id].length = pfile_header_out->length;
    //storage is updated in _file_mountpoint
    //file_stat[file_id].storage=pfile_header_out->storage_class;

    if (buffer && length)
    {
        if (offset)
        {
            if ( (rtc = vfs_lseek(fd, offset, SEEK_SET)) != 0 )
            {
                vfs_close(fd);
                DEBUG("Error seeking fileid=%d header (%d)\n",file_id, rtc);
                return ALP_STATUS_UNKNOWN_ERROR;
            }
        }

        memset(buffer,0,length);
        if ( (rtc = vfs_read(fd, buffer, length)) < 0 )
        {
            vfs_close(fd);
            DEBUG("Error reading fileid=%d (%d) data[%d]\n",file_id, rtc, length);
            return ALP_STATUS_UNKNOWN_ERROR;
        }
    }

    vfs_close(fd);

    DEBUG("OK _fs_read_file(%d,%x,%d,%x,%d)\n",file_id, (unsigned)pfile_header_out, offset, (unsigned)buffer, length);
    return ALP_STATUS_OK;
}

alp_status_codes_t fs_read_file(uint8_t file_id, uint8_t offset, uint8_t* buffer, uint8_t length)
{
    WAIT_is_fs_init_completed();
    return _fs_read_file(file_id, NULL, offset, buffer, length);
}

alp_status_codes_t fs_read_file_header(uint8_t file_id, fs_file_header_t* file_header)
{
    WAIT_is_fs_init_completed();
    return _fs_read_file(file_id, file_header, 0, NULL, 0);
}

static alp_status_codes_t _fs_write_file(uint8_t file_id,
                                  fs_file_header_t* pfile_header_out,
                                  fs_file_header_t* pfile_header_in,
                                  uint8_t offset, const uint8_t* buffer, uint8_t length)
{
    //if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS;
    //if(file_stat[file_id].length < offset + length) return ALP_STATUS_UNKNOWN_ERROR;
    // TODO more specific error (wait for spec discussion)

    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;
    if ((rtc = _file_name(fn, file_id, NULL)) <=0)
    {
        DEBUG("Error creating fileid=%d name (%d)\n",file_id, rtc);
        return ALP_STATUS_UNKNOWN_ERROR;
    }

    if ((fd = vfs_open(fn, O_RDWR, 0)) < 0)
    {
        DEBUG("Error opening fileid=%d rdwr (%s)\n",file_id, fn);
        return ALP_STATUS_FILE_ID_NOT_EXISTS;
    }

    /*always read the header and update the cache if possible*/
    if ((pfile_header_out) || (!pfile_header_in))
    {
        fs_file_header_t fh;
        if (!pfile_header_out) pfile_header_out = &fh;

        memset(pfile_header_out,0,sizeof(fs_file_header_t));
        rtc = vfs_read(fd, pfile_header_out, sizeof(fs_file_header_t));
        if ( (rtc<0) || ((unsigned)rtc < sizeof(fs_file_header_t)) )
        {
            vfs_close(fd);
            DEBUG("Error reading fileid=%d header (%d)\n",file_id, rtc);
            return ALP_STATUS_UNKNOWN_ERROR;
        }
        //todo checks here, before read.
        file_stat[file_id].length = pfile_header_out->length;
    }

    if (pfile_header_in)
    {
        if (file_stat[file_id].storage != pfile_header_in->file_properties.storage_class) {
            DEBUG("Oops change of storage_class not supported\n");
            assert(false);
            return ALP_STATUS_UNKNOWN_ERROR;
        }

        if ( (rtc = vfs_lseek(fd, 0, SEEK_SET)) != 0 )
        {
            vfs_close(fd);
            DEBUG("Error seeking fileid=%d header (%d)\n",file_id, rtc);
            return ALP_STATUS_UNKNOWN_ERROR;
        }

        rtc = vfs_write(fd, pfile_header_in, sizeof(fs_file_header_t));
        if ( (rtc<0) || ((unsigned)rtc < sizeof(fs_file_header_t)) )
        {
            vfs_close(fd);
            DEBUG("Error writing fileid=%d header (%d)\n",file_id, rtc);
            return ALP_STATUS_UNKNOWN_ERROR;
        }

        file_stat[file_id].length = pfile_header_in->length;
    }

    if (buffer && length)
    {
        assert(offset+length <= file_stat[file_id].length);
        rtc = vfs_lseek(fd, offset+sizeof(fs_file_header_t), SEEK_SET);
        if ( (rtc<0) || ((unsigned)rtc < offset+sizeof(fs_file_header_t)) )
        {
            vfs_close(fd);
            DEBUG("Error seeking fileid=%d header (%d)\n",file_id, rtc);
            return ALP_STATUS_UNKNOWN_ERROR;
        }
        rtc = vfs_write(fd, buffer, length);
        if ( (rtc<0) || ((unsigned)rtc < length) )
        {
            vfs_close(fd);
            DEBUG("Error writing fileid=%d (%d) data[%d]\n",file_id, rtc,length);
            return ALP_STATUS_UNKNOWN_ERROR;
        }
    }

    vfs_close(fd);

    DEBUG("OK _fs_write_file(%d,%x,%x,%d,%x,%d)\n",
          file_id,(unsigned)pfile_header_out, (unsigned)pfile_header_in, offset, (unsigned)buffer, length);
    return ALP_STATUS_OK;
}

static void _execute_d7a_action_protocol(uint8_t command_file_id, uint8_t interface_file_id)
{

    assert(is_file_defined(command_file_id));
    // TODO interface_file_id is optional, how do we code this in file header?
    // for now we assume it's always used
    assert(is_file_defined(interface_file_id));

#define FS_MAX_INTERFACE_FILE_SIZE  16
    assert(FS_MAX_INTERFACE_FILE_SIZE >= file_stat[interface_file_id].length);
    assert(FS_MAX_INTERFACE_FILE_SIZE >= file_stat[command_file_id].length);

    uint8_t buffer[FS_MAX_INTERFACE_FILE_SIZE];

    memset(buffer, 0, FS_MAX_INTERFACE_FILE_SIZE);
    fs_read_file(interface_file_id, 0, buffer, file_stat[interface_file_id].length);

    uint8_t* data_ptr = (uint8_t*)buffer;
    d7ap_session_config_t fifo_config;
    assert((*data_ptr) == ALP_ITF_ID_D7ASP); // only D7ASP supported for now
    data_ptr++;
    // TODO add length field according to spec
    fifo_config.qos.raw = (*data_ptr); data_ptr++;
    fifo_config.dormant_timeout = (*data_ptr); data_ptr++;;
    // TODO add Te field according to spec
    fifo_config.addressee.ctrl.raw = (*data_ptr); data_ptr++;
    fifo_config.addressee.access_class = (*data_ptr); data_ptr++;
    memcpy(&(fifo_config.addressee.id), data_ptr, 8); data_ptr += 8; // TODO assume 8 for now

    if(d7aactp_callback)
    {
        memset(buffer, 0, FS_MAX_INTERFACE_FILE_SIZE);
        fs_read_file(command_file_id, 0, buffer, file_stat[command_file_id].length);
        d7aactp_callback(&fifo_config,
                         (uint8_t*)buffer,
                         file_stat[command_file_id].length);
    }
}

static void _fs_exec_on_write(uint8_t file_id, fs_file_header_t* file_header)
{
    assert(file_header != NULL);
    if(file_header->file_properties.action_protocol_enabled == true
       && file_header->file_properties.action_condition == ALP_ACT_COND_WRITE) // TODO ALP_ACT_COND_WRITEFLUSH?
    {
        _execute_d7a_action_protocol(file_header->alp_cmd_file_id, file_header->interface_file_id);
    }

    if(file_modified_callbacks[file_id])
        file_modified_callbacks[file_id](file_id);
}

alp_status_codes_t fs_write_file(uint8_t file_id, uint8_t offset, const uint8_t* buffer, uint8_t length)
{
    WAIT_is_fs_init_completed();
    alp_status_codes_t rtc;
    fs_file_header_t file_header;
    rtc = _fs_write_file(file_id, &file_header, NULL, offset, buffer, length);
    if (rtc != ALP_STATUS_OK)
        return rtc;

    _fs_exec_on_write(file_id, &file_header);

    return ALP_STATUS_OK;
}

alp_status_codes_t fs_write_file_header(uint8_t file_id, fs_file_header_t* file_header)
{
    //if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS; //TRY PERSISTENT
    assert(file_header != NULL);
    return _fs_write_file(file_id, NULL, file_header, 0, NULL, 0);
}


void fs_read_access_class(uint8_t access_class_index, dae_access_profile_t *access_class)
{
    WAIT_is_fs_init_completed();
    assert(access_class_index < D7A_FILE_ACCESS_PROFILE_COUNT);
    //assert(is_file_defined(D7A_FILE_ACCESS_PROFILE_ID + access_class_index));//TRY PERSISTENT
    //assert(file_stat[D7A_FILE_ACCESS_PROFILE_ID + access_class_index].length <= D7A_FILE_ACCESS_PROFILE_SIZE);
    uint8_t data[D7A_FILE_ACCESS_PROFILE_SIZE];
    memset( data, 0, D7A_FILE_ACCESS_PROFILE_SIZE);
    int rtc = fs_read_file(D7A_FILE_ACCESS_PROFILE_ID + access_class_index, 0, data, D7A_FILE_ACCESS_PROFILE_SIZE);
    if (rtc != ALP_STATUS_OK) {
        memset(access_class, 0, sizeof(dae_access_profile_t));
        DEBUG("Error: read access class %d failed (%d)\n",access_class_index, rtc);
        assert(false);//no rtc...
        return;
    }

    uint8_t* data_ptr = data;
    memcpy(&(access_class->channel_header), data_ptr, 1); data_ptr++;

    for(uint8_t i = 0; i < SUBPROFILES_NB; i++)
    {
        memcpy(&(access_class->subprofiles[i].subband_bitmap), data_ptr, 1); data_ptr++;
        memcpy(&(access_class->subprofiles[i].scan_automation_period), data_ptr, 1); data_ptr++;
    }

    for(uint8_t i = 0; i < SUBBANDS_NB; i++)
    {
        memcpy(&(access_class->subbands[i].channel_index_start), data_ptr, 2); data_ptr += 2;
        memcpy(&(access_class->subbands[i].channel_index_end), data_ptr, 2); data_ptr += 2;
        memcpy(&(access_class->subbands[i].eirp), data_ptr, 1); data_ptr++;
        memcpy(&(access_class->subbands[i].cca), data_ptr, 1); data_ptr++;
        memcpy(&(access_class->subbands[i].duty), data_ptr, 1); data_ptr++;
    }
}

static void _fs_write_access_class(uint8_t access_class_index, dae_access_profile_t* access_class, bool _exec)
{
    assert(access_class_index < D7A_FILE_ACCESS_PROFILE_COUNT);

    uint16_t current_data_offset = 0;
    uint8_t  data[D7A_FILE_ACCESS_PROFILE_SIZE];
    memset(data,0,D7A_FILE_ACCESS_PROFILE_SIZE);

    memcpy(data + current_data_offset, &(access_class->channel_header), 1); current_data_offset++;
    for(uint8_t i = 0; i < SUBPROFILES_NB; i++)
    {
        memcpy(data + current_data_offset, &(access_class->subprofiles[i].subband_bitmap), 1); current_data_offset++;
        memcpy(data + current_data_offset, &(access_class->subprofiles[i].scan_automation_period), 1); current_data_offset++;
    }

    for(uint8_t i = 0; i < SUBBANDS_NB; i++)
    {
        memcpy(data + current_data_offset, &(access_class->subbands[i].channel_index_start), 2); current_data_offset += 2;
        memcpy(data + current_data_offset, &(access_class->subbands[i].channel_index_end), 2); current_data_offset += 2;
        data[current_data_offset] = access_class->subbands[i].eirp; current_data_offset++;
        data[current_data_offset] = access_class->subbands[i].cca; current_data_offset++;
        data[current_data_offset] = access_class->subbands[i].duty; current_data_offset++;
    }

    //write without callback
    fs_file_header_t file_header;
    _fs_write_file(D7A_FILE_ACCESS_PROFILE_ID + access_class_index, &file_header, NULL,
                   0, data, current_data_offset/*sizeof(data)*/);

    if (_exec) {
        _fs_exec_on_write(D7A_FILE_ACCESS_PROFILE_ID + access_class_index, &file_header);
    }
}

void fs_write_access_class(uint8_t access_class_index, dae_access_profile_t* access_class)
{
    WAIT_is_fs_init_completed();
    _fs_write_access_class(access_class_index, access_class, true);
}

uint8_t fs_get_file_length(uint8_t file_id)
{
    WAIT_is_fs_init_completed();
    //FIXME:only if defined?
    //assert(is_file_defined(file_id));
    fs_file_header_t fh;
    if (fs_read_file_header(file_id, &fh) != ALP_STATUS_OK)
    {
        return 0;
    }
    
    return file_stat[file_id].length;
}



/************************* UNCHANGED *************************/


void fs_init_file_with_d7asp_interface_config(uint8_t file_id, const d7ap_session_config_t* fifo_config)
{
    // TODO check file not already defined

    uint8_t alp_command_buffer[40] = { 0 };
    uint8_t* ptr = alp_command_buffer;
    (*ptr) = ALP_ITF_ID_D7ASP; ptr++;
    (*ptr) = fifo_config->qos.raw; ptr++;
    (*ptr) = fifo_config->dormant_timeout; ptr++;
    (*ptr) = fifo_config->addressee.ctrl.raw; ptr++;
    (*ptr) = fifo_config->addressee.access_class; ptr++;
    memcpy(ptr, &(fifo_config->addressee.id), 8); ptr += 8; // TODO assume 8 for now

    uint32_t len = ptr - alp_command_buffer;
    // TODO fixed header implemented here, or should this be configurable by app?
    fs_file_header_t file_header = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = len,
        .allocated_length = len,
    };

    fs_init_file(file_id, &file_header, alp_command_buffer);
}

void fs_init_file_with_D7AActP(uint8_t file_id, const d7ap_session_config_t* fifo_config, const uint8_t* alp_command, const uint8_t alp_command_len)
{
    uint8_t alp_command_buffer[40] = { 0 };
    uint8_t* ptr = alp_command_buffer;
    (*ptr) = ALP_ITF_ID_D7ASP; ptr++;
    (*ptr) = fifo_config->qos.raw; ptr++;
    (*ptr) = fifo_config->dormant_timeout; ptr++;
    (*ptr) = fifo_config->addressee.ctrl.raw; ptr++;
    (*ptr) = fifo_config->addressee.access_class; ptr++;
    memcpy(ptr, &(fifo_config->addressee.id), 8); ptr += 8; // TODO assume 8 for now

    memcpy(ptr, alp_command, alp_command_len); ptr += alp_command_len;

    uint32_t len = ptr - alp_command_buffer;
    // TODO fixed header implemented here, or should this be configurable by app?
    fs_file_header_t action_file_header = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = len,
        .allocated_length = len,
    };

    fs_init_file(file_id, &action_file_header, alp_command_buffer);
}

void fs_read_uid(uint8_t *buffer)
{
    fs_read_file(D7A_FILE_UID_FILE_ID, 0, buffer, D7A_FILE_UID_SIZE);
}

void fs_read_vid(uint8_t *buffer)
{
    fs_read_file(D7A_FILE_DLL_CONF_FILE_ID, 1, buffer, 2);
}

void fs_write_vid(uint8_t* buffer)
{
    fs_write_file(D7A_FILE_DLL_CONF_FILE_ID, 1, buffer, 2);
}

uint8_t fs_read_dll_conf_active_access_class(void)
{
    uint8_t access_class;
    fs_read_file(D7A_FILE_DLL_CONF_FILE_ID, 0, &access_class, 1);
    return access_class;
}

void fs_write_dll_conf_active_access_class(uint8_t access_class)
{
    fs_write_file(D7A_FILE_DLL_CONF_FILE_ID, 0, &access_class, 1);
}

bool fs_register_file_modified_callback(uint8_t file_id, fs_modified_file_callback_t callback)
{
    if(file_modified_callbacks[file_id])
        return false; // already registered
    
    file_modified_callbacks[file_id] = callback;
    return true;
}

#if defined(APS_MINICLIB) && defined(FIXME)
#undef snprintf
#endif

#endif /*MODULE_VFS*/
