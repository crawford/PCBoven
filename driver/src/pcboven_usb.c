#include <linux/usb.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include "pcboven_usb.h"

#define IN_BUF_LEN  9
#define OUT_BUF_LEN 3
#define IN_INTERVAL 1
#define IN_EP       0x01
#define OUT_EP      0x02

void intr_callback(struct urb *urb);
int usb_probe(struct usb_interface *intf, const struct usb_device_id *id_table);
void usb_disconnect(struct usb_interface *intf);
void urb_complete(struct urb *urb);
int usb_ioctl(struct usb_interface *intf, unsigned int code, void *buf);

struct oven {
	int16_t probe_temp;
	int16_t internal_temp;
	int16_t target_temp;
	bool enable_filaments;
	bool fault_short_vcc;
	bool fault_short_gnd;
	bool fault_open_circuit;
	bool filament_top_on;
	bool filament_bottom_on;
};

struct transfer_context {
	struct oven oven;
	uint8_t transfer_buffer[IN_BUF_LEN];
};

struct __attribute__ ((__packed__)) oven_usb_frame {
	int16_t probe;
	int16_t internal;
	uint8_t short_vcc;
	uint8_t short_gnd;
	uint8_t open_circuit;
	uint8_t top_on;
	uint8_t bottom_on;
};

static struct usb_device_id id_table [] = {
	{ USB_DEVICE(USB_ID_VENDOR, USB_ID_PRODUCT) },
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver driver_info = {
	.name = "PCBoven",
	.probe = &usb_probe,
	.id_table = id_table,
	.disconnect = &usb_disconnect,
	.unlocked_ioctl = &usb_ioctl,
};

ssize_t probe_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.probe_temp);
}

DEVICE_ATTR(probe_temp, S_IRUSR, probe_temp_show, NULL);

ssize_t internal_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.internal_temp);
}

DEVICE_ATTR(internal_temp, S_IRUSR, internal_temp_show, NULL);

ssize_t fault_short_vcc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.fault_short_vcc);
}

DEVICE_ATTR(fault_short_vcc, S_IRUSR, fault_short_vcc_show, NULL);

ssize_t fault_short_gnd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.fault_short_gnd);
}

DEVICE_ATTR(fault_short_gnd, S_IRUSR, fault_short_gnd_show, NULL);

ssize_t fault_open_circuit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.fault_open_circuit);
}

DEVICE_ATTR(fault_open_circuit, S_IRUSR, fault_open_circuit_show, NULL);

ssize_t filament_top_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.filament_top_on);
}

DEVICE_ATTR(filament_top_on, S_IRUSR, filament_top_on_show, NULL);

ssize_t filament_bottom_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.filament_bottom_on);
}

DEVICE_ATTR(filament_bottom_on, S_IRUSR, filament_bottom_on_show, NULL);

ssize_t target_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct transfer_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.target_temp);
}

ssize_t target_temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct urb *request;
	int result;
	int val;

	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct transfer_context *context = usb_get_intfdata(intf);

	uint8_t *out_buf;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	context->oven.target_temp = val;

	out_buf = kmalloc(sizeof(char) * OUT_BUF_LEN, GFP_KERNEL);
	out_buf[0] = (context->oven.target_temp >> 0) & 0xFF;
	out_buf[1] = (context->oven.target_temp >> 8) & 0xFF;
	out_buf[2] = context->oven.enable_filaments;

	request = usb_alloc_urb(0, GFP_KERNEL);
	usb_fill_bulk_urb(request,
	                  usbdev,
	                  usb_sndbulkpipe(usbdev, OUT_EP),
	                  out_buf,
	                  OUT_BUF_LEN,
	                  &urb_complete,
	                  out_buf);

	result = usb_submit_urb(request, GFP_KERNEL);
	if (result) {
		printk(KERN_ERR "Error writing urb (%d)", result);
		return -EFAULT;
	}

	return strlen(buf);
}

DEVICE_ATTR(target_temp, S_IRUSR | S_IWUSR, target_temp_show, target_temp_store);

int __init init_module()
{
	int retval = usb_register(&driver_info);
	if (retval) {
		err("usb_register(): error %d\n", retval);
		return -1;
	}

	printk("REGISTERED DRIVER\n");
	return 0;
}

void __exit cleanup_module()
{
	printk("DESTROYING DRIVER\n");
	usb_deregister(&driver_info);
}

int usb_probe(struct usb_interface *intf, const struct usb_device_id *id_table)
{
	int ret;
	struct transfer_context *context;
	struct urb *usb_request;
	int result;

	if (interface_to_usbdev(intf)->descriptor.idVendor == USB_ID_VENDOR &&
	    interface_to_usbdev(intf)->descriptor.idProduct == USB_ID_PRODUCT)
	{
		try_module_get(THIS_MODULE);

		if (ret = device_create_file(&intf->dev, &dev_attr_probe_temp), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_internal_temp), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_fault_short_gnd), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_fault_short_vcc), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_fault_open_circuit), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_filament_top_on), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_filament_bottom_on), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		if (ret = device_create_file(&intf->dev, &dev_attr_target_temp), ret)
			printk(KERN_ERR "device_create_file(): %d\n", ret);

		context = kzalloc(sizeof(struct transfer_context), GFP_KERNEL);
		if (context == NULL)
			return -ENOMEM;
		usb_set_intfdata(intf, context);

		usb_request = usb_alloc_urb(0, GFP_KERNEL);

		usb_fill_int_urb(usb_request,
		                 interface_to_usbdev(intf),
		                 usb_rcvintpipe(interface_to_usbdev(intf), IN_EP),
		                 context->transfer_buffer,
		                 IN_BUF_LEN,
		                 &intr_callback,
		                 context,
		                 IN_INTERVAL);
		result = usb_submit_urb(usb_request, GFP_KERNEL);
		if (result) {
			printk(KERN_ERR "Error registering urb (%d)", result);
			return -EFAULT;
		}

		return 0;
	}

	return -ENODEV;
}

void usb_disconnect(struct usb_interface *intf)
{
	device_remove_file(&intf->dev, &dev_attr_probe_temp);
	device_remove_file(&intf->dev, &dev_attr_internal_temp);
	device_remove_file(&intf->dev, &dev_attr_fault_short_gnd);
	device_remove_file(&intf->dev, &dev_attr_fault_short_vcc);
	device_remove_file(&intf->dev, &dev_attr_fault_open_circuit);
	device_remove_file(&intf->dev, &dev_attr_filament_top_on);
	device_remove_file(&intf->dev, &dev_attr_filament_bottom_on);
	device_remove_file(&intf->dev, &dev_attr_target_temp);

	kfree(usb_get_intfdata(intf));

	module_put(THIS_MODULE);
}

void intr_callback(struct urb *urb)
{
	int result;
	struct oven *oven = &((struct transfer_context *)urb->context)->oven;

	if (urb->status == 0) {
		if (urb->actual_length >= sizeof(struct oven_usb_frame)) {
			struct oven_usb_frame *reading = urb->transfer_buffer;

			// Convert probe temp from 14 bit value to 16 bit
			oven->probe_temp = (reading->probe << 2) >> 4;

			// Convert internal temp from 12 bit value to 16 bit
			oven->internal_temp = (reading->internal << 4) >> 8;

			oven->fault_short_vcc    = !!reading->short_vcc;
			oven->fault_short_gnd    = !!reading->short_gnd;
			oven->fault_open_circuit = !!reading->open_circuit;
			oven->filament_top_on    = !!reading->top_on;
			oven->filament_bottom_on = !!reading->bottom_on;
		}
	} else {
		printk(KERN_ERR "Urb failed with: %d", urb->status);
	}

	result = usb_submit_urb(urb, GFP_KERNEL);
	if (result)
		printk(KERN_ERR "Error reregistering urb (%d)", result);
}

void urb_complete(struct urb *urb)
{
	printk(KERN_ERR "Urb status: %d", urb->status);
	kfree(urb->context);
	usb_free_urb(urb);
}

int usb_ioctl(struct usb_interface *intf, unsigned int code, void *buf)
{
	struct transfer_context *context = usb_get_intfdata(intf);
	int temp = *((int *)buf);

	switch (code) {
	case PCB_OVEN_SET_TEMPERATURE:
		context->oven.target_temp = (int16_t)temp;
		return 0;
	case PCB_OVEN_ENABLE_FILAMENTS:
		context->oven.enable_filaments = true;
		return 0;
	case PCB_OVEN_DISABLE_FILAMENTS:
		context->oven.enable_filaments = false;
		return 0;
	default:
		return -ENOTTY;
	}
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Crawford");

