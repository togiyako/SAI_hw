#include <stdio.h>
#include "sai.h"

const char* test_profile_get_value(_In_ sai_switch_profile_id_t profile_id, _In_ const char* variable) { return 0; }
int test_profile_get_next_value(_In_ sai_switch_profile_id_t profile_id, _Out_ const char** variable, _Out_ const char** value) { return -1; }
const service_method_table_t test_services = { test_profile_get_value, test_profile_get_next_value };

int main()
{
    sai_status_t      status;
    sai_switch_api_t *switch_api = NULL;
    sai_lag_api_t    *lag_api = NULL;
    
    sai_object_id_t lag1, lag2;
    sai_object_id_t mem1, mem2, mem3, mem4;
    sai_attribute_t attr;
    sai_object_id_t port_list[32];

    sai_api_initialize(0, &test_services);
    status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api);
    if (status != SAI_STATUS_SUCCESS || switch_api == NULL) {
        printf("Failed to query SWITCH API. Status: %d\n", status);
        return -1;
    }

    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    if (status != SAI_STATUS_SUCCESS || lag_api == NULL) {
        printf("Failed to query LAG API. Status: %d\n", status);
        return -1;
    }

    switch_api->initialize_switch(0, "HW_ID", 0, NULL);

    printf("\n");

    status = lag_api->create_lag(&lag1, 0, NULL);
    if (status == SAI_STATUS_SUCCESS) printf("Created LAG#1: 0x%lX\n", lag1);

    lag_api->create_lag_member(&mem1, 0, NULL);
    printf("Created LAG_MEMBER#1: 0x%lX\n", mem1);

    lag_api->create_lag_member(&mem2, 0, NULL);
    printf("Created LAG_MEMBER#2: 0x%lX\n", mem2);

    lag_api->create_lag(&lag2, 0, NULL);
    printf("Created LAG#2: 0x%lX\n", lag2);

    lag_api->create_lag_member(&mem3, 0, NULL);
    printf("Created LAG_MEMBER#3: 0x%lX\n", mem3);

    lag_api->create_lag_member(&mem4, 0, NULL);
    printf("Created LAG_MEMBER#4: 0x%lX\n", mem4);

    printf("\n");

    printf("Get LAG#1 PORT_LIST: ");
    attr.id = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list = port_list;
    attr.value.objlist.count = 32;
    lag_api->get_lag_attribute(lag1, 1, &attr);

    printf("Get LAG#2 PORT_LIST: ");
    attr.id = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list = port_list;
    attr.value.objlist.count = 32;
    lag_api->get_lag_attribute(lag2, 1, &attr);

    printf("Get LAG_MEMBER#1 LAG_ID: ");
    attr.id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    lag_api->get_lag_member_attribute(mem1, 1, &attr);

    printf("Get LAG_MEMBER#3 PORT_ID: ");
    attr.id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    lag_api->get_lag_member_attribute(mem3, 1, &attr);

    printf("\n");

    lag_api->remove_lag_member(mem2);

    printf("Get LAG#1 PORT_LIST (after removal): ");
    attr.id = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list = port_list;
    attr.value.objlist.count = 32;
    lag_api->get_lag_attribute(lag1, 1, &attr);

    lag_api->remove_lag_member(mem3);

    printf("Get LAG#2 PORT_LIST (after removal): ");
    attr.id = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list = port_list;
    attr.value.objlist.count = 32;
    lag_api->get_lag_attribute(lag2, 1, &attr);

    printf("\n");

    lag_api->remove_lag_member(mem1);
    lag_api->remove_lag_member(mem4);
    lag_api->remove_lag(lag2);
    lag_api->remove_lag(lag1);

    switch_api->shutdown_switch(0);
    sai_api_uninitialize();
    
    return 0;
}