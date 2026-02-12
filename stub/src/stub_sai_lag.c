#include "stub_sai.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_LIST_VALUE_STR_LEN 1000
#define MAX_NUMBER_OF_LAG_MEMBERS 16
#define MAX_NUMBER_OF_LAGS 5


typedef struct _lag_member_db_entry_t {
    bool            is_used;
    sai_object_id_t port_oid;
    sai_object_id_t lag_oid;
} lag_member_db_entry_t;

typedef struct _lag_db_entry_t {
    bool            is_used;
    sai_object_id_t members_ids[MAX_NUMBER_OF_LAG_MEMBERS];
    uint32_t        member_count; 
} lag_db_entry_t;

struct lag_db_t {
    lag_db_entry_t        lags[MAX_NUMBER_OF_LAGS];
    lag_member_db_entry_t members[MAX_NUMBER_OF_LAG_MEMBERS];
} lag_db = {0};

sai_status_t get_lag_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg);

sai_status_t get_lag_member_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg);

static const sai_attribute_entry_t lag_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST, false, false, false, true,
      "List of ports in LAG", SAI_ATTR_VAL_TYPE_OBJLIST },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

static const sai_vendor_attribute_entry_t lag_vendor_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST,
      { false, false, false, true },
      { false, false, false, true },
      get_lag_attribute, (void*) SAI_LAG_ATTR_PORT_LIST,
      NULL, NULL }
};

static const sai_attribute_entry_t lag_member_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID, true, true, false, true,
      "LAG ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_LAG_MEMBER_ATTR_PORT_ID, true, true, false, true,
      "PORT ID", SAI_ATTR_VAL_TYPE_OID },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

static const sai_vendor_attribute_entry_t lag_member_vendor_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*) SAI_LAG_MEMBER_ATTR_LAG_ID,
      NULL, NULL },
    { SAI_LAG_MEMBER_ATTR_PORT_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*) SAI_LAG_MEMBER_ATTR_PORT_ID,
      NULL, NULL }
};

sai_status_t get_lag_member_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg)
{
    sai_status_t status;
    uint32_t     db_index;

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG_MEMBER, &db_index);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG MEMBER DB index.\n");
        return status;
    }

    if (db_index >= MAX_NUMBER_OF_LAG_MEMBERS || !lag_db.members[db_index].is_used) {
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    switch ((int64_t)arg) {
    case SAI_LAG_MEMBER_ATTR_LAG_ID:
        value->oid = lag_db.members[db_index].lag_oid;
        break;
    case SAI_LAG_MEMBER_ATTR_PORT_ID:
        value->oid = lag_db.members[db_index].port_oid;
        break;
    default:
        printf("Got unexpected attribute ID for LAG MEMBER\n");
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t get_lag_attribute(
    _In_ const sai_object_key_t   *key,
    _Inout_ sai_attribute_value_t *value,
    _In_ uint32_t                  attr_index,
    _Inout_ vendor_cache_t        *cache,
    void                          *arg)
{
    sai_status_t status;
    uint32_t     db_index;

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG, &db_index);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB index.\n");
        return status;
    }

    if (db_index >= MAX_NUMBER_OF_LAGS || !lag_db.lags[db_index].is_used) {
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    switch ((int64_t)arg) {
    case SAI_LAG_ATTR_PORT_LIST:
        {
            uint32_t count = 0;
            sai_object_id_t temp_port_list[MAX_NUMBER_OF_LAG_MEMBERS];

            for (uint32_t i = 0; i < MAX_NUMBER_OF_LAG_MEMBERS; i++) {
                sai_object_id_t member_oid = lag_db.lags[db_index].members_ids[i];
                if (member_oid != SAI_NULL_OBJECT_ID) {
                    uint32_t member_idx;
                    if (stub_object_to_type(member_oid, SAI_OBJECT_TYPE_LAG_MEMBER, &member_idx) == SAI_STATUS_SUCCESS) {
                        if (member_idx < MAX_NUMBER_OF_LAG_MEMBERS && lag_db.members[member_idx].is_used) {
                            temp_port_list[count++] = lag_db.members[member_idx].port_oid;
                        }
                    }
                }
            }

            if (value->objlist.count < count) {
            value->objlist.count = count;
            return SAI_STATUS_BUFFER_OVERFLOW;
            }

            status = stub_fill_objlist(temp_port_list, count, &value->objlist);
            if (status != SAI_STATUS_SUCCESS) {
                return status;
            }
        }
        break;
    default:
        printf("Got unexpected attribute ID for LAG\n");
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_sai_create_lag(
    _Out_ sai_object_id_t *lag_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    char list_str[MAX_LIST_VALUE_STR_LEN];
    uint32_t ii = 0;

    status = check_attribs_metadata(attr_count, attr_list, lag_attribs, 
                                    lag_vendor_attribs, SAI_OPERATION_CREATE);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed attributes check for LAG creation\n");
        return status;
    }

    for (; ii < MAX_NUMBER_OF_LAGS; ii++) {
        if (!lag_db.lags[ii].is_used) {
            break;
        }
    }

    if (ii == MAX_NUMBER_OF_LAGS) {
        printf("Cannot create LAG: limit is reached (max %d LAGs)\n", MAX_NUMBER_OF_LAGS);
        return SAI_STATUS_TABLE_FULL;
    }

    uint32_t lag_db_id = ii;

    lag_db.lags[lag_db_id].is_used = true;
    lag_db.lags[lag_db_id].member_count = 0;
    memset(lag_db.lags[lag_db_id].members_ids, 0, sizeof(lag_db.lags[lag_db_id].members_ids));

    status = stub_create_object(SAI_OBJECT_TYPE_LAG, lag_db_id, lag_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot create a LAG OID\n");
        lag_db.lags[lag_db_id].is_used = false;
        return status;
    }

    sai_attr_list_to_str(attr_count, (const sai_attribute_t *)attr_list, 
                         lag_attribs, MAX_LIST_VALUE_STR_LEN, list_str);
    printf("CREATE LAG: 0x%lx (%s)\n", *lag_id, list_str);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_sai_remove_lag(
    _In_ sai_object_id_t lag_id)
{
    sai_status_t status;
    uint32_t lag_db_id;

    status = stub_object_to_type(lag_id, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID.\n");
        return status;
    }

    if (lag_db_id >= MAX_NUMBER_OF_LAGS || !lag_db.lags[lag_db_id].is_used) {
        printf("LAG DB ID is invalid or not in use.\n");
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    if (lag_db.lags[lag_db_id].member_count > 0) {
        printf("Cannot remove LAG 0x%lx: it still has %d members.\n", lag_id, lag_db.lags[lag_db_id].member_count);
        return SAI_STATUS_OBJECT_IN_USE;
    }

    lag_db.lags[lag_db_id].is_used = false;
    memset(lag_db.lags[lag_db_id].members_ids, 0, sizeof(lag_db.lags[lag_db_id].members_ids));
    lag_db.lags[lag_db_id].member_count = 0;

    printf("REMOVE LAG: 0x%lx\n", lag_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_sai_get_lag_attribute(
    _In_ sai_object_id_t lag_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    const sai_object_key_t key = { .object_id = lag_id };
    return sai_get_attributes(&key, NULL, lag_attribs, lag_vendor_attribs, attr_count, attr_list);
}

sai_status_t stub_sai_create_lag_member(
    _Out_ sai_object_id_t *lag_member_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    char list_str[MAX_LIST_VALUE_STR_LEN];
    uint32_t ii = 0;
    
    const sai_attribute_value_t *lag_id_val, *port_id_val;
    uint32_t lag_id_idx, port_id_idx;

    status = check_attribs_metadata(attr_count, attr_list, lag_member_attribs, 
                                    lag_member_vendor_attribs, SAI_OPERATION_CREATE);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed attributes check for LAG MEMBER creation\n");
        return status;
    }

    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_LAG_ID, 
                                 &lag_id_val, &lag_id_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("LAG_ID attribute not found\n");
        return SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
    }

    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_PORT_ID, 
                                 &port_id_val, &port_id_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("PORT_ID attribute not found\n");
        return SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
    }

    uint32_t lag_db_idx;
    status = stub_object_to_type(lag_id_val->oid, SAI_OBJECT_TYPE_LAG, &lag_db_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Invalid LAG OID\n");
        return status;
    }
    
    if (lag_db_idx >= MAX_NUMBER_OF_LAGS || !lag_db.lags[lag_db_idx].is_used) {
        printf("Parent LAG does not exist\n");
        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (lag_db.lags[lag_db_idx].member_count >= MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("LAG is full - cannot add more members (max %d)\n", MAX_NUMBER_OF_LAG_MEMBERS);
        return SAI_STATUS_TABLE_FULL;
    }

    for (uint32_t i = 0; i < MAX_NUMBER_OF_LAG_MEMBERS; i++) {
        if (lag_db.members[i].is_used && 
            lag_db.members[i].lag_oid == lag_id_val->oid &&
            lag_db.members[i].port_oid == port_id_val->oid) {
            printf("Port 0x%lx is already a member of LAG 0x%lx\n", 
                   port_id_val->oid, lag_id_val->oid);
            return SAI_STATUS_ITEM_ALREADY_EXISTS;
        }
    }

    for (; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++) {
        if (!lag_db.members[ii].is_used) {
            break;
        }
    }

    if (ii == MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Cannot create LAG MEMBER: global limit is reached\n");
        return SAI_STATUS_TABLE_FULL;
    }

    uint32_t member_db_id = ii;

    status = stub_create_object(SAI_OBJECT_TYPE_LAG_MEMBER, member_db_id, lag_member_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot create LAG MEMBER OID\n");
        return status;
    }

    bool added_to_lag = false;
    for (int k = 0; k < MAX_NUMBER_OF_LAG_MEMBERS; k++) {
        if (lag_db.lags[lag_db_idx].members_ids[k] == SAI_NULL_OBJECT_ID) {
            lag_db.lags[lag_db_idx].members_ids[k] = *lag_member_id;
            lag_db.lags[lag_db_idx].member_count++;
            added_to_lag = true;
            break;
        }
    }

    if (!added_to_lag) {
        printf("Failed to add member to LAG (no free slots)\n");
        return SAI_STATUS_FAILURE;
    }

    lag_db.members[member_db_id].is_used = true;
    lag_db.members[member_db_id].lag_oid = lag_id_val->oid;
    lag_db.members[member_db_id].port_oid = port_id_val->oid;

    sai_attr_list_to_str(attr_count, (const sai_attribute_t *)attr_list, 
                         lag_member_attribs, MAX_LIST_VALUE_STR_LEN, list_str);
    printf("CREATE LAG MEMBER: 0x%lx (%s)\n", *lag_member_id, list_str);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_sai_remove_lag_member(
    _In_ sai_object_id_t lag_member_id)
{
    sai_status_t status;
    uint32_t member_db_id;

    status = stub_object_to_type(lag_member_id, SAI_OBJECT_TYPE_LAG_MEMBER, &member_db_id);
    if (status != SAI_STATUS_SUCCESS) return status;

    if (member_db_id >= MAX_NUMBER_OF_LAG_MEMBERS || !lag_db.members[member_db_id].is_used) {
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    sai_object_id_t parent_lag_oid = lag_db.members[member_db_id].lag_oid;
    uint32_t lag_db_idx;
    
    if (stub_object_to_type(parent_lag_oid, SAI_OBJECT_TYPE_LAG, &lag_db_idx) == SAI_STATUS_SUCCESS) {
        for(int k=0; k < MAX_NUMBER_OF_LAG_MEMBERS; k++) {
            if(lag_db.lags[lag_db_idx].members_ids[k] == lag_member_id) {
                lag_db.lags[lag_db_idx].members_ids[k] = SAI_NULL_OBJECT_ID;
                if(lag_db.lags[lag_db_idx].member_count > 0) 
                    lag_db.lags[lag_db_idx].member_count--;
                break;
            }
        }
    }

    lag_db.members[member_db_id].is_used = false;
    lag_db.members[member_db_id].lag_oid = SAI_NULL_OBJECT_ID;
    lag_db.members[member_db_id].port_oid = SAI_NULL_OBJECT_ID;

    printf("REMOVE LAG MEMBER: 0x%lx\n", lag_member_id);

    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_sai_get_lag_member_attribute(
    _In_ sai_object_id_t lag_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    const sai_object_key_t key = { .object_id = lag_member_id };
    return sai_get_attributes(&key, NULL, lag_member_attribs, lag_member_vendor_attribs, attr_count, attr_list);
}

const sai_lag_api_t lag_api = {
    stub_sai_create_lag,
    stub_sai_remove_lag,
    NULL, // set_lag_attribute
    stub_sai_get_lag_attribute,
    stub_sai_create_lag_member,
    stub_sai_remove_lag_member,
    NULL, // set_lag_member_attribute
    stub_sai_get_lag_member_attribute,
};