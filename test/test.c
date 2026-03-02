#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "sai.h"

const char* test_profile_get_value(_In_ sai_switch_profile_id_t profile_id, _In_ const char* variable) { return 0; }
int test_profile_get_next_value(_In_ sai_switch_profile_id_t profile_id, _Out_ const char** variable, _Out_ const char** value) { return -1; }
const service_method_table_t test_services = { test_profile_get_value, test_profile_get_next_value };

static sai_status_t create_lag_member_full(sai_lag_api_t   *lag_api,
                                           sai_object_id_t *member_oid,
                                           sai_object_id_t  lag_oid,
                                           sai_object_id_t  port_oid)
{
    sai_attribute_t attrs[2];
    attrs[0].id        = SAI_LAG_MEMBER_ATTR_LAG_ID;
    attrs[0].value.oid = lag_oid;
    attrs[1].id        = SAI_LAG_MEMBER_ATTR_PORT_ID;
    attrs[1].value.oid = port_oid;
    return lag_api->create_lag_member(member_oid, 2, attrs);
}

static int objlist_contains(sai_object_id_t *list, uint32_t count, sai_object_id_t oid)
{
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] == oid) return 1;
    }
    return 0;
}

int main()
{
    sai_status_t      status;
    sai_switch_api_t *switch_api = NULL;
    sai_lag_api_t    *lag_api    = NULL;

    sai_api_initialize(0, &test_services);

    status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api);
    if (status != SAI_STATUS_SUCCESS || switch_api == NULL) {
        printf("switch api query failed, status: %d\n", status);
        return -1;
    }

    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    if (status != SAI_STATUS_SUCCESS || lag_api == NULL) {
        printf("lag api query failed, status: %d\n", status);
        return -1;
    }

    switch_api->initialize_switch(0, "HW_ID", 0, NULL);

    sai_attribute_t attr;
    sai_object_id_t port_list[64];

    attr.id                    = SAI_SWITCH_ATTR_PORT_LIST;
    attr.value.objlist.list    = port_list;
    attr.value.objlist.count   = 64;
    status = switch_api->get_switch_attribute(1, &attr);
    if (status != SAI_STATUS_SUCCESS || attr.value.objlist.count < 3) {
        printf("Failed to get port list or not enough ports \n");
        return -1;
    }
    uint32_t num_ports = attr.value.objlist.count;
    printf("Switch has %u ports\n\n", num_ports);

    sai_object_id_t port1 = port_list[0];
    sai_object_id_t port2 = port_list[1];
    sai_object_id_t port3 = port_list[2];

    printf("Using ports: port1=0x%lX  port2=0x%lX  port3=0x%lX\n\n",
           port1, port2, port3);

    printf("--- test: basic lag create ---\n");
    sai_object_id_t lag1;
    status = lag_api->create_lag(&lag1, 0, NULL);
    assert(status == SAI_STATUS_SUCCESS);
    printf("Created LAG: 0x%lX\n\n", lag1);

    printf("--- test: create member with no attrs ---\n");
    sai_object_id_t bad_member;
    status = lag_api->create_lag_member(&bad_member, 0, NULL);
    assert(status != SAI_STATUS_SUCCESS);
    printf("create_lag_member with no attrs correctly returned failure\n\n");

    printf("--- test: create member with invalid port oid ---\n");
    sai_object_id_t invalid_port = 0xDEADBEEFUL;
    sai_object_id_t bad_member2;
    status = create_lag_member_full(lag_api, &bad_member2, lag1, invalid_port);
    assert(status != SAI_STATUS_SUCCESS);
    printf("create_lag_member with invalid port oid correctly returned failure\n\n");

    printf("--- test: create lag members ---\n");
    sai_object_id_t mem1, mem2, mem3;

    status = create_lag_member_full(lag_api, &mem1, lag1, port1);
    assert(status == SAI_STATUS_SUCCESS);
    printf("member1: 0x%lX (port=0x%lX)\n", mem1, port1);

    status = create_lag_member_full(lag_api, &mem2, lag1, port2);
    assert(status == SAI_STATUS_SUCCESS);
    printf("member2: 0x%lX (port=0x%lX)\n", mem2, port2);

    sai_object_id_t lag2;
    status = lag_api->create_lag(&lag2, 0, NULL);
    assert(status == SAI_STATUS_SUCCESS);
    printf("lag2: 0x%lX\n", lag2);

    status = create_lag_member_full(lag_api, &mem3, lag2, port3);
    assert(status == SAI_STATUS_SUCCESS);
    printf("member3: 0x%lX (port=0x%lX)\n\n", mem3, port3);

    printf("--- test: get lag member attrs ---\n");
    attr.id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    status = lag_api->get_lag_member_attribute(mem1, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.oid == lag1);
    printf("mem1 lag_id: 0x%lX (expected 0x%lX)\n", attr.value.oid, lag1);

    attr.id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    status = lag_api->get_lag_member_attribute(mem1, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.oid == port1);
    printf("mem1 port_id: 0x%lX (expected 0x%lX)\n", attr.value.oid, port1);

    attr.id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    status = lag_api->get_lag_member_attribute(mem3, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.oid == lag2);
    printf("mem3 lag_id: 0x%lX (expected 0x%lX)\n", attr.value.oid, lag2);

    attr.id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    status = lag_api->get_lag_member_attribute(mem3, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.oid == port3);
    printf("mem3 port_id: 0x%lX (expected 0x%lX)\n\n", attr.value.oid, port3);

    printf("--- test: get lag port_list ---\n");
    sai_object_id_t lag_ports[32];

    attr.id                    = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list    = lag_ports;
    attr.value.objlist.count   = 32;
    status = lag_api->get_lag_attribute(lag1, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.objlist.count == 2);
    assert(objlist_contains(lag_ports, attr.value.objlist.count, port1));
    assert(objlist_contains(lag_ports, attr.value.objlist.count, port2));
    printf("lag1 port_list: count=%d, has port1 and port2\n", attr.value.objlist.count);

    attr.id                    = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list    = lag_ports;
    attr.value.objlist.count   = 32;
    status = lag_api->get_lag_attribute(lag2, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.objlist.count == 1);
    assert(objlist_contains(lag_ports, attr.value.objlist.count, port3));
    printf("lag2 port_list: count=%d, has port3\n\n", attr.value.objlist.count);

    printf("--- test: remove lag with members should fail ---\n");
    status = lag_api->remove_lag(lag1);
    assert(status != SAI_STATUS_SUCCESS);
    printf("remove_lag correctly returned failure\n\n");

    printf("--- test: port_list after removing a member ---\n");
    status = lag_api->remove_lag_member(mem2);
    assert(status == SAI_STATUS_SUCCESS);

    attr.id                    = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list    = lag_ports;
    attr.value.objlist.count   = 32;
    status = lag_api->get_lag_attribute(lag1, 1, &attr);
    assert(status == SAI_STATUS_SUCCESS);
    assert(attr.value.objlist.count == 1);
    assert(objlist_contains(lag_ports, attr.value.objlist.count, port1));
    assert(!objlist_contains(lag_ports, attr.value.objlist.count, port2));
    printf("After removing mem2, LAG#1 count == 1, only port1 remains\n\n");

    printf("--- test: remove lag after all members gone ---\n");
    status = lag_api->remove_lag_member(mem1);
    assert(status == SAI_STATUS_SUCCESS);

    status = lag_api->remove_lag(lag1);
    assert(status == SAI_STATUS_SUCCESS);
    printf("remove_lag succeeded after all members removed\n\n");

    printf("--- test: lag limit (max 5) ---\n");
    sai_object_id_t extra_lags[4];
    for (int i = 0; i < 3; i++) {
        status = lag_api->create_lag(&extra_lags[i], 0, NULL);
        assert(status == SAI_STATUS_SUCCESS);
    }
    status = lag_api->create_lag(&extra_lags[3], 0, NULL);
    assert(status == SAI_STATUS_SUCCESS);
    printf("created 5 lags total\n");

    sai_object_id_t overflow_lag;
    status = lag_api->create_lag(&overflow_lag, 0, NULL);
    assert(status != SAI_STATUS_SUCCESS);
    printf("6th lag failed\n\n");

    for (int i = 0; i < 4; i++) lag_api->remove_lag(extra_lags[i]);

    printf("--- test: lag slot reuse ---\n");
    status = lag_api->remove_lag_member(mem3);
    assert(status == SAI_STATUS_SUCCESS);
    status = lag_api->remove_lag(lag2);
    assert(status == SAI_STATUS_SUCCESS);

    sai_object_id_t reuse_lags[5];
    for (int i = 0; i < 5; i++) {
        status = lag_api->create_lag(&reuse_lags[i], 0, NULL);
        assert(status == SAI_STATUS_SUCCESS);
    }
    printf("After freeing all LAGs, 5 new LAGs created successfully\n\n");
    for (int i = 0; i < 5; i++) lag_api->remove_lag(reuse_lags[i]);

    printf("--- test: members per lag limit (max 16) ---\n");
    if (num_ports < 18) {
        printf("need at least 18 ports, have %u\n\n", num_ports);
    } else {
        sai_object_id_t test_lag;
        status = lag_api->create_lag(&test_lag, 0, NULL);
        assert(status == SAI_STATUS_SUCCESS);

        sai_object_id_t members[17];
        for (int i = 0; i < 16; i++) {
            status = create_lag_member_full(lag_api, &members[i], test_lag, port_list[i]);
            assert(status == SAI_STATUS_SUCCESS);
        }
        printf("created 16 members\n");

        status = create_lag_member_full(lag_api, &members[16], test_lag, port_list[16]);
        assert(status != SAI_STATUS_SUCCESS);
        printf("17th member failed\n\n");

        for (int i = 0; i < 16; i++) lag_api->remove_lag_member(members[i]);
        lag_api->remove_lag(test_lag);
    }

    printf("--- test: global member limit ---\n");
    if (num_ports < 18) {
        printf("need at least 18 ports, have %u\n\n", num_ports);
    } else {
        sai_object_id_t lag_a, lag_b;
        status = lag_api->create_lag(&lag_a, 0, NULL);
        assert(status == SAI_STATUS_SUCCESS);
        status = lag_api->create_lag(&lag_b, 0, NULL);
        assert(status == SAI_STATUS_SUCCESS);

        sai_object_id_t global_members[16];
        for (int i = 0; i < 15; i++) {
            status = create_lag_member_full(lag_api, &global_members[i], lag_a, port_list[i]);
            assert(status == SAI_STATUS_SUCCESS);
        }

        status = create_lag_member_full(lag_api, &global_members[15], lag_b, port_list[15]);
        assert(status == SAI_STATUS_SUCCESS);
        printf("created 16 members across 2 lags\n");

        sai_object_id_t overflow_member;
        status = create_lag_member_full(lag_api, &overflow_member, lag_b, port_list[16]);
        assert(status != SAI_STATUS_SUCCESS);
        printf("17th member failed\n\n");

        for (int i = 0; i < 16; i++)
            lag_api->remove_lag_member(global_members[i]);
        lag_api->remove_lag(lag_a);
        lag_api->remove_lag(lag_b);
    }

    switch_api->shutdown_switch(0);
    sai_api_uninitialize();

    return 0;
}