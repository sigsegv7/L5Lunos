# All about the L5 PCI(e) driver

## Overview

The PCI(e) bus driver is responsible for communicating with the PCI(e) root complex or host bridge,
enumerating devices, and bringing up PCI(e) device drivers. The bus is made up of several devices that
each have their own device drivers as well. These device drivers communicate with the PCI(e) bus driver
during initialization to advertise themselves.

## Initialization basics

A PCI(e) device driver must have a generic module initialization routine to initialize basic state
as well as advertising itself to the PCI(e) bus driver, a basic routine of this kind may look like this:


```c
static int
driver_init(struct module *modp)
{
    int error;

    ...

    /* Advertise ourselves to the PCI(e) bus driver */
    if ((error = pci_advoc(&driver)) < 0) {
        pr_trace("failed to advocate for HBA\n");
        return error;
    }

    ...
    return 0;
}

static int
dev_attach(struct pci_adv *adv)
{
    struct bus_space bs;

    ...

    return hw_init();
}

static struct pci_adv driver = {
    .lookup = PCI_CS_ID(class, subcls), /* Class/subclass IDs */
    .attach = dev_attach,               /* Attach routine */
    .classrev = 1                       /* We are using class/subclass */
};

MODULE_EXPORT("driver-name", MODTYPE_PCI, driver_init);
```

The ``pci_adv`` structure contains information describing the device that the driver advocates itself
for. The lookup field can either be a pair of device ID and vendor ID values or device class and
subclass values (denoted to be used by the boolean ``classrev`` field).

## Device attachment

When a PCI(e) devices is attached and/or detected, the PCI(e) bus driver will look for a PCI(e) device
driver with a matching ``pci_adv`` lookup field. If one is found, the ``attach`` callback is invoked in
which the device / hardware initialization begins and the drivers functionality kicks on.
