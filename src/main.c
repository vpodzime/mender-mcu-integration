// Copyright 2024 Northern.tech AS
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mender_app, LOG_LEVEL_DBG);

#include "netup.h"
#include "certs.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include "mender-client.h"
#include "mender-inventory.h"
#include "mender-flash.h"

#ifdef CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE
#include "mender-zephyr-image-update-module.h"
#endif /* CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE */

// TODO: Rework with MEN-7547
#define DEVICE_TYPE   "espressif-esp32"

static mender_err_t
network_connect_cb(void) {
    LOG_INF("network_connect_cb");
    return MENDER_OK;
}

static mender_err_t
network_release_cb(void) {
    LOG_INF("network_release_cb");
    return MENDER_OK;
}

static mender_err_t
deployment_status_cb(mender_deployment_status_t status, char *desc) {
    LOG_INF("deployment_status_cb: %s", desc);
    return MENDER_OK;
}

static mender_err_t
restart_cb(void) {
    LOG_INF("restart_cb");

    sys_reboot(SYS_REBOOT_WARM);

    return MENDER_OK;
}

static char              mac_address[18] = { 0 };
static mender_identity_t mender_identity = { .name = "mac", .value = mac_address };

static mender_err_t
get_identity_cb(mender_identity_t **identity) {
    LOG_INF("get_identity_cb");
    if (NULL != identity) {
        *identity = &mender_identity;
        return MENDER_OK;
    }
    return MENDER_FAIL;
}

int
main(void) {
    printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

    netup_wait_for_network();

    netup_get_mac_address(mender_identity.value);

    certs_add_credentials();

    LOG_INF("Initializing Mender Client with:");
    LOG_INF("   Device type:   '%s'", DEVICE_TYPE);
    LOG_INF("   Identity:      '{\"%s\": \"%s\"}'", mender_identity.name, mender_identity.value);

    /* Initialize mender-client */
    mender_client_config_t    mender_client_config    = { .device_type                  = DEVICE_TYPE,
                                                          .recommissioning              = false };
    mender_client_callbacks_t mender_client_callbacks = { .network_connect        = network_connect_cb,
                                                          .network_release        = network_release_cb,
                                                          .deployment_status      = deployment_status_cb,
                                                          .restart                = restart_cb,
                                                          .get_identity           = get_identity_cb,
                                                          .get_user_provided_keys = NULL };

    assert(MENDER_OK == mender_client_init(&mender_client_config, &mender_client_callbacks));
    LOG_INF("Mender client initialized");

#ifdef CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE
    assert(MENDER_OK == mender_zephyr_image_register_update_module());
    LOG_INF("Update Module 'zephyr-image' initialized");
#endif /* CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE */

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY
    mender_keystore_t inventory[] = { { .name = "demo", .value = "demo" }, { .name = "foo", .value = "bar" }, { .name = NULL, .value = NULL } };
    assert(MENDER_OK == mender_inventory_set(inventory));
    LOG_INF("Mender inventory set");
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

    /* Finally activate mender client */
    if (MENDER_OK != mender_client_activate()) {
        LOG_ERR("Unable to activate mender-client");
    } else {
        LOG_INF("Mender client activated and running!");
    }

    k_sleep(K_FOREVER);

    return 0;
}
