#include "mp3.h"


static unsigned int get_tag_size(const unsigned char size[4])
{
    return ((unsigned int)(size[0] & 0x7F) << 21) |
           ((unsigned int)(size[1] & 0x7F) << 14) |
           ((unsigned int)(size[2] & 0x7F) << 7)  |
           ((unsigned int)(size[3] & 0x7F));
}


static unsigned int get_frame_size(const unsigned char size[4])
{
    return ((unsigned int)size[0] << 24) |
           ((unsigned int)size[1] << 16) |
           ((unsigned int)size[2] << 8)  |
           ((unsigned int)size[3]);
}


static void print_codepoint(unsigned int codepoint)
{
    if (codepoint <= 0x7F)
    {
        putchar((int)codepoint);
    }
    else if (codepoint <= 0x7FF)
    {
        putchar((int)(0xC0 | (codepoint >> 6)));
        putchar((int)(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint <= 0xFFFF)
    {
        putchar((int)(0xE0 | (codepoint >> 12)));
        putchar((int)(0x80 | ((codepoint >> 6) & 0x3F)));
        putchar((int)(0x80 | (codepoint & 0x3F)));
    }
    else
    {
        putchar((int)(0xF0 | (codepoint >> 18)));
        putchar((int)(0x80 | ((codepoint >> 12) & 0x3F)));
        putchar((int)(0x80 | ((codepoint >> 6) & 0x3F)));
        putchar((int)(0x80 | (codepoint & 0x3F)));
    }
}

static unsigned int read_utf16_unit(const unsigned char *data,
                                    unsigned int offset,
                                    int little_endian)
{
    if (little_endian)
        return (unsigned int)data[offset] |
               ((unsigned int)data[offset + 1] << 8);

    return ((unsigned int)data[offset] << 8) |
           (unsigned int)data[offset + 1];
}

static void print_utf16_value(const unsigned char *data,
                              unsigned int size,
                              unsigned int offset,
                              int little_endian)
{
    if (offset + 1 < size &&
        read_utf16_unit(data, offset, little_endian) == 0xFEFF)
        offset += 2;

    while (offset + 1 < size)
    {
        unsigned int codepoint = read_utf16_unit(data, offset, little_endian);
        offset += 2;

        if (codepoint == 0)
            break;

        if (codepoint >= 0xD800 && codepoint <= 0xDBFF &&
            offset + 1 < size)
        {
            unsigned int low = read_utf16_unit(data, offset, little_endian);

            if (low >= 0xDC00 && low <= 0xDFFF)
            {
                codepoint = 0x10000 + ((codepoint - 0xD800) << 10) +
                            (low - 0xDC00);
                offset += 2;
            }
            else
            {
                codepoint = 0xFFFD;
            }
        }
        else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF)
        {
            codepoint = 0xFFFD;
        }

        if (codepoint >= 32 && codepoint != 127)
            print_codepoint(codepoint);
    }
}

static void print_text_value(const unsigned char *data, unsigned int size)
{
    unsigned int i;

    if (size == 0)
        return;

    if (data[0] == 1)
    {
        int little_endian = size < 3 ||
                            !(data[1] == 0xFE && data[2] == 0xFF);

        print_utf16_value(data, size, size >= 3 ? 3 : size, little_endian);
    }
    else
    {
        for (i = 1; i < size && data[i] != 0; i++)
        {
            if (data[i] >= 32 && data[i] != 127)
                print_codepoint(data[i]);
        }
    }
}

/*
 * COMMENT frame:
 * Encoding(1) + Language(3) + Description + Text
 */
static void print_comment(const unsigned char *data, unsigned int size)
{
    unsigned int i;

    if (size <= 4)
        return;

    if (data[0] == 1)
    {
        int little_endian = 1;

        i = 4;
        if (i + 1 < size && data[i] == 0xFE && data[i + 1] == 0xFF)
        {
            little_endian = 0;
            i += 2;
        }
        else if (i + 1 < size &&
                 data[i] == 0xFF && data[i + 1] == 0xFE)
        {
            i += 2;
        }

        while (i + 1 < size &&
               read_utf16_unit(data, i, little_endian) != 0)
            i += 2;

        if (i + 1 < size)
            i += 2;

        print_utf16_value(data, size, i, little_endian);
    }
    else
    {
        i = 4;

        while (i < size && data[i] != 0)
        {
            if (data[i] >= 32 && data[i] != 127)
                print_codepoint(data[i]);
            i++;
        }

        while (i < size && data[i] == 0)
            i++;

        while (i < size && data[i] != 0)
        {
            if (data[i] >= 32 && data[i] != 127)
                print_codepoint(data[i]);
            i++;
        }
    }
}

/* Displays the value of one frame. */
static void display_frame(const char *id,
                          const unsigned char *data,
                          unsigned int size)
{
    if (strcmp(id, TITLE) == 0)
    {
        printf("Title   : ");
        print_text_value(data, size);
        printf("\n");
    }
    else if (strcmp(id, ARTIST) == 0)
    {
        printf("Artist  : ");
        print_text_value(data, size);
        printf("\n");
    }
    else if (strcmp(id, ALBUM) == 0)
    {
        printf("Album   : ");
        print_text_value(data, size);
        printf("\n");
    }
    else if (strcmp(id, YEAR) == 0)
    {
        printf("Year    : ");
        print_text_value(data, size);
        printf("\n");
    }
    else if (strcmp(id, GENRE) == 0)
    {
        printf("Genre   : ");
        print_text_value(data, size);
        printf("\n");
    }
    else if (strcmp(id, TRACK) == 0)
    {
        printf("Track   : ");
        print_text_value(data, size);
        printf("\n");
    }
    else if (strcmp(id, COMMENT) == 0)
    {
        printf("Comment : ");
        print_comment(data, size);
        printf("\n");
    }
}

/* Read and display ID3v2.3 information. */
int view_tags(const char *filename)
{
    FILE *fp;
    unsigned char header[ID3_HEADER_SIZE];
    unsigned int tag_size;
    unsigned int bytes_read = 0;

    fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        perror("Error opening MP3 file");
        return 1;
    }

    if (fread(header, 1, ID3_HEADER_SIZE, fp) != ID3_HEADER_SIZE)
    {
        printf("Error: Unable to read ID3 header.\n");
        fclose(fp);
        return 1;
    }

    /* Checks the first three bytes: I D 3 */
    if (header[0] != 'I' ||
        header[1] != 'D' ||
        header[2] != '3')
    {
        printf("Error: No ID3 tag found.\n");
        fclose(fp);
        return 1;
    }

    printf("\n-------------------------------\n");
    printf("       MP3 Tag Information\n");
    printf("-------------------------------\n");

    printf("ID3 Version : %d.%d\n",
           header[3], header[4]);

    if (header[3] != 3)
    {
        printf("This version is not handled by this v2.3 reader.\n");
        fclose(fp);
        return 1;
    }

    tag_size = get_tag_size(&header[6]);

    printf("Tag Size    : %u bytes\n", tag_size);
    printf("-------------------------------\n");

    if ((header[5] & 0x40) != 0)
    {
        unsigned char extended_size_bytes[4];
        unsigned int extended_size;
        unsigned int remaining;
        unsigned char buffer[4096];

        if (tag_size < sizeof(extended_size_bytes) ||
            fread(extended_size_bytes, 1, sizeof(extended_size_bytes), fp) !=
                sizeof(extended_size_bytes))
        {
            fprintf(stderr, "Error: Invalid ID3 extended header.\n");
            fclose(fp);
            return 1;
        }

        bytes_read = sizeof(extended_size_bytes);
        extended_size = ((unsigned int)extended_size_bytes[0] << 24) |
                        ((unsigned int)extended_size_bytes[1] << 16) |
                        ((unsigned int)extended_size_bytes[2] << 8) |
                        (unsigned int)extended_size_bytes[3];

        if (extended_size < 6 || extended_size > tag_size - bytes_read)
        {
            fprintf(stderr, "Error: Invalid ID3 extended header size.\n");
            fclose(fp);
            return 1;
        }

        remaining = extended_size;
        while (remaining > 0)
        {
            size_t chunk = remaining < sizeof(buffer) ?
                           remaining : sizeof(buffer);
            size_t count = fread(buffer, 1, chunk, fp);

            if (count != chunk)
            {
                fprintf(stderr, "Error reading ID3 extended header.\n");
                fclose(fp);
                return 1;
            }

            remaining -= (unsigned int)count;
        }
        bytes_read += extended_size;
    }

    while (bytes_read + FRAME_HEADER_SIZE <= tag_size)
    {
        unsigned char frame_header[FRAME_HEADER_SIZE];
        unsigned int frame_size;
        unsigned char *frame_data;

        if (fread(frame_header, 1, FRAME_HEADER_SIZE, fp) !=
            FRAME_HEADER_SIZE)
        {
            break;
        }

        bytes_read += FRAME_HEADER_SIZE;

        /* Padding starts with zero bytes. */
        if (frame_header[0] == 0)
            break;

        frame_size = get_frame_size(&frame_header[4]);

        if (frame_size == 0)
            break;

        if (frame_size > tag_size - bytes_read)
        {
            printf("Error: Invalid frame size.\n");
            break;
        }

        frame_data = malloc(frame_size);

        if (frame_data == NULL)
        {
            printf("Memory allocation failed.\n");
            fclose(fp);
            return 1;
        }

        if (fread(frame_data, 1, frame_size, fp) != frame_size)
        {
            free(frame_data);
            break;
        }

        bytes_read += frame_size;

      
        {
            char frame_id[5];

            memcpy(frame_id, frame_header, 4);
            frame_id[4] = '\0';

            display_frame(frame_id, frame_data, frame_size);
        }

        free(frame_data);
    }

    printf("-------------------------------\n");

    fclose(fp);
    return 0;
}

void display_help(void)
{
    printf("\nMP3 Tag Reader\n");
    printf("-------------------------------\n");
    printf("Usage:\n");
    printf("  ./mp3reader -h\n");
    printf("  ./mp3reader -v <mp3_file>\n");
    printf("  ./mp3reader -e <-t|-a|-A|-y|-G|-c> \"value\" <mp3_file>\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h          Display help\n");
    printf("  -v          View MP3 tag information\n");
    printf("  -e -t       Edit Title\n");
    printf("  -e -a       Edit Artist\n");
    printf("  -e -A       Edit Album\n");
    printf("  -e -y       Edit Year\n");
    printf("  -e -G       Edit Genre\n");
    printf("  -e -c       Edit Comment\n");
    printf("\n");
}
