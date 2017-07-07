# collaboriso
Multiboot usb generator

## REQUIRED
* lslk
* umount
* parted
* mkfs.fat
* mkntfs
* mount
* chroot
* grub-install
* 7z
* kdesudo | gksudo

## OS
Only Linux is supported at this time. The source code may compile on other platforms but without the console apps listed above you won't be able to generate the usb.

## BUILD
qmake -recursive
make
cd gui
./collaboriso

## TODO
* replace all console apps with cross-platform libraries
* implement import/restore feature
* improve drag/drop behavior to highlight tree items on hover
* selectively update individual ISOs instead of entire db

## NOTES
This application is intended to be used with a web app where users can post commands to boot any iso. Without the web app you will not be able to update the database and all ISOs will be listed as unsupported. However, you can still use this application to design the grub menu and generate the bootable usb and grub.cfg files. In the examples folder there is a json file that can be used for testing purposes. If you have a local server running just copy collaboriso.json to your root web folder (/var/www) and run the update.

## SUPPORT THE PROJECT
[Donate at patreon](https://www.patreon.com/collaboriso)
