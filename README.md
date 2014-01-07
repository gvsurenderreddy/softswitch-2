softswitch
==========

A network traffic sniffering module inside Xen's hypervisors

We use the Linux 2.6.32.43 kernel Xenified with Xen 3.4.3 to implement our sniffering module. Source files in three directories of the Linux kernel are modified to enable traffic sniffering. In particular, net/soft-bridge is a new directory added to the Linux kernel that implements the main logic of the traffic sniffering module. 

In run time, the hypevisor will direct network traffic in either the normal or our soft bridge.

More documentation is to be added.
