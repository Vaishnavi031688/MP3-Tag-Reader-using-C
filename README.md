# MP3-Tag-Reader-using-C

**Project Description**
The MP3 Tag Reader project is a lightweight C-based command-line tool that reads and edits metadata stored in MP3 files. It can display important tag information such as title, artist, album, year, genre, and comment, and also lets users update selected tags. The project is useful for quickly viewing or modifying MP3 file details without affecting the audio itself.

This project is a simple MP3 tag reader/writer built in C. It reads metadata (ID3 tags) from an MP3 file and displays details like:
Title
Artist
Album
Year
Genre
Comment

It also supports editing some of these fields through command-line options. For example:
-v sample.mp3 → view tags
-e -t "New Title" sample.mp3 → edit the title
-e -a "New Artist" sample.mp3 → edit the artist
The project is structured around:

main.c: handles command-line arguments and calls functions
view.c: reads and displays tag frames
mp3.h: contains constants and function declarations
sample.mp3: sample MP3 file used for testing
In short, it is a lightweight command-line utility for inspecting and modifying MP3 metadata without using a GUI.

**Topics covered**
C programming basics
File handling in C
Reading binary data from MP3 files
Working with ID3 tags
Parsing tag frames and metadata
String handling and memory operations
Command-line argument processing
Editing and updating file content
Data interpretation for text and UTF-16 formats

**Learning progress**
Learned how to read and interpret structured binary data from files
Understood how MP3 metadata is stored in ID3 tags
Improved skills in parsing custom file formats
Gained hands-on experience in command-line tool development
Strengthened understanding of C functions, pointers, and file I/O
Practiced debugging and managing project structure in C
