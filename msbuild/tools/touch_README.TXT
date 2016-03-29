===============================================================================
Win32 Console ToolBox, Version 1.1                   Last Updated: June 8, 2013
===============================================================================

This readme contains the following information:

    1. About the Win32 Console ToolBox
    2. Version history
    3. Overview of command line syntax
    4. Help for DeTab
    5. Help for ErrMsg
    6. Help for FileCase
    7. Help for FileType
    8. Help for Global
    9. Help for NewLine
   10. Help for ShutDown
   11. Help for Touch
   12. Help for WebCat



===============================================================================
1. About the Win32 Console ToolBox
===============================================================================

The Win32 Console ToolBox consists of nine freeware tools developed for the
Windows 95 and Windows NT operating systems.  All tools were developed by 
Steve P. Miller.  The latest binaries for these tools, as well as many other
applications, can be found on my web page at http://SteveMiller.net/.
Please mail any bugs or suggestions to web@stevemiller.net.



===============================================================================
2. Version history
===============================================================================

May 15, 1997 - Initial version
June 8, 2013 - Signed all EXE files with a digital certificate to make Windows
               and malware detection programs happier. No functional changes.



===============================================================================
3. Overview of command line syntax
===============================================================================

I have made every effort to standardize the command line syntax for all the
tools in the toolbox.  The following rules apply to all tools.

- Running any tool with a /? option will display the full command line usage.
  For example, "detab /?" will display the complete help screen for DeTab.

- All command line options can be specified with either a leading / or -.
  For example, "detab /?" is equivalent to "detab -?"

- Options that do not take extra arguments can be grouped together.
  For example, "detab /s /h /r *" is equivalent to "detab /shr *"

- Options that do take extra arguments can be placed at the end of a grouping.
  For example, "detab /s /h /r /t3 *" is equivalent to "detab /shrt3 *"

- Spaces are optional between options and their arguments.
  For example, "detab /t3 *" is equivalent to "detab /t 3 *"

Many of the tools take one or more filespecs.  A filespec can be any
combination of a file, path, directory, or wildcard search string.  Here are
some example filespecs:

.                 matches all files in the current directory.
*                 matches all files in the current directory.
*.exe             matches all files with ".exe" extension in the current dir.
a*.xl?            matches all files in the current directory that start with
                  an "a" and end with ".xl" followed by any single character.
readme            matches a single file named "readme" in the current dir.
c:\bin\*          matches all files in the c:\bin directory.
c:\bin\           matches all files in the c:\bin directory.
c:\bin            matches all files in the c:\bin directory.
c:\bin\.          matches all files in the c:\bin directory.
c:\bin\readme     matches a single file named "readme" in the c:\bin dir.
d:*               matches all files in the d: drive's current directory.
\\server\share\*  matches all files on the specified network share.

All tools that take filespecs also support recursive directory processing.
Just add a /s to the command line of any tool, and that tool will expand its
file matching search to include all subdirectories under the initial directory
for that filespec.  For all the examples above, the /s option would repeat the
search described for every subdirectory under the starting directory.

All the tools are smart enough to break apart the path portion of the filespec
from the file or wildcard portion.  A filespec like "/s c:\bin\readme" will find
all files named "readme" in the c:\bin directory and in subdirectories under
the c:\bin directory.



===============================================================================
4. DeTab
===============================================================================

Description: DeTab replaces all tab characters in one or more files with 
             spaces.

-------------------------------------------------------------------------------

DeTab for Win32, Version 1.0

Usage: detab [/s] [/h] [/r] [/p] [/q] [/a] [/b] [/t size] filespec ...

  /s Process subdirectories.
  /h Process hidden/system files/directories.
  /r Convert read-only files.
  /p Prompt for each file to be converted (Yes/No/All/Quit).
  /q Quiet mode; Only display errors.
  /a Treat all files as text files.
  /b Create backup files.
  /t Tab size (default is 8).

By default, any non-text or binary files will be skipped.  Use the /a option
to override this behavior and force all files to be converted.



===============================================================================
5. ErrMsg
===============================================================================

Description: ErrMsg will display the system error message for one or more
             Win32 error values.

-------------------------------------------------------------------------------

ErrMsg for Win32, Version 1.0

Usage: errmsg value ...

ErrMsg will display the Windows error message for one or more error values.
These error values are usually obtained from the GetLastError() function.
The error values can be either in decimal, hexadecimal, or octal format.
All hexadecimal error values must be preceded with "0x".
All octal error values must begin with a leading zero "0".

Examples: errmsg 2
          errmsg 0x8001010D
          errmsg 044



===============================================================================
6. FileCase
===============================================================================

Description: FileCase will rename one or more file names and/or directory
             names to uppercase or lowercase.

-------------------------------------------------------------------------------

FileCase for Win32, Version 1.0

Usage: filecase [/s] [/h] [/p] [/q] [/d] [/l | /u] filespec ...

  /s Process subdirectories.
  /h Process hidden/system files/directories.
  /p Prompt for each file/directory to be renamed (Yes/No/All/Quit).
  /q Quiet mode; Only display errors.
  /d Rename directory names as well as file names.
  /l Convert names to lowercase (default).
  /u Convert names to uppercase.



===============================================================================
7. FileType
===============================================================================

Description: FileType displays the file type of one or more files.  For
             executable files, it will give details such as 16/32 bit, 
             console/GUI, CPU type, etc.  For all other files it will list
             the registered system description for that type of file.

-------------------------------------------------------------------------------

FileType for Win32, Version 1.0

Usage: filetype [/s] [/h] [/e] filespec ...

  /s Process subdirectories.
  /h Process hidden/system files/directories.
  /e Only display executable files.

-------------------------------------------------------------------------------

FileType displays detailed type information about one or more files.  While 
listing files, it automatically identifies which files are executables and 
which files are not.  FileType does not just look at the file extension; it
actually examines the contents of the file to determine if the file is an
executable and if so, what type of executable it is.  Therefore, FileType will
detect all executable files, regardless of their extension.

The following is a list of executable formats recognized by FileType.

Architecture: 16-bit, 32-bit
Platform:     DOS, Windows, Window CE, OS/2, POSIX
Subsystem:    native, console, graphical (GUI)
CPU:          x86, MIPS R3000, MIPS R4000, MIPS R10000, MIPS (big-endian),
              DEC Alpha, IBM PowerPC, Hitachi SH3

For files that are not executable, FileType will scan the registry for the
description that is registered with the system for that type of file.  For
example, "Microsoft Word Document" might be displayed for a file ending with
".doc".  These descriptions may vary from computer to computer, depending on
what software is installed.



===============================================================================
8. Global
===============================================================================

Description: Global makes any application or utility recursive.  It will 
             recursively execute any command in the current directory and in
             all subdirectories under the current directory.

-------------------------------------------------------------------------------

Global for Win32, Version 1.0

Usage: global [/h] [/p] [/q] [/i] command [args ...]

  /h Process hidden/system directories.
  /p Prompt for each directory to be processed (Yes/No/All/Quit).
  /q Quiet mode.  Does not display each directory name before processed.
  /i Ignore exit codes.  Default is to exit if command returns non-zero.

Global will execute any shell command, shell alias, or application, in the
current directory and in all subdirectories under the current directory.

Examples: global copy *.gif c:\gifs\
          global if exist README start notepad README
          global /hi grep "TO-DO" *.c* *.h* | more

-------------------------------------------------------------------------------

By default, Global quits whenever a command it runs returns a non-zero return
value.  A non-zero return value from an application usually signifies an error.
Use the /i option to force Global to ignore return values.  At any time while
Global is running, you can press CTRL+C or CTRL+Break to stop further directory
processing.

Note about piping and redirecting: The Global utility and the command it runs
are usually treated as a single unit by the shell.  This single unit is then
attached to any pipe or redirection that you may be using.  You may use quotes
to group the command with the pipe or redirection to override the default
behavior.  Here are some examples:

   global /hi grep BUG *.c* *.h* > bugs.txt

This example will create a single file named "bugs.txt" in the directory that
Global was executed.  To the shell, the command looks like the following:

   (global /hi grep BUG *.c* *.h*) > bugs.txt

If you would like the redirection to occur on a per directory basis, you can
group the command and the redirection in quotes.  The following example will
create a "bugs.txt" file in every directory that Global processes:

   global /hi "grep BUG *.c* *.h* > bugs.txt"

To the shell, this quoted command essentially looks like the following:

   global /hi (grep BUG *.c* *.h* > bugs.txt)



===============================================================================
9. NewLine
===============================================================================

Description: NewLine converts one or more text files to use a single LF or a
             CR/LF sequence to end each line in each file.

-------------------------------------------------------------------------------

NewLine for Win32, Version 1.0

Usage: newline [/s] [/h] [/r] [/p] [/q] [/a] [/b] [/1 | /2] filespec ...

  /s Process subdirectories.
  /h Process hidden/system files/directories.
  /r Convert read-only files.
  /p Prompt for each file to be converted (Yes/No/All/Quit).
  /q Quiet mode; Only display errors.
  /a Treat all files as text files.
  /b Create backup files.
  /1 Ensures that all lines end with a single LF.
  /2 Ensures that all lines end with a CR/LF (default).

By default, any non-text or binary files will be skipped.  Use the /a option to
override this behavior and force all files to be converted.  NewLine will not
modify any line in a file that already uses the requested newline sequence.



===============================================================================
10. ShutDown
===============================================================================

Description: ShutDown allows you to automatically log off, shut down, reboot,
             or power off your system.

-------------------------------------------------------------------------------

ShutDown for Win32, Version 1.0

Usage: shutdown /l | /s | /r | /p [/f] [/q]

  /l Log off only.
  /s Log off and shut down the system.
  /r Log off, shut down, and reboot the system.
  /p Log off, shut down, and power off the system (if supported).
  /f Force all applications to close, even at risk of loosing data (NT only).
  /q Quiet mode.  Do not prompt for confirmation.

ShutDown will attempt to safely close all running applications and flush all
file buffers.  Any applications that do not respond or refuse to close will
prevent logging off from completing.  The /f option will cause these blocked
applications to be forcefully terminated without prompting, possibly causing
any unsaved data in those applications to be lost.  Currently, the /f option
is not supported by Windows 95.



===============================================================================
11. Touch
===============================================================================

Description: Touch displays or sets the created, access, and modified times of
             one or more files.

-------------------------------------------------------------------------------

Touch for Win32, Version 1.0

Usage: touch [/s] [/h] [/p] [/q] [/v] [/c] [/m] [/a] [/r file]
             [/d mm-dd[-[cc]yy]] [/t hh[:mm[:ss[.ms]]]] filespec ...

  /s Process subdirectories.
  /h Process hidden/system files/directories.
  /p Prompt for each file (Yes/No/All/Quit). Ignored in view mode.
  /q Quiet mode; Only display errors; Ignored in view mode.
  /v View mode; Display times rather than set times.
  /c View/set created time.
  /m View/set modified time; Default if /a and /c are not specified.
  /a View/set accessed time.
  /r Reference file; Use this file's date and time to set other files.
  /d Date: month, day, century, year.
  /t Time: hour (0-24), minutes, seconds, milliseconds.

In view mode (/v), the /p, /q, /r, /d, and /t options are ignored.
In set mode, the default is to use the current date and time.
Use the /r, /d, and/or /t options to override the default set time.
The century (cc) is assumed to be 19 for years 69-99, and 20 for years 00-68.

-------------------------------------------------------------------------------

You may notice that you are unable to set the date and/or time of a file to
some exact setting.  It is possible that the operating system may need to round
a value you have specified up or down to overcome limitations in the file
system.  The following are some facts about FAT and NTFS:

FAT
   Created time is accurate to the nearest 10 milliseconds (rounded down).
   Modified time is accurate to the nearest 2 seconds (up on NT, down on 95).
   Accessed time is accurate to the nearest day (rounded down).
   Dates can range between 1/1/1980 and 12/31/2107
   Dates and times are physically stored as locally adjusted times.

NTFS
   Created time is accurate to the nearest 100 nanoseconds.
   Modified time is accurate to the nearest 100 nanoseconds.
   Accessed time is accurate to the nearest 100 nanoseconds.
   Dates can range between 1/1/1601 and 9/13/30828
   Dates and times are physically stored as UTC/GMT times.



===============================================================================
12. WebCat
===============================================================================

Description: WebCat quickly generates an HTML page to catalog all your images,
             animated icons, and other web related files.  Now you can easily
             use your web browser to catalog all your content on one page.

-------------------------------------------------------------------------------

WebCat for Win32, Version 1.0

Usage: webcat [/s] [/h] [/i] [/f outfile] filespec ... [> outfile]

  /s Process subdirectories.
  /h Process hidden/system files/directories.
  /i Only catalog image files.  Usually used with a filespec of "*".
  /f File to save output to.  Default is to write output to console.

WebCat will output HTML source containing links to all the files matching
the listed filespecs.  Files known to be images will be displayed as such.
Image files include gif, jpg, jpeg, bmp, dib, pcx, png, tga, xbm.
