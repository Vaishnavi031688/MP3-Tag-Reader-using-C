#include "mp3.h"

/* ID3 stores the tag size as four 7-bit numbers. */
static unsigned int get_tag_size(const unsigned char size[4])
{
    return ((unsigned int)(size[0] & 0x7F) << 21) |
           ((unsigned int)(size[1] & 0x7F) << 14) |
           ((unsigned int)(size[2] & 0x7F) << 7) |
           (unsigned int)(size[3] & 0x7F);
}

static void set_tag_size(unsigned char size[4], unsigned int value)
{
    size[0] = (value >> 21) & 0x7F;
    size[1] = (value >> 14) & 0x7F;
    size[2] = (value >> 7) & 0x7F;
    size[3] = value & 0x7F;
}

static void set_frame_size(unsigned char size[4], unsigned int value)
{
    size[0] = (value >> 24) & 0xFF;
    size[1] = (value >> 16) & 0xFF;
    size[2] = (value >> 8) & 0xFF;
    size[3] = value & 0xFF;
}

/*
 * Make the value for a normal text frame.
 * This simple version supports basic English characters (ASCII).
 */
static unsigned char *make_text_data(const char *value,
                                     unsigned int *data_size)
{
    unsigned int length = (unsigned int)strlen(value);
    unsigned int i;
    unsigned char *data;

    *data_size = 3 + length * 2;
    data = malloc(*data_size);

    if (data == NULL)
        return NULL;

    data[0] = 1;       /* UTF-16 text */
    data[1] = 0xFF;    /* Little-endian byte order mark */
    data[2] = 0xFE;

    for (i = 0; i < length; i++)
    {
        if ((unsigned char)value[i] > 127)
        {
            free(data);
            return NULL;
        }

        data[3 + i * 2] = (unsigned char)value[i];
        data[4 + i * 2] = 0;
    }

    return data;
}

/* Make the value for a comment frame. */
static unsigned char *make_comment_data(const char *value,
                                        unsigned int *data_size)
{
    unsigned int length = (unsigned int)strlen(value);
    unsigned int i;
    unsigned char *data;

    *data_size = 5 + length;
    data = malloc(*data_size);

    if (data == NULL)
        return NULL;

    data[0] = 0;       /* Text is stored as basic English characters. */
    data[1] = 'e';
    data[2] = 'n';
    data[3] = 'g';
    data[4] = 0;       /* Empty description. */

    for (i = 0; i < length; i++)
    {
        if ((unsigned char)value[i] > 127)
        {
            free(data);
            return NULL;
        }

        data[5 + i] = (unsigned char)value[i];
    }

    return data;
}

int edit_tag(const char *filename, const char *frame_id, const char *new_value)
{
    FILE *source = NULL;
    FILE *temporary = NULL;
    unsigned char header[10];
    unsigned char *old_tag = NULL;
    unsigned char *new_tag = NULL;
    unsigned char *new_data = NULL;
    unsigned int old_tag_size;
    unsigned int new_tag_size;
    unsigned int frame_position = 0;
    unsigned int old_frame_size = 0;
    unsigned int new_data_size;
    int frame_found = 0;
    int temporary_created = 0;
    int result = 1;
    char temporary_name[1024];
    int name_length;

    source = fopen(filename, "rb");
    if (source == NULL)
    {
        perror("Error opening MP3 file");
        goto cleanup;
    }

    if (fread(header, 1, sizeof(header), source) != sizeof(header))
    {
        fprintf(stderr, "Error: Could not read the ID3 header.\n");
        goto cleanup;
    }

    if (memcmp(header, "ID3", 3) != 0)
    {
        fprintf(stderr, "Error: No ID3 tag found.\n");
        goto cleanup;
    }

    if (header[3] != 3)
    {
        fprintf(stderr, "Error: Only ID3v2.3 files can be edited.\n");
        goto cleanup;
    }

    if ((header[5] & 0xC0) != 0)
    {
        fprintf(stderr,
                "Error: This file uses ID3 features that this simple editor does not support.\n");
        goto cleanup;
    }

    old_tag_size = get_tag_size(&header[6]);
    old_tag = malloc(old_tag_size == 0 ? 1 : old_tag_size);

    if (old_tag == NULL)
    {
        fprintf(stderr, "Error: Not enough memory for the ID3 tag.\n");
        goto cleanup;
    }

    if (fread(old_tag, 1, old_tag_size, source) != old_tag_size)
    {
        fprintf(stderr, "Error: Could not read the complete ID3 tag.\n");
        goto cleanup;
    }

    /* Find the frame to change. Each frame starts with a 10-byte header. */
    while (old_tag_size - frame_position >= 10)
    {
        unsigned int frame_size =
            ((unsigned int)old_tag[frame_position + 4] << 24) |
            ((unsigned int)old_tag[frame_position + 5] << 16) |
            ((unsigned int)old_tag[frame_position + 6] << 8) |
            (unsigned int)old_tag[frame_position + 7];

        if (old_tag[frame_position] == 0)
            break;  /* Zero bytes mark the end of the frames. */

        if (frame_size > old_tag_size - frame_position - 10)
        {
            fprintf(stderr, "Error: The MP3 has an invalid frame size.\n");
            goto cleanup;
        }

        if (memcmp(&old_tag[frame_position], frame_id, 4) == 0)
        {
            frame_found = 1;
            old_frame_size = frame_size;
            break;
        }

        frame_position += 10 + frame_size;
    }

    if (!frame_found)
    {
        fprintf(stderr, "Error: Frame %s was not found.\n", frame_id);
        goto cleanup;
    }

    /* Do not change a frame that is compressed, encrypted, or grouped. */
    if ((old_tag[frame_position + 9] & 0xE0) != 0)
    {
        fprintf(stderr, "Error: This frame uses unsupported options.\n");
        goto cleanup;
    }

    if (strcmp(frame_id, COMMENT) == 0)
        new_data = make_comment_data(new_value, &new_data_size);
    else
        new_data = make_text_data(new_value, &new_data_size);

    if (new_data == NULL)
    {
        fprintf(stderr,
                "Error: Could not make the new tag. Use basic English characters.\n");
        goto cleanup;
    }

    new_tag_size = old_tag_size - old_frame_size + new_data_size;

    /* The ID3 size field can store at most 28 bits. */
    if (new_tag_size > 0x0FFFFFFF)
    {
        fprintf(stderr, "Error: The updated ID3 tag is too large.\n");
        goto cleanup;
    }

    new_tag = malloc(new_tag_size == 0 ? 1 : new_tag_size);
    if (new_tag == NULL)
    {
        fprintf(stderr, "Error: Not enough memory for the updated tag.\n");
        goto cleanup;
    }

    /* Copy the frames before the changed frame. */
    memcpy(new_tag, old_tag, frame_position);

    /* Write the changed frame's ID, size, flags, and new value. */
    memcpy(&new_tag[frame_position], frame_id, 4);
    set_frame_size(&new_tag[frame_position + 4], new_data_size);
    new_tag[frame_position + 8] = old_tag[frame_position + 8];
    new_tag[frame_position + 9] = old_tag[frame_position + 9];
    memcpy(&new_tag[frame_position + 10], new_data, new_data_size);

    /* Copy the frames after the changed frame. */
    {
        unsigned int old_end = frame_position + 10 + old_frame_size;
        unsigned int new_end = frame_position + 10 + new_data_size;
        unsigned int bytes_after = old_tag_size - old_end;

        memcpy(&new_tag[new_end], &old_tag[old_end], bytes_after);
    }

    set_tag_size(&header[6], new_tag_size);

    name_length = snprintf(temporary_name,
                           sizeof(temporary_name),
                           "%s.tmp",
                           filename);
    if (name_length < 0 || (size_t)name_length >= sizeof(temporary_name))
    {
        fprintf(stderr, "Error: The MP3 file path is too long.\n");
        goto cleanup;
    }

    temporary = fopen(temporary_name, "wb");
    if (temporary == NULL)
    {
        perror("Error creating temporary file");
        goto cleanup;
    }
    temporary_created = 1;

    if (fwrite(header, 1, sizeof(header), temporary) != sizeof(header) ||
        fwrite(new_tag, 1, new_tag_size, temporary) != new_tag_size)
    {
        fprintf(stderr, "Error writing the updated ID3 tag.\n");
        goto cleanup;
    }

    /* Copy the audio after the ID3 tag without changing it. */
    {
        unsigned char buffer[4096];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            if (fwrite(buffer, 1, bytes_read, temporary) != bytes_read)
            {
                fprintf(stderr, "Error copying the audio data.\n");
                goto cleanup;
            }
        }

        if (ferror(source))
        {
            fprintf(stderr, "Error reading the audio data.\n");
            goto cleanup;
        }
    }

    if (fclose(temporary) != 0)
    {
        temporary = NULL;
        perror("Error finishing the temporary file");
        goto cleanup;
    }
    temporary = NULL;

    if (fclose(source) != 0)
    {
        source = NULL;
        perror("Error closing the original MP3 file");
        goto cleanup;
    }
    source = NULL;

    /*
     * rename replaces the old file on systems such as Linux and WSL.
     * The original file is left alone if the rename fails.
     */
    if (rename(temporary_name, filename) != 0)
    {
        perror("Error replacing the MP3 file");
        goto cleanup;
    }
    temporary_created = 0;

    printf("Tag edited successfully.\n");
    printf("Frame : %s\n", frame_id);
    printf("Value : %s\n", new_value);
    result = 0;

cleanup:
    if (source != NULL)
        fclose(source);
    if (temporary != NULL)
        fclose(temporary);
    if (temporary_created)
        remove(temporary_name);

    free(new_data);
    free(new_tag);
    free(old_tag);
    return result;
}
