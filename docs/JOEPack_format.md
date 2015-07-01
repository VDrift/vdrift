This is a simple file format that just crams a bunch of files together, like a zip file, but without any compression.

Technical specification
-----------------------

"JoePack" is a binary file format in which multi-byte values are expressed with the little-endian byte order. This section details version 1 of the file format.

### Data Type Map

The following table explicitly defines the various data types used in a JOE file.

| Identifier     | Detailed Description                |
|----------------|-------------------------------------|
| unsigned int   | 32-bit un-signed integer            |
| unsigned short | 16-bit un-signed integer            |
| string\[*x*\]  | Array of characters with length *x* |

### File Header

This block of information initiates every file.

| Data type    | Block offset | Name       | Description                                                                                             |
|--------------|--------------|------------|---------------------------------------------------------------------------------------------------------|
| string\[8\]  | 0            | versionstr | Report the file version that this file conforms to. This specification details version 1 of the format. |
| unsigned int | 8            | numobjs    | This is the number of files contained in the pack                                                       |
| unsigned int | 12           | maxstrlen  | The maximum file name length in this pack                                                               |

### File Allocation Table (FAT)

The FAT consists of *numobjs* entries of the following format:

| Data type             | Block offset | Name     | Description                                                                                                                                                                                        |
|-----------------------|--------------|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| unsigned int          | 0            | offset   | Offset into the file at which this file starts. This offset is in bytes from the beginning of the file.                                                                                            |
| unsigned int          | 4            | length   | The length of the file this entry corresponds to.                                                                                                                                                  |
| string\[*maxstrlen*\] | 8            | filename | The name of the file stored at this entry. Note that this is not necessarily null terminated - VDrift stores it in a string of length *maxstrlen + 1* and pads it with a null character at the end |

### File Data

Following the FAT, the JoePack file simply consists of all the data stored sequentially. Seeking to the offset specified in the FAT (from the beginning of the file) will allow you to read that file's data like normal.

<Category:Files>
