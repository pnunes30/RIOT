/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     pkg_d7a
 * @file
 * @brief       Implementation of FS platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */
#include <string.h>

#include "framework/inc/fs.h"
#include "framework/inc/errors.h"

#include "board.h"
#include "mtd.h"
#include "dae.h"

#define ENABLE_DEBUG (0)
#include "debug.h"


#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length);
#define DPRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DPRINT_DATA(...) log_print_data(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINT_DATA(...)
#endif

const char _GIT_SHA1[] = "${D7A_GIT_SHA1}";
const char _APP_NAME[] = "${APPLICATION}";

#define FS_MTD_DEVICES_COUNT 3

typedef enum
{
    FS_MTD_TYPE_METADATA = FS_BLOCKDEVICE_TYPE_METADATA,
    FS_MTD_TYPE_PERMANENT = FS_BLOCKDEVICE_TYPE_PERMANENT,
    FS_MTD_TYPE_VOLATILE = FS_BLOCKDEVICE_TYPE_VOLATILE
} fs_mtd_types_t;

static fs_file_t files[FRAMEWORK_FS_FILE_COUNT] = { 0 }; // TODO do not keep all file metadata in RAM but use smaller MRU cache to save RAM

static bool is_fs_init_completed = false;  //set in _d7a_verify_magic()

#define IS_SYSTEM_FILE(file_id)         (file_id <= 0x3F)

static fs_modified_file_callback_t file_modified_callbacks[FRAMEWORK_FS_FILE_COUNT] = { NULL }; // TODO limit to lower number so save RAM?

static uint32_t mtd_data_offset[FS_MTD_DEVICES_COUNT] = { 0 };
static mtd_dev_t* mtd[FS_MTD_DEVICES_COUNT] = { 0 };

#ifdef MODULE_VFS
#include "vfs.h"

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

#define FS_STORAGE_CLASS_NUMOF_CHECK (sizeof(d7a_fs)/sizeof(d7a_fs[0]))
#endif


#if !defined(PLATFORM_METADATA_MTD)
extern mtd_dev_t * const mtd0;
#define PLATFORM_METADATA_MTD mtd0
#endif

#if !defined(PLATFORM_PERMANENT_MTD)
extern mtd_dev_t * const mtd1;
#define PLATFORM_PERMANENT_MTD mtd1
#endif

#if !defined(PLATFORM_VOLATILE_MTD)
extern mtd_dev_t * const mtd2;
#define PLATFORM_VOLATILE_MTD mtd2
#endif

/* forward internal declarations */
#ifdef MODULE_VFS
static int _get_file_name(char* file_name, uint8_t file_id);
static int _create_file_name(char* file_name, uint8_t file_id, fs_storage_class_t storage_class);
#endif

static int _fs_init(void);
static int _fs_create_magic(void);
static int _fs_verify_magic(uint8_t* magic_number);
static int _fs_create_file(uint8_t file_id, fs_mtd_types_t mtd_type, const uint8_t* initial_data, uint32_t initial_data_length, uint32_t length);

#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}
#endif

static inline bool _is_file_defined(uint8_t file_id)
{
    //return files[file_id].storage == FS_STORAGE_INVALID;
    return files[file_id].length != 0;
}

static inline uint32_t _get_file_header_address(uint8_t file_id)
{
    return FS_FILE_HEADERS_ADDRESS + (file_id * FS_FILE_HEADER_SIZE);
}

error_t fs_register_block_device(blockdevice_t* block_device, uint8_t bd_index)
{
    (void) block_device;
    (void) bd_index;

    return FAIL;
}

uint32_t fs_get_address(uint8_t file_id) { return files[file_id].addr; }

void fs_init(void)
{
    if (is_fs_init_completed)
        return /*0*/;

    memset(files,0,sizeof(files));

#ifdef MODULE_VFS
    //mount permanent and transient storages
    int res=0;
    for (int i=0; i < FS_STORAGE_CLASS_NUMOF; i++) {
        res = vfs_mount((vfs_mount_t*)d7a_fs[i]);
        if (!(res == 0 || res==-EBUSY)) {
#if (ALLOW_FORMAT==1)
            DPRINT("Error: fs[%d] mount failed (%d). trying format",i,res);
            if ((res = vfs_format((vfs_mount_t*)d7a_fs[i]))!=0)
            {
                DPRINT("Error: fs[%d] format failed (%d)\n",i,res);
                /* no return. (re)try mount before failing */
            }
#endif
            if ((res = vfs_mount((vfs_mount_t*)d7a_fs[i])) != 0)
            {
                DPRINT("D7A fs_init ERROR: fs[%d] mount failed (%d)",i,res);
                return /*-1*/;
            }
        }
        DPRINT("D7A fs[%d] mounted",i);
    }
#endif

    // inject the mandatory blockdevice types from the platform
    // for now, only metadata, permanent and volatile storage are supported
    mtd[FS_MTD_TYPE_METADATA] = PLATFORM_METADATA_MTD;
    mtd[FS_MTD_TYPE_PERMANENT] = PLATFORM_PERMANENT_MTD;
    mtd[FS_MTD_TYPE_VOLATILE] = PLATFORM_VOLATILE_MTD;

    _fs_init();

    is_fs_init_completed = true;
    DPRINT("fs_init OK");
}

int _fs_init(void)
{
    uint8_t expected_magic_number[FS_MAGIC_NUMBER_SIZE] = FS_MAGIC_NUMBER;
    uint32_t number_of_files;
    if (_fs_verify_magic(expected_magic_number) < 0)
    {
        DPRINT("fs_init: no valid magic, recreating fs...");

#ifdef MODULE_VFS

#if (FORCE_REFORMAT == 1)
        DPRINT("init_permanent_systemfiles: force format\n");
        int res;
        if ((res = vfs_umount((vfs_mount_t*)d7a_fs[FS_STORAGE_PERMANENT])) != 0)
        {
            DPRINT("init_permanent_systemfiles: Oops, umount failed (%d)", res);
        }
        if ((res = vfs_format((vfs_mount_t*)d7a_fs[FS_STORAGE_PERMANENT])) != 0)
        {
            DPRINT("init_permanent_systemfiles: Oops, format failed (%d)", res);
            return res;
        }
        if ((res = vfs_mount((vfs_mount_t*)d7a_fs[FS_STORAGE_PERMANENT]))!=0)
        {
            DPRINT("init_permanent_systemfiles: Oops, remount failed (%d)",res);
            return res;
        }
        DPRINT("init_permanent_systemfiles: force format complete");
#endif

#endif

        _fs_create_magic();
        number_of_files = 0;
        mtd_write(mtd[FS_MTD_TYPE_METADATA], (uint8_t*)&number_of_files, FS_NUMBER_OF_FILES_ADDRESS, FS_NUMBER_OF_FILES_SIZE);
        return 0;
   }

    // initialise system file caching
    mtd_read(mtd[FS_MTD_TYPE_METADATA], (uint8_t*)&number_of_files, FS_NUMBER_OF_FILES_ADDRESS, FS_NUMBER_OF_FILES_SIZE);
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
    number_of_files = __builtin_bswap32(number_of_files);
#endif

    assert(number_of_files < FRAMEWORK_FS_FILE_COUNT);
    DPRINT("number_of_files: %ld", number_of_files);
    for(int file_id = 0; file_id < FRAMEWORK_FS_FILE_COUNT; file_id++)
    {
        mtd_read(mtd[FS_MTD_TYPE_METADATA], (uint8_t*)&files[file_id],
                         _get_file_header_address(file_id), FS_FILE_HEADER_SIZE);

#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
        // FS headers are stored in big endian
        files[file_id].addr = __builtin_bswap32(files[file_id].addr);
        files[file_id].length = __builtin_bswap32(files[file_id].length);
#endif

#ifdef MODULE_VFS
        if(_is_file_defined(file_id))
        {
            uint8_t data[files[file_id].length];

            mtd_read(mtd[FS_MTD_DEVICE_TYPE_PERMANENT], data, files[file_id].addr, files[file_id].length);

            fs_storage_class_t storage_class = FS_STORAGE_PERMANENT;
            if(files[file_id].blockdevice_index == FS_MTD_DEVICE_TYPE_VOLATILE)
                storage_class = FS_STORAGE_VOLATILE;

            _fs_create_file(file_id, storage_class, data, files[file_id].length);
        }
 #else
        DPRINT("File %i, bd %i, len %i, addr %i", file_id, files[file_id].blockdevice_index, files[file_id].length, files[file_id].addr);
        if(_is_file_defined(file_id))
        {
            if (files[file_id].blockdevice_index == FS_MTD_TYPE_VOLATILE)
                DPRINT("volatile file (%i) will not be initialized", file_id);
            else
                mtd_data_offset[files[file_id].blockdevice_index] += files[file_id].length;
        }

#endif
    }

    return 0;
}

//TODO: CRC MAGIC
static int _fs_create_magic(void)
{
    assert(!is_fs_init_completed);
    uint8_t magic[] = FS_MAGIC_NUMBER;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    snprintf(fn, MAX_FILE_NAME, "%s/d7f.magic", (d7a_fs[FS_STORAGE_PERMANENT])->mount_point);

    if ((fd = vfs_open(fn, O_CREAT | O_RDWR, 0)) < 0)
    {
        DPRINT("Error opening file magic for create (%s)",fn);
        return -ENOENT;
    }

    /* write the magic */
    rtc = vfs_write(fd, magic, FS_MAGIC_NUMBER_SIZE);
    if ( (rtc < 0) || ((unsigned)rtc < FS_MAGIC_NUMBER_SIZE) )
    {
        vfs_close(fd);
        DPRINT("Error writing file magic (%d)",rtc);
        return rtc;
    }
    vfs_close(fd);
#endif

    /* verify */
    return _fs_verify_magic(magic);
}


/* The magic number allows to check filesystem integrity.*/
static int _fs_verify_magic(uint8_t* expected_magic_number)
{
    is_fs_init_completed = false;

    uint8_t magic_number[FS_MAGIC_NUMBER_SIZE];
    memset(magic_number,0,FS_MAGIC_NUMBER_SIZE);

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    snprintf( fn, MAX_FILE_NAME,   "%s/d7f.magic",
            (d7a_fs[FS_STORAGE_PERMANENT])->mount_point);

    if ((fd = vfs_open(fn, O_RDONLY, 0)) < 0)
    {
        DPRINT("Error opening file magic for reading (%s)",fn);
        return -ENOENT;
    }

    /* read the magic */
    rtc = vfs_read(fd, magic_number, FS_MAGIC_NUMBER_SIZE);
    if ( (rtc < 0) || ((unsigned)rtc < FS_MAGIC_NUMBER_SIZE) )
    {
        vfs_close(fd);
        DPRINT("Error reading file magic (%d) exp:%d",rtc, FS_MAGIC_NUMBER_SIZE);
        return rtc;
    }
    vfs_close(fd);
#else
    mtd_read(mtd[FS_MTD_TYPE_METADATA], magic_number, 0, FS_MAGIC_NUMBER_SIZE);
#endif

    DPRINT("READ MAGIC NUMBER:");
    DPRINT_DATA(magic_number, FS_MAGIC_NUMBER_SIZE);
    if(memcmp(expected_magic_number, magic_number, FS_MAGIC_NUMBER_SIZE) != 0) // if not the FS on EEPROM is not compatible with the current code
        return -EINVAL;

    return 0;
}

#ifdef MODULE_VFS
static int _create_file_name(char* file_name, uint8_t file_id, fs_storage_class_t storage_class)
{
    memset(file_name, 0, MAX_FILE_NAME);

    uint8_t mtd_type = FS_MTD_DEVICE_TYPE_PERMANENT;
    if(storage_class == FS_STORAGE_VOLATILE)
        mtd_type = FS_MTD_DEVICE_TYPE_VOLATILE;

    if (_is_file_defined(file_id)) {
        if (files[file_id].blockdevice_index != mtd_type) {
            //FIXME: this should not happen.
            DPRINT("Oops: somebody's trying to change the storage class.... mv m1 m2");
            assert(false);
        }
    } else {
        files[file_id].blockdevice_index = mtd_type;
    }

    return snprintf( file_name, MAX_FILE_NAME,   "%s/d7f.%u",
                     (d7a_fs[storage_class])->mount_point,
                     (unsigned)file_id );
}

static int _get_file_name(char* file_name, uint8_t file_id)
{
    memset(file_name, 0, MAX_FILE_NAME);

    if (!_is_file_defined(file_id))
    {
        files[file_id].blockdevice_index = FS_STORAGE_PERMANENT;
    }

    fs_storage_class_t storage_class = FS_STORAGE_PERMANENT;
    if(files[file_id].blockdevice_index == FS_MTD_DEVICE_TYPE_VOLATILE)
         storage_class = FS_STORAGE_VOLATILE;

    return snprintf( file_name, MAX_FILE_NAME,   "%s/d7f.%u",
                     (d7a_fs[storage_class])->mount_point,
                     (unsigned)file_id );
}
#endif

int _fs_create_file(uint8_t file_id, fs_mtd_types_t mtd_type, const uint8_t* initial_data, uint32_t initial_data_length, uint32_t length)
{
    if(file_id >= FRAMEWORK_FS_FILE_COUNT)
        return -EBADF;

#ifndef MODULE_VFS // create file during the provisioning
    if (_is_file_defined(file_id))
        return -EEXIST;
#endif

    // update file caching for stat lookup
    files[file_id].blockdevice_index = (uint8_t)mtd_type;
    files[file_id].length = length;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;
    if ((rtc = _create_file_name(fn, file_id, mtd_type)) <=0) {
        DPRINT("Error creating fileid=%d name (%d)",file_id, rtc);
        return -ENOENT;
    }

    DPRINT("creating file Id = %d (%s) ", file_id, fn);

    if ((fd = vfs_open(fn, O_CREAT | O_RDWR, 0)) < 0) {
        DPRINT("Error creating fileid=%d (%s)",file_id, fn);
        return -ENOENT;
    }

    if(initial_data != NULL) {
        if ( (rtc = vfs_write(fd, initial_data, initial_data_length)) != initial_data_length) {
            vfs_close(fd);
            DPRINT("Error writing fileid=%d header (%d)",file_id, rtc);
            return rtc;
        }
    }

    vfs_close(fd);
#else
    if (mtd_type == FS_MTD_TYPE_VOLATILE)
    {
        files[file_id].addr = mtd_data_offset[mtd_type];
        mtd_data_offset[mtd_type] += length;
    }
    else
    {
        files[file_id].addr = mtd_data_offset[mtd_type];

#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
        fs_file_t file_header_big_endian;
        memcpy(&file_header_big_endian, (void*)&files[file_id], sizeof (fs_file_t));
        file_header_big_endian.length = __builtin_bswap32(file_header_big_endian.length);
        file_header_big_endian.addr = __builtin_bswap32(file_header_big_endian.addr);
        mtd_write(mtd[FS_MTD_TYPE_METADATA], (uint8_t*)&file_header_big_endian, _get_file_header_address(file_id), FS_FILE_HEADER_SIZE);
#else
        mtd_write(mtd[FS_MTD_TYPE_METADATA], (uint8_t*)&files[file_id], _get_file_header_address(file_id), FS_FILE_HEADER_SIZE);
#endif

        mtd_data_offset[mtd_type] += length;
    }

    if(initial_data != NULL) {
        uint32_t current_address = files[file_id].addr;
        uint32_t remaining_length = initial_data_length;
        uint8_t* current_data = (uint8_t*)initial_data;
        do {
            /* if we can write the remaining data in one write-block, program the remaining length */
            if((remaining_length <= mtd[mtd_type]->page_size) && ((mtd[mtd_type]->page_size == UINT32_MAX) || ((current_address & (0xFFFFFFFF - (mtd[mtd_type]->page_size - 1))) == ((current_address + remaining_length) & (0xFFFFFFFF - (mtd[mtd_type]->page_size - 1)))))) {
                DPRINT("program remaining length of %i", remaining_length);

                mtd_write(mtd[mtd_type], current_data, current_address, remaining_length);
                remaining_length = 0;
            /* else if this is the starting block, only write until the end of the first write_block */
            } else if(current_address == files[file_id].addr) {
                remaining_length -= mtd[mtd_type]->page_size - (current_address & (mtd[mtd_type]->page_size - 1));

                DPRINT("program initial length of %i - %i = %i at address %i", initial_data_length, remaining_length, initial_data_length - remaining_length, current_address);

                mtd_write(mtd[mtd_type], current_data, current_address, initial_data_length - remaining_length);
                current_data += initial_data_length - remaining_length;
                current_address += initial_data_length - remaining_length;
            /* else this is a block in between, just program the maximum amount of block size */
            } else {
                DPRINT("program write block size of %lu while remaining_length is %i at address %i", (uint32_t)mtd[mtd_type]->page_size, remaining_length, current_address);

                remaining_length -= mtd[mtd_type]->page_size;
                mtd_write(mtd[mtd_type], current_data, current_address, mtd[mtd_type]->page_size);
                current_data += mtd[mtd_type]->page_size;
                current_address += mtd[mtd_type]->page_size;
            }
        } while (remaining_length > 0);
    } else {
        // do not use variable length array to limit stack usage, do in chunks instead
        uint8_t default_data[64];
        memset(default_data, 0xff, 64);
        uint32_t remaining_length = length;
        int i = 0;
        while(remaining_length > 64) {
          mtd_write(mtd[mtd_type], default_data, files[file_id].addr + (i * 64), 64);
          remaining_length -= 64;
          i++;
        }

        mtd_write(mtd[mtd_type], default_data, files[file_id].addr  + (i * 64), remaining_length);
    }
#endif

    DPRINT("fs init file(file_id %d, mtd_type %d, addr %i, length %d)\n",file_id, mtd_type, files[file_id].addr, length);
    return 0;
}

int fs_init_file(uint8_t file_id, fs_blockdevice_types_t mtd_type, const uint8_t* initial_data, uint32_t initial_data_length, uint32_t length)
{
    assert(is_fs_init_completed);
    if(file_id >= FRAMEWORK_FS_FILE_COUNT)
        return -EBADF;
   
    return (_fs_create_file(file_id, (fs_mtd_types_t)mtd_type, initial_data, initial_data_length, length));
}

int fs_read_file(uint8_t file_id, uint32_t offset, uint8_t* buffer, uint32_t length)
{
    if(!_is_file_defined(file_id)) return -ENOENT;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    if ((rtc = _get_file_name(fn, file_id)) <= 0)
    {
        DPRINT("Error creating fileid=%d name (%d)",file_id, rtc);
        return rtc;
    }

    if ((fd = vfs_open(fn, O_RDONLY, 0)) < 0)
    {
        DPRINT("Error opening fileid=%d for reading (%s)",file_id, fn);
        return -ENOENT;
    }

    if (buffer && length)
    {
        if (offset)
        {
            rtc = vfs_lseek(fd, offset, SEEK_SET);
            if ((rtc < 0) || ((unsigned)rtc < offset))
            {
                vfs_close(fd);
                DPRINT("Error seeking fileid=%d header (%d)",file_id, rtc);
                return rtc;
            }
        }

        memset(buffer,0,length);
        if ((rtc = vfs_read(fd, buffer, length)) < 0)
        {
            vfs_close(fd);
            DPRINT("Error reading fileid=%d (%d) data[%d]",file_id, rtc, length);
            return rtc;
        }
    }

    vfs_close(fd);
    DPRINT("fs read_file(file_id %d, offset %lu, addr %lu, length %lu)",file_id, offset, files[file_id].addr, length);
    return 0;
}
#else
    if(mtd[files[file_id].blockdevice_index] == NULL) return -EFAULT;

    if(files[file_id].length < offset + length) return -EINVAL;
    
    DPRINT("fs read_file(file_id %d, offset %d, addr %p, bd %i, length %d)\n",file_id, offset, files[file_id].addr, files[file_id].blockdevice_index, length);
    return mtd_read(mtd[files[file_id].blockdevice_index], buffer, files[file_id].addr + offset, length);
#endif
}

int fs_write_file(uint8_t file_id, uint32_t offset, const uint8_t* buffer, uint32_t length)
{
    if(!_is_file_defined(file_id)) return -ENOENT;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;
    if ((rtc = _get_file_name(fn, file_id)) <= 0)
    {
        DPRINT("Error creating fileid=%d name (%d)\n",file_id, rtc);
        return -ENOENT;
    }

    if ((fd = vfs_open(fn, O_RDWR, 0)) < 0)
    {
        DPRINT("Error opening fileid=%d rdwr (%s)\n",file_id, fn);
        return -ENOENT;
    }

    if (buffer && length)
    {
        rtc = vfs_lseek(fd, offset, SEEK_SET);
        if ((rtc < 0) || ((unsigned)rtc < offset))
        {
            vfs_close(fd);
            DPRINT("Error seeking fileid=%d return (%d)\n",file_id, rtc);
            return rtc;
        }
        rtc = vfs_write(fd, buffer, length);
        if ((rtc < 0) || ((unsigned)rtc < length))
        {
            vfs_close(fd);
            DPRINT("Error writing fileid=%d (%d) data[%u]\n",file_id, rtc,length);
            return rtc;
        }
    }

    vfs_close(fd);
#else
    if(mtd[files[file_id].blockdevice_index] == NULL) return -EFAULT;

    if(files[file_id].length < offset + length) return -ENOBUFS;

    uint32_t current_address = files[file_id].addr + offset;
    uint32_t remaining_length = length;
    uint8_t* current_data = (uint8_t*)buffer;
    fs_mtd_types_t mtd_type = files[file_id].blockdevice_index;

    do {
        // calculate the number of bytes that can be written till the end of the block/page
        // @p current_address + @p count must be inside a page boundary. @p addr can be anywhere
        // but the buffer cannot overlap two pages.

        uint32_t bytes_until_end_of_block = mtd[mtd_type]->page_size - ((current_address /*+ mtd[mtd_type]->offset*/) % mtd[mtd_type]->page_size);
        uint32_t bytes_to_program = remaining_length > bytes_until_end_of_block ? bytes_until_end_of_block : remaining_length;
        DPRINT("Programming %i bytes", bytes_to_program);
        mtd_write(mtd[mtd_type], current_data, current_address, bytes_to_program);
        remaining_length -= bytes_to_program;
        current_data += bytes_to_program;
        current_address += bytes_to_program;

    } while (remaining_length > 0);
#endif

    DPRINT("fs write_file (file_id %d, offset %lu, addr %lu, length %lu)",
           file_id, offset, files[file_id].addr, length);

    if(file_modified_callbacks[file_id])
         file_modified_callbacks[file_id](file_id);

    return 0;
}

fs_file_stat_t *fs_file_stat(uint8_t file_id)
{
    assert(is_fs_init_completed);

    if(file_id >= FRAMEWORK_FS_FILE_COUNT)
        return NULL;

    if (_is_file_defined(file_id))
        return (fs_file_stat_t*)&files[file_id];
    else
        return NULL;
}

bool fs_unregister_file_modified_callback(uint8_t file_id) {
    if(file_modified_callbacks[file_id]) {
        file_modified_callbacks[file_id] = NULL;
        return true;
    } else
        return false;
}

bool fs_register_file_modified_callback(uint8_t file_id, fs_modified_file_callback_t callback)
{
    if(!_is_file_defined(file_id))
        return false;

    if(file_modified_callbacks[file_id])
        return false; // already registered

    file_modified_callbacks[file_id] = callback;
    return true;
}
