#include <linux/usb.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include "pcboven_usb.h"

#define IN_BUF_LEN  9
#define OUT_BUF_LEN 3
#define IN_INTERVAL 1
#define IN_EP       0x01
#define OUT_EP      0x02

void intr_callback(struct urb *urb);
int write_settings(struct usb_device *usbdev, int16_t temp, bool filaments);
int usb_probe(struct usb_interface *intf, const struct usb_device_id *id_table);
void usb_disconnect(struct usb_interface *intf);
void urb_complete(struct urb *urb);
long oven_ioctl(struct file *file, unsigned int code, unsigned long data);

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

struct __attribute__ ((__packed__)) oven_usb_frame {
	int16_t probe;
	int16_t internal;
	uint8_t short_vcc;
	uint8_t short_gnd;
	uint8_t open_circuit;
	uint8_t top_on;
	uint8_t bottom_on;
};

struct driver_context {
	struct oven oven;
	struct usb_device *usb_device;
	//struct *fasync_struct;
	uint8_t transfer_buffer[IN_BUF_LEN];
};

static struct driver_context *static_context = NULL;

static struct file_operations oven_fops = {
	.owner             = THIS_MODULE,
	.llseek            = NULL,
	.read              = NULL,
	.write             = NULL,
	.aio_read          = NULL,
	.aio_write         = NULL,
	.readdir           = NULL,
	.poll              = NULL,
	.unlocked_ioctl    = oven_ioctl,
	.compat_ioctl      = oven_ioctl,
	.mmap              = NULL,
	.open              = NULL,
	.flush             = NULL,
	.release           = NULL,
	.fsync             = NULL,
	.aio_fsync         = NULL,
	.lock              = NULL,
	.sendpage          = NULL,
	.get_unmapped_area = NULL,
	.check_flags       = NULL,
	.flock             = NULL,
	.splice_write      = NULL,
	.splice_read       = NULL,
	.setlease          = NULL,
	.fallocate         = NULL
};

static struct miscdevice oven_misc_device = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name = "PCBoven",
	.fops = &oven_fops,
	.nodename = "pcboven"
};

static struct usb_device_id id_table [] = {
	{ USB_DEVICE(PCBOVEN_USB_ID_VENDOR, PCBOVEN_USB_ID_PRODUCT) },
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver oven_usb_driver = {
	.name = "PCBoven",
	.probe = &usb_probe,
	.id_table = id_table,
	.disconnect = &usb_disconnect
};

ssize_t probe_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.probe_temp);
}

DEVICE_ATTR(probe_temp, S_IRUSR, probe_temp_show, NULL);

ssize_t internal_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.internal_temp);
}

DEVICE_ATTR(internal_temp, S_IRUSR, internal_temp_show, NULL);

ssize_t fault_short_vcc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.fault_short_vcc);
}

DEVICE_ATTR(fault_short_vcc, S_IRUSR, fault_short_vcc_show, NULL);

ssize_t fault_short_gnd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.fault_short_gnd);
}

DEVICE_ATTR(fault_short_gnd, S_IRUSR, fault_short_gnd_show, NULL);

ssize_t fault_open_circuit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.fault_open_circuit);
}

DEVICE_ATTR(fault_open_circuit, S_IRUSR, fault_open_circuit_show, NULL);

ssize_t filament_top_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.filament_top_on);
}

DEVICE_ATTR(filament_top_on, S_IRUSR, filament_top_on_show, NULL);

ssize_t filament_bottom_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.filament_bottom_on);
}

DEVICE_ATTR(filament_bottom_on, S_IRUSR, filament_bottom_on_show, NULL);

ssize_t target_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct driver_context *context = usb_get_intfdata(intf);
	return scnprintf(buf, PAGE_SIZE, "%d", context->oven.target_temp);
}

ssize_t target_temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct driver_context *context = usb_get_intfdata(intf);
	int val;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	context->oven.target_temp = val;

	return write_settings(usbdev, context->oven.target_temp, context->oven.enable_filaments) ?: strlen(buf);
}

DEVICE_ATTR(target_temp, S_IRUSR | S_IWUSR, target_temp_show, target_temp_store);

int __init init_module()
{
	int retval = usb_register(&oven_usb_driver);
	if (retval) {
		err("usb_register(): error %d\n", retval);
		return -1;
	}

	retval = misc_register(&oven_misc_device);
	if (retval) {
		err("misc_register(): error %d\n", retval);
		usb_deregister(&oven_usb_driver);
		return -1;
	}

	printk("REGISTERED DRIVER\n");
	return 0;
}

void __exit cleanup_module()
{
	printk("DESTROYING DRIVER\n");
	usb_deregister(&oven_usb_driver);
	misc_deregister(&oven_misc_device);
}

int usb_probe(struct usb_interface *intf, const struct usb_device_id *id_table)
{
	int ret;
	struct urb *usb_request;
	int result;

	if (static_context)
		return -ENODEV;

	if (interface_to_usbdev(intf)->descriptor.idVendor != PCBOVEN_USB_ID_VENDOR ||
	    interface_to_usbdev(intf)->descriptor.idProduct != PCBOVEN_USB_ID_PRODUCT)
	return -ENODEV;

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

	static_context = kzalloc(sizeof(struct driver_context), GFP_KERNEL);
	if (static_context == NULL)
		return -ENOMEM;

	usb_set_intfdata(intf, static_context);
	static_context->usb_device = interface_to_usbdev(intf);

	usb_request = usb_alloc_urb(0, GFP_KERNEL);

	usb_fill_int_urb(usb_request,
					 interface_to_usbdev(intf),
					 usb_rcvintpipe(interface_to_usbdev(intf), IN_EP),
					 static_context->transfer_buffer,
					 IN_BUF_LEN,
					 &intr_callback,
					 static_context,
					 IN_INTERVAL);
	result = usb_submit_urb(usb_request, GFP_KERNEL);
	if (result) {
		printk(KERN_ERR "Error registering urb (%d)", result);
		kfree(static_context);
		return -EFAULT;
	}

	return 0;
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

	kfree(static_context);
	static_context = NULL;

	module_put(THIS_MODULE);
}

void intr_callback(struct urb *urb)
{
	int result;
	struct oven *oven = &((struct driver_context *)urb->context)->oven;

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

int write_settings(struct usb_device *usbdev, int16_t temp, bool filaments)
{
	struct urb *request = NULL;
	uint8_t *out_buf;
	int result;

	out_buf = kmalloc(sizeof(char) * OUT_BUF_LEN, GFP_KERNEL);
	if (out_buf == NULL) {
		printk(KERN_ERR "Error allocating buffer\n");
		result = -ENOMEM;
		goto error;
	}

	out_buf[0] = (temp >> 0) & 0xFF;
	out_buf[1] = (temp >> 8) & 0xFF;
	out_buf[2] = filaments ? 1 : 0;

	request = usb_alloc_urb(0, GFP_KERNEL);
	if (request == NULL) {
		printk(KERN_ERR "Error allocating urb\n");
		result = -ENOMEM;
		goto error;
	}

	usb_fill_bulk_urb(request,
	                  usbdev,
	                  usb_sndbulkpipe(usbdev, OUT_EP),
	                  out_buf,
	                  OUT_BUF_LEN,
	                  &urb_complete,
	                  out_buf);

	result = usb_submit_urb(request, GFP_KERNEL);
	if (result) {
		printk(KERN_ERR "Error writing urb (%d)\n", result);
		goto error;
	}

	return 0;

error:
	if (out_buf)
		kfree(out_buf);
	if (request)
		kfree(request);

	return result;
}

void urb_complete(struct urb *urb)
{
	printk(KERN_ERR "Urb status: %d", urb->status);
	kfree(urb->transfer_buffer);
	usb_free_urb(urb);
}

long oven_ioctl(struct file *file, unsigned int code, unsigned long data)
{
	if ((code != PCBOVEN_IS_CONNECTED) && (static_context == NULL))
		return -ENODEV;

	switch (code) {
	case PCBOVEN_IS_CONNECTED:
		return (static_context != NULL);
	case PCBOVEN_SET_TEMPERATURE:
		static_context->oven.target_temp = (int16_t)data;
		break;
	case PCBOVEN_ENABLE_FILAMENTS:
		static_context->oven.enable_filaments = true;
		break;
	case PCBOVEN_DISABLE_FILAMENTS:
		static_context->oven.enable_filaments = false;
		break;
	default:
		return -ENOTTY;
	}

	return write_settings(static_context->usb_device, static_context->oven.target_temp, static_context->oven.enable_filaments);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Crawford");

