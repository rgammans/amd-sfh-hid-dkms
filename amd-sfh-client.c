// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 *  AMD Sensor Fusion Hub HID client
 *
 *  Authors: Nehal Bakulchandra Shah <Nehal-Bakulchandra.Shah@amd.com>
 *           Richard Neumann <mail@richard-neumann.de>
 */

#include <linux/hid.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "amd-sfh.h"
#include "amd-sfh-client.h"
#include "amd-sfh-hid-ll-drv.h"
#include "amd-sfh-hid-reports.h"
#include "amd-sfh-pci.h"

#define AMD_SFH_HID_VENDOR	0x3fe
#define AMD_SFH_HID_PRODUCT	0x0001
#define AMD_SFH_HID_VERSION	0x0001
#define AMD_SFH_PHY_DEV		"AMD Sensor Fusion Hub (PCIe)"

/**
 * get_sensor_name - Returns the name of a sensor by its index.
 * @sensor_idx:	The sensor's index
 *
 * Returns the name of the respective sensor.
 */
static char *amd_sfh_get_sensor_name(enum sensor_idx sensor_idx)
{
	switch (sensor_idx) {
	case ACCEL_IDX:
		return "accelerometer";
	case GYRO_IDX:
		return "gyroscope";
	case MAG_IDX:
		return "magnetometer";
	case ALS_IDX:
		return "ambient light sensor";
	default:
		return "unknown sensor type";
	}
}

/**
 * amd_sfh_hid_probe - Initializes the respective HID device.
 * @pci_dev:		The underlying PCI device
 * @sensor_idx:		The sensor index
 *
 * Sets up the HID driver data and the corresponding HID device.
 * Returns a pointer to the new HID device or NULL on errors.
 */
static struct hid_device *amd_sfh_hid_probe(struct pci_dev *pci_dev,
					    enum sensor_idx sensor_idx)
{
	struct amd_sfh_hid_data *hid_data;
	struct hid_device *hid;
	char *name;
	int rc;

	hid = hid_allocate_device();
	if (IS_ERR(hid)) {
		pci_err(pci_dev, "Failed to allocate HID device!\n");
		goto err_hid_alloc;
	}

	hid->bus = BUS_I2C;
	hid->group = HID_GROUP_SENSOR_HUB;
	hid->vendor = AMD_SFH_HID_VENDOR;
	hid->product = AMD_SFH_HID_PRODUCT;
	hid->version = AMD_SFH_HID_VERSION;
	hid->type = HID_TYPE_OTHER;
	hid->ll_driver = &amd_sfh_hid_ll_driver;

	name = amd_sfh_get_sensor_name(sensor_idx);

	rc = strscpy(hid->name, name, sizeof(hid->name));
	if (rc >= sizeof(hid->name))
		hid_warn(hid, "Could not set HID device name.\n");

	rc = strscpy(hid->phys, AMD_SFH_PHY_DEV, sizeof(hid->phys));
	if (rc >= sizeof(hid->phys))
		hid_warn(hid, "Could not set HID device location.\n");

	hid_data = devm_kzalloc(&hid->dev, sizeof(*hid_data), GFP_KERNEL);
	if (!hid_data)
		goto destroy_hid_device;

	hid_data->sensor_idx = sensor_idx;
	hid_data->pci_dev = pci_dev;
	hid_data->hid = hid;
	hid_data->cpu_addr = NULL;

	rc = get_descriptor_size(sensor_idx, AMD_SFH_INPUT_REPORT);
	if (rc < 0) {
		hid_err(hid, "Failed to get input descriptor size!\n");
		goto destroy_hid_device;
	}

	hid_data->report_size = rc;

	hid_data->report_buf = devm_kzalloc(&hid->dev, hid_data->report_size,
					    GFP_KERNEL);
	if (!hid_data->report_buf) {
		hid_err(hid, "Failed to allocate memory for report buffer!\n");
		goto destroy_hid_device;
	}

	hid->driver_data = hid_data;

	rc = hid_add_device(hid);
	if (rc)	{
		hid_err(hid, "Failed to add HID device: %d\n", rc);
		goto destroy_hid_device;
	}

	return hid;

destroy_hid_device:
	hid_destroy_device(hid);
err_hid_alloc:
	return NULL;
}

/**
 * amd_sfh_client_init - Initializes the HID devices.
 * @privdata:	The driver data
 *
 * Matches the sensor bitmasks against the sensor bitmask retrieved
 * from amd_sfh_get_sensor_mask().
 * In case of a match, it instantiates a corresponding HID device
 * to process the respective sensor's data.
 */
void amd_sfh_client_init(struct amd_sfh_data *privdata)
{
	struct pci_dev *pci_dev;
	uint sensor_mask;
	int i = 0;

	pci_dev = privdata->pci_dev;
	sensor_mask = amd_sfh_get_sensor_mask(pci_dev);

	if (sensor_mask & ACCEL_MASK)
		privdata->sensors[i++] = amd_sfh_hid_probe(pci_dev, ACCEL_IDX);
	else
		privdata->sensors[i++] = NULL;

	if (sensor_mask & GYRO_MASK)
		privdata->sensors[i++] = amd_sfh_hid_probe(pci_dev, GYRO_IDX);
	else
		privdata->sensors[i++] = NULL;

	if (sensor_mask & MAGNO_MASK)
		privdata->sensors[i++] = amd_sfh_hid_probe(pci_dev, MAG_IDX);
	else
		privdata->sensors[i++] = NULL;

	if (sensor_mask & ALS_MASK)
		privdata->sensors[i++] = amd_sfh_hid_probe(pci_dev, ALS_IDX);
	else
		privdata->sensors[i++] = NULL;
}

/**
 * amd_sfh_client_deinit - Removes all active HID devices.
 * @privdata:	The driver data
 *
 * Destroys all initialized HID devices.
 */
void amd_sfh_client_deinit(struct amd_sfh_data *privdata)
{
	int i;

	for (i = 0; i < AMD_SFH_MAX_SENSORS; i++) {
		if (privdata->sensors[i])
			hid_destroy_device(privdata->sensors[i]);

		privdata->sensors[i] = NULL;
	}
}
