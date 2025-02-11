# List Shared Memory Objects

A utility for QNX that parses through the `/proc/<pid>/mappings` file looking
for shared memory objects between all processes.

On a Raspberry Pi4 running QNX 8.0.0, the following output might be seen:

```txt
# ./lshmem -vmsr
Querying PID: 1 (/proc/boot/procnto-smp-instr)... done.
Querying PID: 2 (sbin/pipe)... done.
Querying PID: 20483 (bin/slogger2)... done.
Querying PID: 20484 (usr/sbin/dumper)... done.
Querying PID: 20485 (sbin/mqueue)... done.
Querying PID: 20486 (usr/sbin/random)... done.
Querying PID: 20487 (bin/wdtkick)... done.
Querying PID: 20488 (sbin/i2c-bcm2711)... done.
Querying PID: 20489 (sbin/pci-server)... done.
Querying PID: 20490 (sbin/devc-serminiuart)... done.
Querying PID: 20491 (sbin/spi-bcm2711)... done.
Querying PID: 20492 (sbin/devb-sdmmc-bcm2711)... done.
Querying PID: 110605 (sbin/io-sock)... done.
Querying PID: 45070 (sbin/io-usb-otg)... done.
Querying PID: 69647 (sbin/devf-ram)... done.
Querying PID: 163856 (sbin/devc-pty)... done.
Querying PID: 163857 (usr/sbin/qconn)... done.
Querying PID: 110610 (sbin/dhcpcd)... done.
Querying PID: 155667 (usr/sbin/sshd)... done.
Querying PID: 167956 (bin/ksh)... done.
PID: 1 (/proc/boot/procnto-smp-instr)...
PID: 2 (sbin/pipe)...
PID: 20483 (bin/slogger2)...
 Shared Physical Memory Map
  1f287a000,11000,1,3,1,7d774bd0fb70817d,/dev/shmem/slogger2/dhcpcd.110608
  1f411a000,11000,1,3,1,7d774bd0fb72468d,/dev/shmem/slogger2/io_usb_otg.45070
  1f5223000,5000,20001,3,1,7d774bd0fb7c933d,/dev/shmem/slogger2/console.20483
  1f550c000,11000,1,3,1,7d774bd0fb7cb2dd,/dev/shmem/slogger2/wdtkick.20487
  1f55b8000,11000,1,3,1,7d774bd0fb7cd29d,/dev/shmem/slogger2/i2c_bcm2711.20488
  1f57e2000,11000,1,3,1,7d774bd0fb7cf1ed,/dev/shmem/slogger2/devc_serminiuart.20490
  1f5974000,11000,1,3,1,7d774bd0fb7c17ad,/dev/shmem/slogger2/devb_sdmmc_bcm2711.20492
PID: 20484 (usr/sbin/dumper)...
 Shared Physical Memory Map
  1f523f000,2000,20001,3,1,7d774bd0fb7ca0ad,/dev/shmem/slogger2/dumper.20484
PID: 20485 (sbin/mqueue)...
PID: 20486 (usr/sbin/random)...
 Shared Physical Memory Map
  1f5331000,9000,20001,3,1,7d774bd0fb7cb02d,/dev/shmem/slogger2/random.20486
PID: 20487 (bin/wdtkick)...
 Shared Physical Memory Map
  fe100000,1000,10001,b,407,1,memory
PID: 20488 (sbin/i2c-bcm2711)...
 Shared Physical Memory Map
  fe804000,1000,10001,b,407,1,memory
PID: 20489 (sbin/pci-server)...
 Shared Physical Memory Map
  fd500000,a000,10001,b,407,1,memory
  1f5680000,11000,20001,3,1,7d774bd0fb7ce7ed,/dev/shmem/slogger2/pci_server.20489
  1f5694000,11000,20001,3,1,7d774bd0fb7ce1ad,/dev/shmem/slogger2/pci_server.20489..0
  1f56ad000,1000,1,3,1,7d774bd0fb7ce2fd,/dev/shmem/pci/pci_hw
  1f56ae000,2000,1,3,1,7d774bd0fb7cf6fd,/dev/shmem/pci/pci_db
  1f56b0000,1000,1,3,1,7d774bd0fb7cf7bd,/dev/shmem/pci/pci_sync
 Overlapping Shared Memory Map
  (10001,b,407,1) <-> 45070: (10001,b,407,1) [memory] length=a000
  (1,3,1,7d774bd0fb7ce2fd) <-> 45070: (1,3,1,7d774bd0fb7ce2fd) [/dev/shmem/pci/pci_hw] length=1000
  (1,3,1,7d774bd0fb7cf6fd) <-> 45070: (1,3,1,7d774bd0fb7cf6fd) [/dev/shmem/pci/pci_db] length=2000
  (1,3,1,7d774bd0fb7cf7bd) <-> 45070: (1,3,1,7d774bd0fb7cf7bd) [/dev/shmem/pci/pci_sync] length=1000
  (1,3,1,7d774bd0fb7cf6fd) <-> 110605: (1,3,1,7d774bd0fb7cf6fd) [/dev/shmem/pci/pci_db] length=2000
  (1,3,1,7d774bd0fb7cf7bd) <-> 110605: (1,3,1,7d774bd0fb7cf7bd) [/dev/shmem/pci/pci_sync] length=1000
PID: 20490 (sbin/devc-serminiuart)...
 Shared Physical Memory Map
  fe215000,1000,10001,b,407,1,memory
PID: 20491 (sbin/spi-bcm2711)...
 Shared Physical Memory Map
  fe007000,1000,10001,b,407,1,memory
  fe204000,1000,10001,b,407,1,memory
  1f58a0000,9000,20001,3,1,7d774bd0fb7c06dd,/dev/shmem/slogger2/io_spi.20491
  1f58ab000,9000,20001,3,1,7d774bd0fb7d87bd,/dev/shmem/slogger2/bcm2711_dma_lib.20491
  1f58b4000,1000,1,3,1,7d774bd0fb7d817d,/dev/shmem/bcm2711_dmac_mutex
PID: 20492 (sbin/devb-sdmmc-bcm2711)...
 Shared Physical Memory Map
  19000,2000,10001,b,1,7d774bd0fb7c132d,below1G
  1b000,20000,10001,3,1,7d774bd0fb7c132d,below1G
  3b000,1000,10001,b,1,7d774bd0fb7c132d,below1G
  2000000,a49f000,90001,3,1,7d774bd0fb7ccd8d,**anonymous**
  fe340000,1000,10001,b,407,1,memory
  1f5990000,11000,20001,3,1,7d774bd0fb7fe71d,/dev/shmem/slogger2/devb_sdmmc_bcm2711.20492..0
PID: 45070 (sbin/io-usb-otg)...
 Shared Physical Memory Map
  0,2000,10001,b,1,7d774bd0fb7c132d,below1G
  1000,1000,10001,b,1,7d774bd01368e9b5,**unlinked**
  2000,2000,10001,b,1,7d774bd0fb7c132d,below1G
  3000,1000,10001,b,1,7d774bd01368e9b5,**unlinked**
  4000,1000,10001,b,1,7d774bd0fb7c132d,below1G
  5000,1000,10001,3,1,7d774bd0fb7c132d,below1G
  3c000,2000,10001,b,1,7d774bd0fb7c132d,below1G
  3d000,1000,10001,b,1,7d774bd01368e9b5,**unlinked**
  3e000,3f000,10001,b,1,7d774bd0fb7c132d,below1G
  3f000,3e000,10001,b,1,7d774bd01368e9b5,**unlinked**
  7d000,2000,10001,b,1,7d774bd0fb7c132d,below1G
  7e000,1000,10001,b,1,7d774bd01368e9b5,**unlinked**
  7f000,1000,10001,b,1,7d774bd0fb7c132d,below1G
  fd500000,a000,10001,b,407,1,memory
  1f424b000,11000,20001,3,1,7d774bd0fb7d70fd,/dev/shmem/slogger2/io_usb_otg.45070..0
  1f425f000,11000,20001,3,1,7d774bd0fb72bc5d,/dev/shmem/slogger2/io_usb_otg.45070..1
  1f56ad000,1000,1,3,1,7d774bd0fb7ce2fd,/dev/shmem/pci/pci_hw
  1f56ae000,2000,1,1,1,7d774bd0fb7cf6fd,/dev/shmem/pci/pci_db
  1f56b0000,1000,1,3,1,7d774bd0fb7cf7bd,/dev/shmem/pci/pci_sync
  1f634f000,1000,90001,3,1,7d774bd0fb7cc20d,**anonymous**
  600000000,1000,10001,b,407,1,memory
 Overlapping Shared Memory Map
  (10001,b,407,1) <-> 20489: (10001,b,407,1) [memory] length=a000
  (1,3,1,7d774bd0fb7ce2fd) <-> 20489: (1,3,1,7d774bd0fb7ce2fd) [/dev/shmem/pci/pci_hw] length=1000
  (1,1,1,7d774bd0fb7cf6fd) <-> 20489: (1,1,1,7d774bd0fb7cf6fd) [/dev/shmem/pci/pci_db] length=2000
  (1,3,1,7d774bd0fb7cf7bd) <-> 20489: (1,3,1,7d774bd0fb7cf7bd) [/dev/shmem/pci/pci_sync] length=1000
  (1,1,1,7d774bd0fb7cf6fd) <-> 110605: (1,1,1,7d774bd0fb7cf6fd) [/dev/shmem/pci/pci_db] length=2000
  (1,3,1,7d774bd0fb7cf7bd) <-> 110605: (1,3,1,7d774bd0fb7cf7bd) [/dev/shmem/pci/pci_sync] length=1000
PID: 69647 (sbin/devf-ram)...
 Shared Physical Memory Map
  1f3000000,525000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f40cc000,5000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f40e2000,b000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f433b000,11000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f4365000,84000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f4533000,16d000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46a4000,20000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46c5000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46c9000,2000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46cc000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46ce000,3000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46d2000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46d4000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46d6000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46d8000,4000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46dd000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f46e0000,69b000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f4d7c000,1000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
  1f4e02000,1fe000,1,3,1,7d774bd0fb72b4bd,/dev/shmem/fs9
PID: 110605 (sbin/io-sock)...
 Shared Physical Memory Map
  6000,1000,10001,b,1,7d774bd0fb7c132d,below1G
  7000,1000,10001,3,1,7d774bd0fb7c132d,below1G
  8000,4000,90001,3,1,7d774bd0f9219c0d,**anonymous**
  1fc6000,20000,90001,3,1,7d774bd0fb70208d,**anonymous**
  1fe6000,4000,90001,3,1,7d774bd0f9219c8d,**anonymous**
  1fea000,4000,90001,3,1,7d774bd0f9219f0d,**anonymous**
  1fee000,4000,90001,3,1,7d774bd0f9219f8d,**anonymous**
  1ff2000,4000,90001,3,1,7d774bd0f921980d,**anonymous**
  1ff6000,4000,90001,3,1,7d774bd0f921988d,**anonymous**
  1ffa000,4000,90001,3,1,7d774bd0f9219b0d,**anonymous**
  2e000000,20000,90001,3,1,7d774bd0fb70210d,**anonymous**
  2e020000,20000,90001,3,1,7d774bd0fb70218d,**anonymous**
  2e040000,20000,90001,3,1,7d774bd0fb70220d,**anonymous**
  2e060000,20000,90001,3,1,7d774bd0f9219d0d,**anonymous**
  2e080000,20000,90001,3,1,7d774bd0f9219d8d,**anonymous**
  2e0a0000,20000,90001,3,1,7d774bd0f9219e0d,**anonymous**
  2e0c0000,20000,90001,3,1,7d774bd0f9219e8d,**anonymous**
  2e0e0000,20000,90001,3,1,7d774bd0f921990d,**anonymous**
  2e100000,20000,90001,3,1,7d774bd0f921998d,**anonymous**
  2e120000,20000,90001,3,1,7d774bd0f9219a0d,**anonymous**
  2e140000,20000,90001,3,1,7d774bd0f9219a8d,**anonymous**
  2e160000,4000,90001,3,1,7d774bd0f9219b8d,**anonymous**
  2e164000,4000,90001,3,1,7d774bd0f921940d,**anonymous**
  2e168000,4000,90001,3,1,7d774bd0f921948d,**anonymous**
  2e16c000,4000,90001,3,1,7d774bd0f921950d,**anonymous**
  2e170000,4000,90001,3,1,7d774bd0f921958d,**anonymous**
  2e174000,4000,90001,3,1,7d774bd0f921960d,**anonymous**
  2e178000,4000,90001,3,1,7d774bd0f921968d,**anonymous**
  2e17c000,20000,90001,3,1,7d774bd0f921970d,**anonymous**
  2e19c000,20000,90001,3,1,7d774bd0f921978d,**anonymous**
  2e1bc000,20000,90001,3,1,7d774bd0f921900d,**anonymous**
  2e1dc000,20000,90001,3,1,7d774bd0f921908d,**anonymous**
  2e1fc000,40000,90001,3,1,7d774bd0f921910d,**anonymous**
  2e23c000,40000,90001,3,1,7d774bd0f921918d,**anonymous**
  2e27c000,40000,90001,3,1,7d774bd0f921920d,**anonymous**
  2e2bc000,40000,90001,3,1,7d774bd0f921928d,**anonymous**
  2e2fc000,40000,90001,3,1,7d774bd0f921930d,**anonymous**
  2e33c000,4000,90001,3,1,7d774bd0f9206c0d,**anonymous**
  2e340000,4000,90001,3,1,7d774bd0f9206c8d,**anonymous**
  2e344000,4000,90001,3,1,7d774bd0f9206d0d,**anonymous**
  2e348000,4000,90001,3,1,7d774bd0f9206d8d,**anonymous**
  2e34c000,20000,90001,3,1,7d774bd0f9206e0d,**anonymous**
  2e36c000,20000,90001,3,1,7d774bd0f9206e8d,**anonymous**
  2e38c000,20000,90001,3,1,7d774bd0f9206f0d,**anonymous**
  2e3ac000,20000,90001,3,1,7d774bd0f9206f8d,**anonymous**
  2e3cc000,40000,90001,3,1,7d774bd0f920680d,**anonymous**
  2e40c000,40000,90001,3,1,7d774bd0f920688d,**anonymous**
  2e44c000,40000,90001,3,1,7d774bd0f920690d,**anonymous**
  2e48c000,40000,90001,3,1,7d774bd0f920698d,**anonymous**
  2eff2000,e000,10001,1,407,1,memory
  fd580000,10000,10001,b,407,1,memory
  fd5d2000,1000,10001,b,407,1,memory
  1f2c01000,11000,20001,3,1,7d774bd0fb70905d,/dev/shmem/slogger2/io_sock.110605..0
  1f2c16000,11000,20001,3,1,7d774bd0fb7091ed,/dev/shmem/slogger2/io_sock.110605..1
  1f378c000,9000,20001,3,1,7d774bd0fb72b58d,/dev/shmem/slogger2/io_sock.110605
  1f56ae000,2000,1,1,1,7d774bd0fb7cf6fd,/dev/shmem/pci/pci_db
  1f56b0000,1000,1,3,1,7d774bd0fb7cf7bd,/dev/shmem/pci/pci_sync
  1f6365000,1000,90001,3,1,7d774bd0fb702c0d,**anonymous**
  1fa010000,2000,90001,3,1,7d774bd0fb702c8d,**anonymous**
  1fa012000,3000,90001,3,1,7d774bd0fb702d0d,**anonymous**
  1fa015000,3000,90001,3,1,7d774bd0fb702d8d,**anonymous**
  1fa018000,4000,90001,3,1,7d774bd0fb702e0d,**anonymous**
  1fa01c000,4000,90001,3,1,7d774bd0fb702e8d,**anonymous**
  1fa020000,4000,90001,3,1,7d774bd0fb702f0d,**anonymous**
  1fa024000,4000,90001,3,1,7d774bd0fb702f8d,**anonymous**
  1fa028000,4000,90001,3,1,7d774bd0fb70280d,**anonymous**
  1fa02c000,4000,90001,3,1,7d774bd0fb702a0d,**anonymous**
  1fa030000,4000,90001,3,1,7d774bd0fb702a8d,**anonymous**
  1fa034000,4000,90001,3,1,7d774bd0fb702b0d,**anonymous**
  1fa038000,4000,90001,3,1,7d774bd0fb702b8d,**anonymous**
  1fa03c000,4000,90001,3,1,7d774bd0fb70240d,**anonymous**
  1fa040000,4000,90001,3,1,7d774bd0fb70248d,**anonymous**
  1fa044000,4000,90001,3,1,7d774bd0fb70250d,**anonymous**
  1fa048000,4000,90001,3,1,7d774bd0fb70258d,**anonymous**
  1fa04c000,4000,90001,3,1,7d774bd0fb70260d,**anonymous**
  1fa050000,4000,90001,3,1,7d774bd0fb70268d,**anonymous**
  1fa054000,4000,90001,3,1,7d774bd0fb70270d,**anonymous**
  1fa058000,4000,90001,3,1,7d774bd0fb70278d,**anonymous**
  1fa05c000,20000,90001,3,1,7d774bd0fb70200d,**anonymous**
  1fa07c000,4000,90001,3,1,7d774bd0fb70228d,**anonymous**
  1fa080000,4000,90001,3,1,7d774bd0fb70230d,**anonymous**
 Overlapping Shared Memory Map
  (1,1,1,7d774bd0fb7cf6fd) <-> 20489: (1,1,1,7d774bd0fb7cf6fd) [/dev/shmem/pci/pci_db] length=2000
  (1,3,1,7d774bd0fb7cf7bd) <-> 20489: (1,3,1,7d774bd0fb7cf7bd) [/dev/shmem/pci/pci_sync] length=1000
  (1,1,1,7d774bd0fb7cf6fd) <-> 45070: (1,1,1,7d774bd0fb7cf6fd) [/dev/shmem/pci/pci_db] length=2000
  (1,3,1,7d774bd0fb7cf7bd) <-> 45070: (1,3,1,7d774bd0fb7cf7bd) [/dev/shmem/pci/pci_sync] length=1000
PID: 110610 (sbin/dhcpcd)...
PID: 155667 (usr/sbin/sshd)...
PID: 163856 (sbin/devc-pty)...
PID: 163857 (usr/sbin/qconn)...
PID: 167956 (bin/ksh)...
```

Because querying the `mappings` file might be slow, the tool prints to the
console the status of interrogating files.

Then for all PIDs in the system, it will print:

- A condensed mapping of physical address regions. You could group the dev,ino
  and sum the lengths (which are contiguous from the `mi_paddr`) to find out the
  total size of the shared object.

  ```txt
  mi_paddr,length,ri_flags,ri_prot,dev,ino,object_name
  ```

- Then all other PIDs are also compared, looking for overlaps in physical
  memory. This is in the form:

  ```txt
  (ri_flags,ri_prot,dev,ino) <-> PID: (ri_flags,ri_prot,dev,ino) [object_name] length=length
  ```

  The first tuple is for the PID in question, the second tuple is the matching
  PID. So as to not have to hunt down previous entries, the results will be
  repeated (but reversed) for the second PID when it is printed.
