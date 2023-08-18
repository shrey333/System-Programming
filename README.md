
# System Programming

This repository contains different low level code implemented using C language utilizing System calls. 


## Project Objective

To learn different aspect of linux and low level programming using practical implementation of scripts that can be used to reduced repititive tasks.

## ncpmvdir.c

When you want to bulk copy or move particular sets of files (in this case files that matches particular sets of extensions) from source directory to destination directory, ncpmvdir.c is usuful program to do that.


## ncpmvdir.c

This progam can be used to copy or move an directory tree rooted at a specific path in home directory to a specific destination directory in the home directory minus the file types specified in the extension list.


```bash
  Usage: ncpmvdir [source_dir] [destination_dir] [options] <extension list>
```


## prcinfo.c

This program can be useful for searching processed in the process tree rooted at a specific process and outputs the requested information based on the input parameters.


```bash
  Usage: prcinfo [root_process] [process_id1] ... [process_id5] [options]
  OPTIONS:
  -nd: lists the PIDs of all the non-direct descendants of process_id1
  -dd: lists the PIDs of all immediate descendants of process_id1
  -sb: lists the PIDs of all the siblings processes of process_id1
  -sz: lists the PIDs of aall siblings processes of process_id1 that are defunct
  -gc: lists the PIDs of all the grandchildren of process_id1
  -zz: prints the status of process_id1 (defunct or non-defunct)
  -zc: lists the PIDs of all the direct descendants of process_id1that are currently in defunct state
```


## deftreeminus.c

This progam can be used for searching defunct processes in process tree rooted at specified process and forcefully terminates their parent processes based on the input parameters.

```bash
  Usage: deftreeminus [option1] [option2] [-processid]
  OPTION1:
    -t: forcefully terminates parent processes(whose elapsed time is greater than PROC_ELTIME) of all the defunct processes in the process tree rooted at root_process
    -b: forcefully terminates all the processes in the process tree rooted at root_process that have >= NO_OF_DFCS defunct child
  OPTION2:
    PROC_ELTIME: The elapsed time of the process in the minutes since it was created
    NO_OF_DFCS: The number of default child processes
```

## mshell.c

This is custom terminal implementation that goes into infinite waiting loop waiting for user's commands. It supports operator like piping, redirection, conditinal execution, background processing and sequential execution.

```bash
  Example: mshell$ ls -l ~/home -S -n | wc | wc -w
```

## backup.sh

This program creates complete backup of all .txt files at directory ~/ by tarring files into cb*****.tar stored at ~/home/backup/cb every 2 minutes.

```bash
  Usage: ./backup.sh
```
## client.c, server and mirror

This is client-server project and can be load balanced with mirror.
Client can request a file or set of files from server. The server searches for the files in it's root directory and returns tar.gz file. Multiple client can connect to the server from different machines and can request files. 

First 6 client request will be handled by server and next 6 client request will be handled by mirror. The remaining client connection will alternate between server and mirror wherein first client connection will be handled by server.

```bash
    List of client command:

    1) fgets file1 file2 ... file4
    - Server will search for file1...file4 and return temp.tar.gz if any file found else "No file found"

    2) tarfgetz size1 size2 <-u>
    - Server will return temp.tar.gz of files that of size1<=size<=size2 and -u will unzip temp.tar.gz at client size

    3) filesrch filename
    - Server will find filename and returns filename, file size and date created

    4) targzf <extension_list> <-u>
    - Server will return temp.tar.gz that contains all the files belonging to file types <extension_list>

    5) getdirf date1 date2 <-u>
    - Server will return temp.tar.gz of files that of date1<=date<=date2 and -u will unzip temp.tar.gz at client size
```
