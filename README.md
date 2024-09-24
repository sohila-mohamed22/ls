# Custom `ls` Command Implementation

This project implements a custom version of the Unix `ls` command, providing detailed file and directory listings with various options (`-l`, `-a`, `-t`, `-u`, `-c`, `-i`, `-f`, `-d`, `-1`). The output supports multiple directories and is organized in a table format for better readability.

## Table of Contents
- [Features](#features)
- [Options](#options)
- [Installation](#installation)
- [Usage](#usage)
- [Examples](#examples)
- [Directory Table Format](#directory-table-format)
- [Video Explanation](#Video-Explanation)

## Features
- Implements the `ls` command with several standard options.
- Supports listing contents of one or more directories.
- Output is organized in a table format for improved readability.
- Optimized for efficiency with multiple flags and options.

## Options
The command supports the following options:

- **`-l`**: Displays detailed information about each file (permissions, number of links, owner, group, size, and modification time).
- **`-a`**: Lists all entries, including hidden files (those starting with `.`).
- **`-t`**: Sorts files by modification time (newest first).
- **`-u`**: Uses the last access time for sorting (instead of modification time).
- **`-c`**: Uses the last status change time for sorting (instead of modification time).
- **`-i`**: Displays the inode number for each file.
- **`-f`**: Lists files without sorting and includes hidden files (equivalent to `-a` without sorting).
- **`-d`**: Lists directories themselves, rather than their contents.
- **`-1`**: Forces output to display one entry per line.

## Installation

To install and use this custom `ls` command:

1. Clone the repository:
   ```bash
   git clone https://github.com/sohila-mohamed22/ls
   cd custom
   ```
2. Compile the project (if using a compiled language like C):
   ```bash
   gcc myls.c ls_Functions.c -o myls
   ```
3. Run the command:
   ```bash
   ./myls [OPTIONS] [DIRECTORY...]
   ```

## Examples

### Basic listing of a directory:
  ```bash
  ./myls
  ```
### Listing all files including hidden files:
  ```bash
  ./myls -a
  ```
### Detailed listing with inode numbers:
  ```bash
  ./myls -li
  ```
### List contents of multiple directories:
  ```bash
  ./myls dir1 dir2
  ```
### Sorting files by last access time:
  ```bash
  ./myls -lu
  ```
### Display directories themselves, not their contents:
  ```bash
  ./myls -d dir1 dir2
  ```

## Directory Table Format

When listing directories, the output is organized into a table format to enhance readability. The table includes the following columns:

| Inode Number | Permissions | Links | Owner | Group | Size | Date | Name |
|--------------|-------------|-------|-------|-------|------|------|------|
| `<INODE>`    | `<PERMS>`   | `<L>` | `<O>` | `<G>` | `<S>`| `<D>`| `<N>`|

- **Inode Number**: The inode number of the file.
- **Permissions**: File permissions (e.g., `drwxr-xr-x`).
- **Links**: Number of hard links to the file.
- **Owner**: The owner of the file.
- **Group**: The group associated with the file.
- **Size**: File size in bytes.
- **Date**: Last modification date.
- **Name**: The name of the file or directory.


## Video Explanation

For a detailed walkthrough of the `ls` command implementation and its options, you can watch the video below:

https://drive.google.com/file/d/10CIcQ_RiDGxQ_Tl_Qvq3J5coAFB4mZva/view?usp=sharing

In this video, we cover:
- How to install and use the custom `ls` command.
- Explanation of all supported options.
- Examples of usage.
- How the directory table format works.

Click the image or the link above to watch the full video.

