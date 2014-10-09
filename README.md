adfrescue
=========

Rescue files from broken ADF Amiga disk file images even if WinUAE cannot read the disks anymore.

This project is in its infancy it currently can:
* only work on OFS (old filesystem) disks
* detect file header blocks
* show the names of all files on a disk (even if the directory blocks are corrupted)
* Checks list of data blocks and reports on the number of broken blocks in the file

TODO
* read data block pointers and extension blocks
* detect block chains

[More info on the OFS Amiga Disk format](http://src.gnu-darwin.org/ports/archivers/unadf/work/Faq/adf_info.html)

I've also added a copy of the ADF FAQ (Copyright (C) 1997-1999 by Laurent Clévy) for convenience. It may be freely distributed, provided the author name and addresses are included and no money is charged for this document.
