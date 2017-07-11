# collaboriso
Multiboot usb generator

## REQUIRED
* lsblk
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

## TAGS
Designed to appear similar to spacebars (from the meteor framework), these tags can be used to insert certain values into your grub configuration files. You can use these tags anywhere in the grub header or in the commands used for any menu entry. There is also one unique tag that can be inserted into the update url.
* {{UUID}}
* {{ISOFOLDER}}
* {{TIMESTAMP}} for update url only

## TODO
* replace all console apps with cross-platform libraries
* implement import/restore feature to edit previously created USBs
* improve drag/drop behavior to highlight tree items on hover
* selectively update individual ISOs instead of entire db

## NOTES
This application is intended to be used with a web app where users can post commands to boot any iso. Without the web app you will not be able to update the database and all ISOs will be listed as unsupported. However, you can still use this application to design the grub menu and generate the bootable usb and grub.cfg files. In the examples folder there is a json file that can be used for testing purposes. If you have a local server running just copy collaboriso.json to your root web folder (/var/www), open the cutils.cpp file and change the updateURL to the location of the json file. Then run the update.

## CONTRIBUTE
I would like to have someone adopt the cli app and take over its development. My focus will be on the web app for the foreseeable future. I'd like to get the qt interface working on all platforms but I just don't have the time. Short term fix would be to create a separate cli app for each platform, determine platform after generate button is clicked, then launch corresponding app. Long term goal is to handle all of these functions internally to properly display a progress indicator, handle errors and avoid using QProcess.
