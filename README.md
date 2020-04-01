# simavr-sd

SD card emulator for simavr. The goal of this project is not to accurately simulate all SD cards, but simply to be good enough to fool the Arduino SD library for testing purposes.

## Setting up a disk image

```sh
truncate -s 1G something.img
fdisk something.img # create partition table, and an empty partition
losetup -fP --show something.img
mkfs.fat /dev/loop0p1 # adjust loop device argument as appropriate
mount /dev/loop0p1 /mnt
touch /mnt/TEST.TXT
umount /mnt
losetup -d /dev/loop0
```

## Credit where credit is due

This is entirely my own work. However, while working I discovered [an existing SD card emulator](https://gitlab.com/brewing-logger/firmware/-/blob/master/simulation/sd_card.c) buried in a larger hardware project on GitLab. I have taken some inspiration from them for things I wasn't sure how to implement, such as how often MISO IRQs should be sent (every time a MOSI IRQ is received, it turns out!)
