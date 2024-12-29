Alternative firmware for stm8s based 4x relay module
====================================================

Overview
--------

This repository contains alternative firmware for popular relay boards
available from China.

![STM8S 4x Relay Board](images/stm8s_relay_board.jpeg)

Flashing
--------

```console
stm8flash -c stlinkv2 -p stm8s003f3p6 -w build/out/relay_ctl.ihx
```

Tools & Trouble Log
-----------

[Modbus tools](https://github.com/wdl83/modbus_tools), can be used to send/receive
requests from the device.
[json](json) directory contains example requests.
Current implementation supports configurable size trouble log which can be fetch
from the device

```console
    master_cli -d /dev/ttyUSB0 -i json/tlog_fetch.json -o tlog.json
```

result can be be decoded with ***tlog_dump***

```console
    cat tlog.json | tlog_dump
```

***master_cli*** supports reading from stdin and writing to stdout, so complete
operation can be done at once

```console
 cat json/tlog_fetch.json | master_cli -d /dev/ttyUSB0 -i - | tlog_dump
```
