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


#include "vfs.h"
#include "board.h"
#include <fcntl.h>

#define ENABLE_DEBUG (1)
#include "debug.h"


#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length);
#define DPRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DPRINT_DATA(...) log_print_data(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINT_DATA(...)
#endif

#define FS_STORAGE_CLASS_NUMOF       (FS_STORAGE_PERMANENT + 1)

const char _GIT_SHA1[] = "${D7A_GIT_SHA1}";
const char _APP_NAME[] = "${APPLICATION}";

typedef struct
{
    uint32_t length;
    fs_storage_class_t storage : 2;
    uint8_t rfu : 6; //FIXME: 'valid' field or invalid storage qualifier?
    uint32_t addr;
} fs_file_t;

static fs_file_t files[FRAMEWORK_FS_FILE_COUNT] = { 0 };

static bool is_fs_init_completed = false;  //set in _d7a_verify_magic()

#define IS_SYSTEM_FILE(file_id)         (file_id <= 0x3F)

static fs_modified_file_callback_t file_modified_callbacks[FRAMEWORK_FS_FILE_COUNT] = { NULL };

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
#else

static uint32_t volatile_data_offset = 0;
static uint32_t permanent_data_offset = 0;

static blockdevice_t* bd[FS_STORAGE_CLASS_NUMOF];

#endif

/* forward internal declarations */
#ifdef MODULE_VFS
//static const char * _file_mount_point(uint8_t file_id, const fs_file_header_t* file_header);
static int _get_file_name(char* file_name, uint8_t file_id);
static int _create_file_name(char* file_name, uint8_t file_id, fs_storage_class_t storage_class);
#endif

static int _fs_init_permanent_systemfiles(fs_systemfiles_t* permanent_systemfiles);
static int _fs_create_magic(fs_storage_class_t storage_class);
static int _fs_verify_magic(fs_storage_class_t storage_class, uint8_t* magic_number);
static int _fs_create_file(uint8_t file_id, fs_storage_class_t storage_class, const uint8_t* initial_data, uint32_t length);

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

void fs_init(fs_systemfiles_t* provisioning)
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

#ifndef MODULE_VFS
    // initialise the blockdevice driver according platform specificities
    // for now, only permanent and volatile storage are supported
    bd[FS_STORAGE_PERMANENT] = PLATFORM_PERMANENT_BD;
    bd[FS_STORAGE_VOLATILE] = PLATFORM_VOLATILE_BD;
#endif

    if (provisioning)
        _fs_init_permanent_systemfiles(provisioning);

    is_fs_init_completed = true;
    DPRINT("fs_init OK");
}

int _fs_init_permanent_systemfiles(fs_systemfiles_t* permanent_systemfiles)
{
    if (_fs_verify_magic(FS_STORAGE_PERMANENT, permanent_systemfiles->magic_number) < 0)
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

        _fs_create_magic(FS_STORAGE_PERMANENT);
   }

#ifdef MODULE_VFS
    for (unsigned int file_id = 0; file_id < permanent_systemfiles->nfiles; file_id++)
    {
        _fs_create_file(file_id, FS_STORAGE_PERMANENT, permanent_systemfiles->files_data + permanent_systemfiles->files_offset[file_id],
                        permanent_systemfiles->files_length[file_id]);
    }
#else
    // initialise system file caching
    for (int file_id = 0; file_id < permanent_systemfiles->nfiles; file_id++)
    {
        files[file_id].storage = FS_STORAGE_PERMANENT;
        files[file_id].length = permanent_systemfiles->files_length[file_id];
        files[file_id].addr = permanent_systemfiles->files_offset[file_id];
    }

    permanent_data_offset = permanent_systemfiles->files_offset[permanent_systemfiles->nfiles - 1] +
                            permanent_systemfiles->files_length[permanent_systemfiles->nfiles - 1];
#endif
    return 0;
}

//TODO: CRC MAGIC
static int _fs_create_magic(fs_storage_class_t storage_class)
{
    assert(!is_fs_init_completed);
    uint8_t magic[] = FS_MAGIC_NUMBER;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    snprintf(fn, MAX_FILE_NAME, "%s/d7f.magic", (d7a_fs[storage_class])->mount_point);

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
    return _fs_verify_magic(storage_class, magic);
}


/* The magic number allows to check filesystem integrity.*/
static int _fs_verify_magic(fs_storage_class_t storage_class, uint8_t* expected_magic_number)
{
    is_fs_init_completed = false;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;

    snprintf( fn, MAX_FILE_NAME,   "%s/d7f.magic",
            (d7a_fs[storage_class])->mount_point);

    if ((fd = vfs_open(fn, O_RDONLY, 0)) < 0)
    {
        DPRINT("Error opening file magic for reading (%s)",fn);
        return -ENOENT;
    }

    /* read the magic */
    char magic[FS_MAGIC_NUMBER_SIZE];
    memset(magic,0,FS_MAGIC_NUMBER_SIZE);
    rtc = vfs_read(fd, magic, FS_MAGIC_NUMBER_SIZE);
    if ( (rtc < 0) || ((unsigned)rtc < FS_MAGIC_NUMBER_SIZE) )
    {
        vfs_close(fd);
        DPRINT("Error reading file magic (%d) exp:%ld",rtc, FS_MAGIC_NUMBER_SIZE);
        return rtc;
    }
    vfs_close(fd);

    /* compare */
    for (int i = 0; i < (int)FS_MAGIC_NUMBER_SIZE; i++) {
        if (magic[i] != expected_magic_number[i]) {
            DPRINT("Error magic[%d] incorrect (%d)", i, magic[i]);
            return -EFAULT;
        }
    }
#else
    uint8_t magic_number[FS_MAGIC_NUMBER_SIZE];
    memset(magic_number,0,FS_MAGIC_NUMBER_SIZE);
    blockdevice_read(bd[storage_class], magic_number, 0, FS_MAGIC_NUMBER_SIZE);
    assert(memcmp(expected_magic_number, magic_number, FS_MAGIC_NUMBER_SIZE) == 0); // if not the FS on EEPROM is not compatible with the current code
#endif

    return 0;
}

#ifdef MODULE_VFS
static int _create_file_name(char* file_name, uint8_t file_id, fs_storage_class_t storage_class)
{
    memset(file_name, 0, MAX_FILE_NAME);

    if (_is_file_defined(file_id)) {
        if (files[file_id].storage != storage_class) {
            //FIXME: this should not happen.
            DPRINT("Oops: somebody's trying to change the storage class.... mv m1 m2");
            assert(false);
        }
    } else {
        files[file_id].storage = storage_class;
    }

    return snprintf( file_name, MAX_FILE_NAME,   "%s/d7f.%u",
                     (d7a_fs[files[file_id].storage])->mount_point,
                     (unsigned)file_id );
}

static int _get_file_name(char* file_name, uint8_t file_id)
{
    memset(file_name, 0, MAX_FILE_NAME);

    if (!_is_file_defined(file_id))
    {
        files[file_id].storage = FS_STORAGE_PERMANENT;
    }

    return snprintf( file_name, MAX_FILE_NAME,   "%s/d7f.%u",
                     (d7a_fs[files[file_id].storage])->mount_point,
                     (unsigned)file_id );
}
#endif

int _fs_create_file(uint8_t file_id, fs_storage_class_t storage_class, const uint8_t* initial_data, uint32_t length)
{
    assert(file_id < FRAMEWORK_FS_FILE_COUNT);

    if (_is_file_defined(file_id))
        return -EEXIST;

#ifdef MODULE_VFS
    char fn[MAX_FILE_NAME];
    int rtc;
    int fd;
    if ((rtc = _create_file_name(fn, file_id, storage_class)) <=0) {
        DPRINT("Error creating fileid=%d name (%d)",file_id, rtc);
        return -ENOENT;
    }

    if ((fd = vfs_open(fn, O_CREAT | O_RDWR, 0)) < 0) {
        DPRINT("Error creating fileid=%d (%s)",file_id, fn);
        return -ENOENT;
    }

    if(initial_data != NULL) {
        if ( (rtc = vfs_write(fd, initial_data, length)) != length ) {
            vfs_close(fd);
            DPRINT("Error writing fileid=%d header (%d)",file_id, rtc);
            return rtc;
        }
    }

    vfs_close(fd);
#else
    // only user files can be created
    assert(file_id >= 0x40);

    if (storage_class == FS_STORAGE_PERMANENT)
    {
        files[file_id].addr = permanent_data_offset;
        permanent_data_offset += length;
    }
    else
    {
        files[file_id].addr = volatile_data_offset;
        volatile_data_offset += length;
    }

    if(initial_data != NULL) {
        blockdevice_program(bd[storage_class], initial_data, files[file_id].addr, length);
    }
    else{
        uint8_t default_data[length];
        memset(default_data, 0xff, length);
        blockdevice_program(bd[storage_class], default_data, files[file_id].addr, length);
    }
#endif

    // update file caching for stat lookup
    files[file_id].storage = storage_class;
    files[file_id].length = length;
    DPRINT("fs init file(file_id %d, storage %d, addr %p, length %d)",file_id, storage_class, files[file_id].addr, length);
    return 0;
}

int fs_init_file(uint8_t file_id, fs_storage_class_t storage_class, const uint8_t* initial_data, uint32_t length)
{
    assert(is_fs_init_completed);
    assert(file_id < FRAMEWORK_FS_FILE_COUNT);
    assert(file_id >= 0x40); // system files may not be inited
    assert(storage_class == FS_STORAGE_VOLATILE || storage_class == FS_STORAGE_PERMANENT); // other options not implemented

    return (_fs_create_file(file_id, storage_class, initial_data, length));
}

int fs_read_file(uint8_t file_id, uint32_t offset, uint8_t* buffer, uint32_t length)
{
    if(!_is_file_defined(file_id)) return -ENOENT;

    if(files[file_id].length < offset + length) return -EINVAL;

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
#else
    fs_storage_class_t storage = files[file_id].storage;
    blockdevice_read(bd[storage], buffer, files[file_id].addr + offset, length);
#endif

    DPRINT("fs read_file(file_id %d, offset %d, addr %p, length %d)",file_id, offset, files[file_id].addr, length);
    return 0;
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
            DPRINT("Error writing fileid=%d (%d) data[%d]\n",file_id, rtc,length);
            return rtc;
        }
    }

    vfs_close(fd);
#else
    if(files[file_id].length < offset + length) return -ENOBUFS;

    fs_storage_class_t storage = files[file_id].storage;
    blockdevice_program(bd[storage], buffer, files[file_id].addr + offset, length);
#endif

    DPRINT("fs write_file (file_id %d, offset %d, addr %p, length %d)",
           file_id, offset, files[file_id].addr, length);

    if(file_modified_callbacks[file_id])
         file_modified_callbacks[file_id](file_id);

    return 0;
}

fs_file_stat_t *fs_file_stat(uint8_t file_id)
{
    assert(is_fs_init_completed);

    assert(file_id < FRAMEWORK_FS_FILE_COUNT);

    if (_is_file_defined(file_id))
        return (fs_file_stat_t*)&files[file_id];
    else
        return NULL;
}

bool fs_register_file_modified_callback(uint8_t file_id, fs_modified_file_callback_t callback)
{
    assert(_is_file_defined(file_id));

    if(file_modified_callbacks[file_id])
        return false; // already registered

    file_modified_callbacks[file_id] = callback;
    return true;
}

