#include "mp3.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        display_help();
        return 0;
    }

    /*
     * ./mp3reader -h
     * ./mp3reader -v sample.mp3
     * ./mp3reader -e -t "New Title" sample.mp3
     */
    if (strcmp(argv[1], "-h") == 0)
    {
        display_help();
    }
    else if (strcmp(argv[1], "-v") == 0)
    {
        if (argc != 3)
        {
            printf("Usage: %s -v <mp3_file>\n", argv[0]);
            return 1;
        }

        return view_tags(argv[2]);
    }
    else if (strcmp(argv[1], "-e") == 0)
    {
        if (argc != 5)
        {
            printf("Usage: %s -e <-t|-a|-A|-y|-G|-c> \"value\" <mp3_file>\n",
                   argv[0]);
            return 1;
        }

        if (strcmp(argv[2], "-t") == 0)
            return edit_tag(argv[4], TITLE, argv[3]);

        if (strcmp(argv[2], "-a") == 0)
            return edit_tag(argv[4], ARTIST, argv[3]);

        if (strcmp(argv[2], "-A") == 0)
            return edit_tag(argv[4], ALBUM, argv[3]);

        if (strcmp(argv[2], "-y") == 0)
            return edit_tag(argv[4], YEAR, argv[3]);

        if (strcmp(argv[2], "-G") == 0)
            return edit_tag(argv[4], GENRE, argv[3]);

        if (strcmp(argv[2], "-c") == 0)
            return edit_tag(argv[4], COMMENT, argv[3]);

        printf("Invalid edit option.\n");
        return 1;
    }
    else
    {
        printf("Invalid option: %s\n", argv[1]);
        printf("Use -h for help.\n");
        return 1;
    }

    return 0;
}
