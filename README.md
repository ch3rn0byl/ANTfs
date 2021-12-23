# ANTfs
ANTfs is an antiforensics project that came about after taking a Digital Forensics class this semester. The name stems from anti and NTFS since I am manipulating the filesystem. This project consists of a usermode application that will recover files that have been deleted and outputs them into a directory of your choice for later analysis. The driver is the kernel mode application that takes care of overwriting the file record and file's contents to wipe it into the abyss -- not hide it. 

The use case of this application is when you have shell access on a computer you ~~don't~~ own and you introduce a file to the system and you need to delete it to hide your traces. For science. 

Also, this is just a PoC. A blogpost will follow when finished.

### Requirements:
1. Windows SDK
2. Windows WDK

The SDK and the WDK are needed to be able to compile this on your own if you choose to modify this. This was compiled using Visual Studio 2019. 

### How to use?
![alt text](https://github.com/ch3rn0byl/ANTfs/blob/master/Images/helpmenu.png)

If you are wanting to recover deleted files from the system: 
```
ANTfs.exe --drive c --recover --outfile
```
A timestamped directory will be created in the same directory ANTfs is being run from if no path is given. Add an output path if needed (highly recommended):
```
ANTfs.exe --drive c --recover --outfile Y:\loot
```
If you are wiping a record from the system, the driver needs to be installed:
```
sc create antfs binPath= <path/to/driver>/antfs driver.sys type= kernel
sc start antfs
```
Then on the application:
```
ANTfs.exe --drive c --wipe some_file_name.file_extension
```
Once the file record is found, the usermode application will make a call to the driver and wipe the record from within the kernel.

### How does this work?
#### Usermode Application: ANTfs.exe
ANTfs is the usermode application that will create a handle to C: (or whatever volume you give it), and begin reading from the MFT. ANTfs will iterate through each file record in the MFT(s) and check its allocation bit. If it's unallocated, I process that record. This does not get files that have been thrown into the recycle bin. ANTfs will either recover the data or wipe the data, depending on what you choose. If you want to wipe the record, that is when the driver comes to play.

#### Kernel Driver: ANTfs Driver.sys
ANTfs Driver is the kernel mode application that will bypass direct write blocking by setting the SL_FORCE_OVERWRITE inside the IRP's flags and sending a Write request to the lower driver. 

### TODOs:
1. Look into the structure of RCRD inside $LOGFILE and then go in and delete the name of the file from there. 
2. See if it's possible to write directly to the record through usermode (if possible). This will eliminate the need for the driver.
3. Look into $USNJRNL to see if there are any artifacts.
