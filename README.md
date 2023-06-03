# ESP32 BLE Printer Server

This project is a good exercise in understanding the Bluetooth protocol and how easy it is to implement a fake/mimic device in code to ease in reverse engineering, or adapting products outside their indented purpose.

This code advertises a service UUID our target apps are looking for, and then creates a service with write and notify characteristics (think of these as endpoints you can write/read/push data to/from), then we simply dump what the Android printer apps send us.

This code has some protocol-specific packet decoding, but that can simply be removed and replaced.
