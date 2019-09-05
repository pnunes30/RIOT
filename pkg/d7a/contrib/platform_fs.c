/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef MODULE_VFS
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

#include "framework/hal/inc/hwsystem.h"

#include "framework/inc/debug.h"
#include "framework/inc/d7ap.h"
#include "framework/inc/fs.h"
#include "framework/inc/ng.h"
#include "framework/inc/version.h"
#include "framework/inc/key.h"

#define D7A_PROTOCOL_VERSION_MAJOR 1
#define D7A_PROTOCOL_VERSION_MINOR 1

#define FS_ENABLE_SSR_FILTER 0 // TODO always enabled? cmake param?

static fs_file_header_t NGDEF(_file_headers)[FRAMEWORK_FS_FILE_COUNT] = { 0 };
#define file_headers NG(_file_headers)

static uint16_t NGDEF(_current_data_offset); // TODO we are using offset here instead of pointer because NG does not support pointers, fix later when NG is replaced
#define current_data_offset NG(_current_data_offset)

static uint8_t NGDEF(_data)[FRAMEWORK_FS_FILESYSTEM_SIZE] = { 0 };
#define data NG(_data)

static uint16_t NGDEF(_file_offsets)[FRAMEWORK_FS_FILE_COUNT] = { 0 };
#define file_offsets NG(_file_offsets)

static bool NGDEF(_is_fs_init_completed) = false;
#define is_fs_init_completed NG(_is_fs_init_completed)

static fs_modified_file_callback_t file_modified_callbacks[FRAMEWORK_FS_FILE_COUNT] = { NULL };

static fs_d7aactp_callback_t d7aactp_callback = NULL;

static inline bool is_file_defined(uint8_t file_id)
{
    return file_headers[file_id].length != 0;
}

static void execute_d7a_action_protocol(uint8_t command_file_id, uint8_t interface_file_id)
{
    assert(is_file_defined(command_file_id));
    // TODO interface_file_id is optional, how do we code this in file header?
    // for now we assume it's always used
    assert(is_file_defined(interface_file_id));

    uint8_t* data_ptr = (uint8_t*)(data + file_offsets[interface_file_id]);

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
      d7aactp_callback(&fifo_config, (uint8_t*)(data + file_offsets[command_file_id]), file_headers[command_file_id].length);
}


void fs_init(fs_init_args_t* init_args)
{
    // TODO store as big endian!
    if (is_fs_init_completed)
        return;

    current_data_offset = 0;

    assert(init_args != NULL);
    assert(init_args->access_profiles_count > 0); // there should be at least one access profile defined

    // 0x00 - UID
    file_offsets[D7A_FILE_UID_FILE_ID] = current_data_offset;
    file_headers[D7A_FILE_UID_FILE_ID] = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = D7A_FILE_UID_SIZE,
        .allocated_length = D7A_FILE_UID_SIZE
    };

    uint64_t id = hw_get_unique_id();
    uint64_t id_be = __builtin_bswap64(id);
    memcpy(data + current_data_offset, &id_be, D7A_FILE_UID_SIZE);
    current_data_offset += D7A_FILE_UID_SIZE;


    // 0x02 - Firmware version
    file_offsets[D7A_FILE_FIRMWARE_VERSION_FILE_ID] = current_data_offset;
    file_headers[D7A_FILE_FIRMWARE_VERSION_FILE_ID] = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = D7A_FILE_FIRMWARE_VERSION_SIZE,
        .allocated_length = D7A_FILE_FIRMWARE_VERSION_SIZE
    };

    memset(data + current_data_offset, D7A_PROTOCOL_VERSION_MAJOR, 1); current_data_offset++;
    memset(data + current_data_offset, D7A_PROTOCOL_VERSION_MINOR, 1); current_data_offset++;
    memcpy(data + current_data_offset, "APPD7A"/*_APP_NAME*/, D7A_FILE_FIRMWARE_VERSION_APP_NAME_SIZE);
    current_data_offset += D7A_FILE_FIRMWARE_VERSION_APP_NAME_SIZE;
    memcpy(data + current_data_offset, "0000001"/*_GIT_SHA1*/, D7A_FILE_FIRMWARE_VERSION_GIT_SHA1_SIZE);
    current_data_offset += D7A_FILE_FIRMWARE_VERSION_GIT_SHA1_SIZE;

    // 0x0A - DLL Configuration
    file_offsets[D7A_FILE_DLL_CONF_FILE_ID] = current_data_offset;
    file_headers[D7A_FILE_DLL_CONF_FILE_ID] = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_RESTORABLE,
        .file_permissions = 0, // TODO
        .length = D7A_FILE_DLL_CONF_SIZE,
        .allocated_length = D7A_FILE_DLL_CONF_SIZE
    };

    data[current_data_offset] = init_args->access_class; current_data_offset += 1; // active access class
    memset(data + current_data_offset, 0xFF, 2); current_data_offset += 2; // VID; 0xFFFF means not valid

    // 0x20-0x2E - Access Profiles
    for(uint8_t i = 0; i < D7A_FILE_ACCESS_PROFILE_COUNT; i++)
    {
        dae_access_profile_t* access_class;

        // make sure we fill all 15 AP files, either with the supplied AP or by repeating the last one
        if(i < init_args->access_profiles_count)
          access_class = &(init_args->access_profiles[i]);
        else
          access_class = &(init_args->access_profiles[init_args->access_profiles_count - 1]);

        file_offsets[D7A_FILE_ACCESS_PROFILE_ID + i] = current_data_offset;
        fs_write_access_class(i, access_class);
        file_headers[D7A_FILE_ACCESS_PROFILE_ID + i] = (fs_file_header_t){
            .file_properties.action_protocol_enabled = 0,
            .file_properties.storage_class = FS_STORAGE_PERMANENT,
            .file_permissions = 0, // TODO
            .length = D7A_FILE_ACCESS_PROFILE_SIZE,
            .allocated_length = D7A_FILE_ACCESS_PROFILE_SIZE
        };
    }

    // 0x0D- Network security
    file_offsets[D7A_FILE_NWL_SECURITY] = current_data_offset;
    file_headers[D7A_FILE_NWL_SECURITY] = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = D7A_FILE_NWL_SECURITY_SIZE,
        .allocated_length = D7A_FILE_ACCESS_PROFILE_SIZE
    };

    memset(data + current_data_offset, 0, D7A_FILE_NWL_SECURITY_SIZE);
    data[current_data_offset] = PROVISIONED_KEY_COUNTER;

    current_data_offset += D7A_FILE_NWL_SECURITY_SIZE;

    // 0x0E - Network security key
    file_offsets[D7A_FILE_NWL_SECURITY_KEY] = current_data_offset;
    file_headers[D7A_FILE_NWL_SECURITY_KEY] = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = D7A_FILE_NWL_SECURITY_KEY_SIZE,
        .allocated_length = D7A_FILE_NWL_SECURITY_KEY_SIZE
    };

    memcpy(data + current_data_offset, AES128_key, D7A_FILE_NWL_SECURITY_KEY_SIZE);
    current_data_offset += D7A_FILE_NWL_SECURITY_KEY_SIZE;

    // 0x0F - Network security state register
    file_offsets[D7A_FILE_NWL_SECURITY_STATE_REG] = current_data_offset;
    file_headers[D7A_FILE_NWL_SECURITY_STATE_REG] = (fs_file_header_t){
        .file_properties.action_protocol_enabled = 0,
        .file_properties.storage_class = FS_STORAGE_PERMANENT,
        .file_permissions = 0, // TODO
        .length = init_args->ssr_filter_mode & FS_ENABLE_SSR_FILTER ? D7A_FILE_NWL_SECURITY_STATE_REG_SIZE : 1,
        .allocated_length = init_args->ssr_filter_mode & FS_ENABLE_SSR_FILTER ? D7A_FILE_NWL_SECURITY_STATE_REG_SIZE : 1
    };

    data[current_data_offset] = init_args->ssr_filter_mode; current_data_offset++;
    data[current_data_offset] = 0; current_data_offset++;
    if (init_args->ssr_filter_mode & FS_ENABLE_SSR_FILTER)
        current_data_offset += D7A_FILE_NWL_SECURITY_STATE_REG_SIZE - 2;

    // init user files
    if(init_args->fs_user_files_init_cb)
        init_args->fs_user_files_init_cb();

    assert(current_data_offset <= FRAMEWORK_FS_FILESYSTEM_SIZE);
    d7aactp_callback = init_args->fs_d7aactp_cb;
    is_fs_init_completed = true;
}

void fs_init_file(uint8_t file_id, const fs_file_header_t* file_header, const uint8_t* initial_data)
{
    assert(file_header->allocated_length>=file_header->length); //must be set explicitly. no default.
    assert(!is_fs_init_completed); // initing files not allowed after fs_init() completed (for now?)
    assert(file_id < FRAMEWORK_FS_FILE_COUNT);
    assert(file_id >= 0x40); // system files may not be inited
    assert(current_data_offset + file_header->allocated_length <= FRAMEWORK_FS_FILESYSTEM_SIZE);

    file_offsets[file_id] = current_data_offset;
    memcpy(file_headers + file_id, file_header, sizeof(fs_file_header_t));
    memset(data + current_data_offset, 0, file_header->allocated_length);
    current_data_offset += file_header->allocated_length;
    if(initial_data != NULL)
        fs_write_file(file_id, 0, initial_data, file_header->length);
}

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

alp_status_codes_t fs_read_file(uint8_t file_id, uint8_t offset, uint8_t* buffer, uint8_t length)
{
    if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS;
    if(file_headers[file_id].length < offset + length) return ALP_STATUS_UNKNOWN_ERROR; // TODO more specific error (wait for spec discussion)

    memcpy(buffer, data + file_offsets[file_id] + offset, length);
    return ALP_STATUS_OK;
}

alp_status_codes_t fs_read_file_header(uint8_t file_id, fs_file_header_t* file_header)
{
  if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS;

  memcpy(file_header, &file_headers[file_id], sizeof(fs_file_header_t));
  return ALP_STATUS_OK;
}

alp_status_codes_t fs_write_file_header(uint8_t file_id, fs_file_header_t* file_header)
{
  if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS;

  memcpy(&file_headers[file_id], file_header, sizeof(fs_file_header_t));
  return ALP_STATUS_OK;
}

alp_status_codes_t fs_write_file(uint8_t file_id, uint8_t offset, const uint8_t* buffer, uint8_t length)
{
    /* O_TRUNC like: shrink allowed, append nok, length>allocated_length nok.*/ 
    if(!is_file_defined(file_id)) return ALP_STATUS_FILE_ID_NOT_EXISTS;
    if(file_headers[file_id].allocated_length < offset + length) return ALP_STATUS_UNKNOWN_ERROR; // TODO more specific error (wait for spec discussion)

    memcpy(data + file_offsets[file_id] + offset, buffer, length);
    file_headers[file_id].length=length;

    if(file_headers[file_id].file_properties.action_protocol_enabled == true
            && file_headers[file_id].file_properties.action_condition == ALP_ACT_COND_WRITE) // TODO ALP_ACT_COND_WRITEFLUSH?
    {
        execute_d7a_action_protocol(file_headers[file_id].alp_cmd_file_id, file_headers[file_id].interface_file_id);
    }


    if(file_modified_callbacks[file_id])
      file_modified_callbacks[file_id](file_id);

    return ALP_STATUS_OK;
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

void fs_read_access_class(uint8_t access_class_index, dae_access_profile_t *access_class)
{
    assert(access_class_index < 15);
    assert(is_file_defined(D7A_FILE_ACCESS_PROFILE_ID + access_class_index));
    uint8_t* data_ptr = data + file_offsets[D7A_FILE_ACCESS_PROFILE_ID + access_class_index];
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

void fs_write_access_class(uint8_t access_class_index, dae_access_profile_t* access_class)
{
    assert(access_class_index < 15);
    current_data_offset = file_offsets[D7A_FILE_ACCESS_PROFILE_ID + access_class_index];
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

uint8_t fs_get_file_length(uint8_t file_id)
{
  assert(is_file_defined(file_id));
  return file_headers[file_id].length;
}

bool fs_register_file_modified_callback(uint8_t file_id, fs_modified_file_callback_t callback)
{
  if(file_modified_callbacks[file_id])
    return false; // already registered

  file_modified_callbacks[file_id] = callback;
  return true;
}

#endif /*MODULE_VFS*/
