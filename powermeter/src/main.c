/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Modification copyright (c) 2017 Oyvind Hovdsveen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>

#include "powermeter.h"

#define M_ITERATION_TIME_MS (10)
#define M_ADV_UPDATE_TIME_MS (5000)

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

struct bt_conn *default_conn;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, u8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
    } else {
        default_conn = bt_conn_ref(conn);
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);

    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static void bt_ready(int err)
{
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
            sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel,
};

//Restarts advertising, uses the latest power stats in the device name
void adv_update(void)
{
    //Check if it is time
    static u32_t last_time = 0;
    u32_t now = k_uptime_get_32();

    if((s32_t)(now - last_time) > M_ADV_UPDATE_TIME_MS)
    {
        last_time += M_ADV_UPDATE_TIME_MS;

        bt_le_adv_stop();

        u32_t last_min = pm_last_time_avg_pow_get(60);
        u32_t last_twenty = pm_last_time_avg_pow_get(60*20);
        u32_t last_hour = pm_last_time_avg_pow_get(60*60);

        if(last_min >= 100000)
        {
            last_min = 99999;
        }

        if(last_twenty >= 100000)
        {
            last_twenty = 9999; } if(last_hour >= 100000) {
                last_hour = 99999;
            }

        char name[30];
        //kW xx.xxx xx.xxx xx.xxx
        uint32_t size = snprintf(name,
                sizeof(name),
                "kW %d.%03d %d.%03d %d.%03d",
                last_min/1000,
                last_min%1000,
                last_twenty/1000,
                last_twenty%1000,
                last_hour/1000,
                last_hour%1000);
        struct bt_data scandata[] = {
            BT_DATA(BT_DATA_NAME_COMPLETE, name, size),
        };

        bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
                scandata, ARRAY_SIZE(scandata));
    }
}

void main(void)
{
    int err;

    err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_cb_display);

    pm_init();

    u64_t next_up = 0;
    while (1) {
        //Spin until next iteration
        while(1)
        {
            u32_t now = k_uptime_get_32();
            s32_t time_to_sleep = next_up - now;
            if(time_to_sleep > 0)
            {
                k_sleep(time_to_sleep);
            }
            else
            {
                break;
            }
        }
        next_up += M_ITERATION_TIME_MS;

        pm_blink_count_update();

        adv_update();
    }
}
