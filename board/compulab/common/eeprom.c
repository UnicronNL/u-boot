/*
 * (C) Copyright 2011 CompuLab, Ltd. <www.compulab.co.il>
 *
 * Authors: Nikita Kiryanov <nikita@compulab.co.il>
 *	    Igor Grinberg <grinberg@compulab.co.il>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>

#define EEPROM_LAYOUT_VER_OFFSET	44
#define BOARD_SERIAL_OFFSET		20
#define BOARD_SERIAL_OFFSET_LEGACY	8
#define BOARD_REV_OFFSET		0
#define BOARD_REV_OFFSET_LEGACY		6
#define BOARD_REV_SIZE			2
#define MAC_ADDR_OFFSET			4
#define MAC_ADDR_OFFSET_LEGACY		0

#define LAYOUT_INVALID	0
#define LAYOUT_LEGACY	0xff

static int cl_eeprom_layout; /* Implicitly LAYOUT_INVALID */

static int cl_eeprom_read(uint offset, uchar *buf, int len)
{
	int res;
	unsigned int current_i2c_bus = i2c_get_bus_num();

	i2c_set_bus_num(CONFIG_SYS_I2C_EEPROM_BUS);
	res = i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, offset,
			CONFIG_SYS_I2C_EEPROM_ADDR_LEN, buf, len);

	i2c_set_bus_num(current_i2c_bus);

	return res;
}

static int cl_igb_eeprom_read(uint offset, uchar *buf, int len)
{
        int res;
        unsigned int current_i2c_bus = i2c_get_bus_num();

        i2c_set_bus_num(CONFIG_SYS_I2C_IGB_EEPROM_BUS);
        res = i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, offset,
                        CONFIG_SYS_I2C_EEPROM_ADDR_LEN, buf, len);

        i2c_set_bus_num(current_i2c_bus);

        return res;
}

static int cl_eeprom_setup_layout(void)
{
	int res;

	if (cl_eeprom_layout != LAYOUT_INVALID)
		return 0;

	res = cl_eeprom_read(EEPROM_LAYOUT_VER_OFFSET,
			     (uchar *)&cl_eeprom_layout, 1);
	if (res) {
		cl_eeprom_layout = LAYOUT_INVALID;
		return res;
	}

	if (cl_eeprom_layout == 0 || cl_eeprom_layout >= 0x20)
		cl_eeprom_layout = LAYOUT_LEGACY;

	return 0;
}

void get_board_serial(struct tag_serialnr *serialnr)
{
	u32 serial[2];
	uint offset;

	memset(serialnr, 0, sizeof(*serialnr));

	if (cl_eeprom_setup_layout())
		return;

	offset = (cl_eeprom_layout != LAYOUT_LEGACY) ?
		BOARD_SERIAL_OFFSET : BOARD_SERIAL_OFFSET_LEGACY;

	if (cl_eeprom_read(offset, (uchar *)serial, 8))
		return;

	if (serial[0] != 0xffffffff && serial[1] != 0xffffffff) {
		serialnr->low = serial[0];
		serialnr->high = serial[1];
	}
}

/*
 * Routine: cl_eeprom_read_mac_addr
 * Description: read mac address and store it in buf.
 */
int cl_eeprom_read_mac_addr(uchar *buf)
{
	uint offset;

	if (cl_eeprom_setup_layout())
		return 0;

	offset = (cl_eeprom_layout != LAYOUT_LEGACY) ?
			MAC_ADDR_OFFSET : MAC_ADDR_OFFSET_LEGACY;

	return cl_eeprom_read(offset, buf, 6);
}

int cl_igb_eeprom_read_mac_addr(uchar *buf)
{
        uint offset;

        if (cl_eeprom_setup_layout())
                return 0;

        offset = (cl_eeprom_layout != LAYOUT_LEGACY) ?
                        MAC_ADDR_OFFSET : MAC_ADDR_OFFSET_LEGACY;

        return cl_igb_eeprom_read(offset, buf, 6);
}


/*
 * Routine: cl_eeprom_get_board_rev
 * Description: read system revision from eeprom
 */
u32 cl_eeprom_get_board_rev(void)
{
	u32 rev = 0;
	char str[5]; /* Legacy representation can contain at most 4 digits */
	uint offset = BOARD_REV_OFFSET_LEGACY;

	if (cl_eeprom_setup_layout())
		return 0;

	if (cl_eeprom_layout != LAYOUT_LEGACY)
		offset = BOARD_REV_OFFSET;

	if (cl_eeprom_read(offset, (uchar *)&rev, BOARD_REV_SIZE))
		return 0;

	/*
	 * Convert legacy syntactic representation to semantic
	 * representation. i.e. for rev 1.00: 0x100 --> 0x64
	 */
	if (cl_eeprom_layout == LAYOUT_LEGACY) {
		sprintf(str, "%x", rev);
		rev = simple_strtoul(str, NULL, 10);
	}

	return rev;
};
