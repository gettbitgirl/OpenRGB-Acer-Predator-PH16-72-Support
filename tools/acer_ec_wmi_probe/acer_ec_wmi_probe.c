// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/acpi.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wmi.h>

#include "acer_ec_wmi_probe.h"

#define ACER_EC_WMI_GUID "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"
#define ACER_EC_SET_GAMING_LED_METHOD 2
#define ACER_EC_SET_GAMING_RGB_KB_METHOD 6
#define ACER_EC_GET_GAMING_RGB_KB_METHOD 7
#define ACER_EC_SET_GAMING_KB_BACKLIGHT_METHOD 20
#define ACER_EC_GET_GAMING_KB_BACKLIGHT_METHOD 21

static bool valid_request(const struct acer_ec_wmi_request *request)
{
    if (request->input_len > ACER_EC_WMI_MAX_DATA ||
        request->output_capacity > ACER_EC_WMI_MAX_DATA)
        return false;

    switch (request->method_id) {
    case ACER_EC_SET_GAMING_LED_METHOD:
        /* PH16-71/72 exposes SetGamingLED as a 12-byte byte array. */
        return request->input_len == 12;
    case ACER_EC_SET_GAMING_RGB_KB_METHOD:
        return request->input_len == sizeof(__u64);
    case ACER_EC_GET_GAMING_RGB_KB_METHOD:
        return request->input_len == sizeof(__u64);
    case ACER_EC_SET_GAMING_KB_BACKLIGHT_METHOD:
        return request->input_len == 16;
    case ACER_EC_GET_GAMING_KB_BACKLIGHT_METHOD:
        return request->input_len == sizeof(__u64);
    default:
        return false;
    }
}

static long acer_ec_wmi_ioctl(struct file *file, unsigned int command,
                              unsigned long argument)
{
    struct acer_ec_wmi_request request;
    struct acpi_buffer input;
    struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
    union acpi_object *object;
    size_t copy_length = 0;
    acpi_status status;

    if (command != ACER_EC_WMI_IOC_CALL)
        return -ENOTTY;

    if (copy_from_user(&request, (void __user *)argument, sizeof(request)))
        return -EFAULT;

    if (!valid_request(&request))
        return -EINVAL;

    request.output_len = 0;
    request.acpi_status = 0;
    memset(request.output, 0, sizeof(request.output));

    input.length = request.input_len;
    input.pointer = request.input;

    status = wmi_evaluate_method(ACER_EC_WMI_GUID, 0, request.method_id,
                                 &input, &output);
    request.acpi_status = (s32)status;

    if (ACPI_SUCCESS(status) && output.pointer) {
        object = output.pointer;

        if (object->type == ACPI_TYPE_BUFFER) {
            copy_length = min_t(size_t, object->buffer.length,
                                request.output_capacity);
            memcpy(request.output, object->buffer.pointer, copy_length);
        } else if (object->type == ACPI_TYPE_INTEGER) {
            copy_length = min_t(size_t, sizeof(object->integer.value),
                                request.output_capacity);
            memcpy(request.output, &object->integer.value, copy_length);
        }

        request.output_len = copy_length;
    }

    kfree(output.pointer);

    if (copy_to_user((void __user *)argument, &request, sizeof(request)))
        return -EFAULT;

    return 0;
}

static const struct file_operations acer_ec_wmi_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = acer_ec_wmi_ioctl,
    .compat_ioctl = acer_ec_wmi_ioctl,
};

static struct miscdevice acer_ec_wmi_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "acer_ec_wmi_probe",
    .fops = &acer_ec_wmi_fops,
    .mode = 0600,
};

static int __init acer_ec_wmi_init(void)
{
    if (!wmi_has_guid(ACER_EC_WMI_GUID)) {
        pr_err("Acer EC WMI GUID is not present\n");
        return -ENODEV;
    }

    return misc_register(&acer_ec_wmi_device);
}

static void __exit acer_ec_wmi_exit(void)
{
    misc_deregister(&acer_ec_wmi_device);
}

module_init(acer_ec_wmi_init);
module_exit(acer_ec_wmi_exit);

MODULE_AUTHOR("OpenRGB Acer Predator reverse-engineering work");
MODULE_DESCRIPTION("Guarded Acer EC WMI probe for rear-lightbar experiments");
MODULE_LICENSE("GPL");
