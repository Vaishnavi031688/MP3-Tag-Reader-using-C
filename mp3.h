#ifndef MP3_H
#define MP3_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_ID_SIZE 4
#define FRAME_HEADER_SIZE 10
#define ID3_HEADER_SIZE 10

#define TITLE   "TIT2"
#define ARTIST  "TPE1"
#define ALBUM   "TALB"
#define YEAR    "TYER"
#define GENRE   "TCON"
#define TRACK   "TRCK"
#define COMMENT "COMM"

/* ID3v2.3 header */
typedef struct
{
    char id[3];
    unsigned char version[2];
    unsigned char flags;
    unsigned char size[4];
} ID3Header;

/* ID3v2.3 frame header */
typedef struct
{
    char id[5];
    unsigned char size[4];
    unsigned char flags[2];
} FrameHeader;

/* Functions used by main.c */
void display_help(void);
int view_tags(const char *filename);

/* Functions used by edit.c */
int edit_tag(const char *filename, const char *frame_id, const char *new_value);

#endif
