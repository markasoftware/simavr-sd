# simavr-sd

SD card emulator for simavr. The goal of this project is not to accurately simulate all SD cards, but simply to be good enough to fool the Arduino SD library for testing purposes.

This is entirely my own work. However, while working I discovered [an existing SD card emulator](https://gitlab.com/brewing-logger/firmware/-/blob/master/simulation/sd_card.c) buried in a larger hardware project on GitLab. I have taken some inspiration from them for things I wasn't sure how to implement, such as how often MISO IRQs should be sent (every time a MOSI IRQ is received, it turns out!)
