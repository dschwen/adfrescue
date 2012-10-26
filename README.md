adfrescue
=========

Rescue files from broken ADF Amiga disk file images even if WinUAE cannot read the disks anymore.

This project is in its infancy it currently can:
* only work on OFS (old filesystem) disks
* detect file header blocks
* show the names of all files on a disk (even if the directory blocks are corrupted)

TODO
* read data block pointers and extension blocks
* detect block chains

[More info on the OFS Amiga Disk format](http://src.gnu-darwin.org/ports/archivers/unadf/work/Faq/adf_info.html)
