#include "fat.hpp"

static void GetString(char *dst, const char *src, int count, bool allowSpaces);

void GetName(char dst[MAX_NAME], const char *src)
{
    GetString(dst, src, NAME_LENGTH, false);
}

void GetExtension(char dst[MAX_EXTENSION], const char *src)
{
    GetString(dst, src, EXTENSION_LENGTH, false);
}

void GetLabel(char dst[MAX_LABEL], const char *src)
{
    GetString(dst, src, LABEL_LENGTH, true);
}

static void GetString(char *dst, const char *src, int count, bool allowSpaces)
{
    if (!allowSpaces)
    {
        int i = 0;
        for (; i < count; i++)
        {
            if (src[i] == ' ' || src[i] == '\0')
            {
                break;
            }
            dst[i] = src[i];
        }
        dst[i] = '\0';
    }
    else
    {
        // Trim leading/trailing space only

        int beg = -1;
        int end = 0;
        int i;

        for (i = 0; i < count; i++)
        {
            if (src[i] != ' ')
            {
                beg = i;
                break;
            }
        }

        for (i = count - 1; i >= 0; i--)
        {
            if (src[i] != ' ')
            {
                end = i;
                break;
            }
        }

        for (i = 0; i < end - beg + 1; i++)
        {
            dst[i] = src[beg + i];
        }
        dst[i] = '\0';
    }
}
