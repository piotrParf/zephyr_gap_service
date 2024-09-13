/*
 * Copyright (c) 2022 Michal Morsisko
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <host/id.h>
#include <host/hci_core.h>

/* Custom Service Variables */
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

static struct bt_le_adv_param adv_param;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL)};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		printk("Connection failed (err 0x%02x)\n", err);
	}
	else
	{
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected};

void set_ble_public_address(bt_addr_t *addr)
{
	struct net_buf *buf = NULL;
	struct net_buf *rsp = NULL;
	char addr_s[BT_ADDR_LE_STR_LEN];

	struct bt_hci_cp_vs_write_bd_addr *cp;

	int err;
	buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_BD_ADDR, sizeof(*cp));
	if (!buf)
	{
		printk("Unable to allocate command buffer");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(&cp->bdaddr, addr, sizeof(bt_addr_t));
	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_BD_ADDR, buf, &rsp);

	if (err)
	{
		printk("Command BT_HCI_OP_VS_WRITE_BD_ADDR not executed properly");
		return;
	}

	bt_addr_to_str(addr, addr_s, sizeof(addr_s));
	printk("Set public address to %s", addr_s);

	bt_setup_public_id_addr();

	net_buf_unref(rsp);
}

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	adv_param = *BT_LE_ADV_CONN_NAME;

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
	}
	else
	{
		printk("Advertising successfully started\n");
	}
}

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_addr_t identity = {
		.val = {0xef, 0xbe, 0x00, 0x00, 0xad, 0xde},
	};

	set_ble_public_address(&identity);

	bt_ready();

	while (1)
	{
		k_sleep(K_FOREVER);
	}
	return 0;
}
