/*****************************************************************************/
/* VGM2PSG PSG Compression function                                          */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Include files
#include <stdio.h>
#include <string.h>
#include <Main.h>
#include <filePSG.h>
#include <filePSGCompress.h>

///////////////////////////////////////////////////////////////////////////////
// Defines
#define CHARACTER_COUNT 256

#define PSG_SUBSTRING									0x08
#define PSG_SUBSTRING_MIN_LEN         4
#define PSG_SUBSTRING_MAX_LEN         51        // 47+4

#define PSG_CBS_DATA				0
#define PSG_CBS_REFERENCED	1
#define PSG_CBS_COMPRESSED	-1


///////////////////////////////////////////////////////////////////////////////
// Types

///////////////////////////////////////////////////////////////////////////////
// Local functions


///////////////////////////////////////////////////////////////////////////////
// Module global variables
uint8_t l_buffer1[FILE_BUFFER_LENGTH];

///////////////////////////////////////////////////////////////////////////////
// Compresses the buffer
int filePSGCompress(uint8_t* in_buffer, int in_buffer_length)
{
  if (in_buffer_length < 4)
    return in_buffer_length;

  if (in_buffer[in_buffer_length - 1] != 0)
    return 0;

  memcpy(l_buffer1, in_buffer, 4);

  int i = 0;
  int j = 0;
  while (i < in_buffer_length && in_buffer[i] > 0)
  {
    if (in_buffer[i] < 56 && in_buffer[i] >= 8)
      return in_buffer_length;

    int k = 0;
    uint8_t b1 = 0;
    uint8_t b2 = 0;
    int m = 0;
    int n = -1;
    while (m + b1 < j && b2 < 51 && in_buffer[i + b2] > 0)
    {
      if (l_buffer1[m + b2] < 56 && l_buffer1[m + b2] >= 8)
      {
        int substring_length = l_buffer1[m + b2] - 4;
        int substring_offset = l_buffer1[m + b2 + 2] << 8 | l_buffer1[m + b2 + 1];
        uint8_t b3 = b2;
        bool found = true;
        uint8_t b4;
        for (b4 = 0; found && b4 < substring_length && i + b2 + b4 < in_buffer_length; b4++)
        {
          if (l_buffer1[substring_offset + b4] != in_buffer[i + b2 + b4] || b3 >= 51)
          {
            found = false;
          }
          else
          {
            b3++;
          }
        }
        if (found)
          for (b4 = 0; i + b2 + substring_length + b4 < in_buffer_length && b3 < 51 && l_buffer1[m + b2 + 3 + b4] == in_buffer[i + b2 + substring_length + b4]; b4++)
            b3++;
        if (b3 + 3 - substring_length > b1)
        {
          n = m + b2;
          k = m;
          b1 = b3;
        }
        else if (b2 > b1)
        {
          n = -1;
          k = m;
          b1 = b2;
        }
        if (b2 == 0)
        {
          m += 3;
        }
        else
        {
          m++;
        }
        b2 = 0;
        continue;
      }
      if (l_buffer1[m + b2] != in_buffer[i + b2])
      {
        if (b2 > b1)
        {
          n = -1;
          k = m;
          b1 = b2;
        }
        m++;
        b2 = 0;
        continue;
      }
      b2++;
    }
    if (b1 >= 4)
    {
      int i1 = k;
      uint8_t b = b1;
      int i2 = n;
      int i3 = i;
      int i4;
      for (i4 = i + 1; i4 < i + b1; i4++)
      {
        b2 = 0;
        m = 0;
        while (m + b < j && b2 < 51 && in_buffer[i + b2] > 0)
        {
          if (l_buffer1[m + b2] < 56 && l_buffer1[m + b2] >= 8)
          {
            int i5 = l_buffer1[m + b2] - 4;
            int i6 = l_buffer1[m + b2 + 2] << 8 | l_buffer1[m + b2 + 1];
            uint8_t b3 = b2;
            bool found = true;
            uint8_t b4;
            for (b4 = 0; found&& b4 < i5 && i + b2 + b4 < in_buffer_length; b4++)
            {
              if (l_buffer1[i6 + b4] != in_buffer[i + b2 + b4] || b3 >= 51)
              {
                found = false;
              }
              else
              {
                b3++;
              }
            }
            if (found)
              for (b4 = 0; i + b2 + i5 + b4 < in_buffer_length && b3 < 51 && l_buffer1[m + b2 + 3 + b4] == in_buffer[i + b2 + i5 + b4]; b4++)
                b3++;
            if (b3 + 3 - i5 > b)
            {
              i2 = m + b2;
              i1 = m;
              b = b3;
              i3 = i4;
            }
            else if (b2 > b)
            {
              i2 = -1;
              i1 = m;
              b = b2;
              i3 = i4;
            }
            if (b2 == 0)
            {
              m += 3;
            }
            else
            {
              m++;
            }
            b2 = 0;
            continue;
          }
          if (l_buffer1[m + b2] != in_buffer[i4 + b2])
          {
            if (b2 > b)
            {
              i2 = -1;
              i1 = m;
              b = b2;
              i3 = i4;
            }
            m++;
            b2 = 0;
            continue;
          }
          b2++;
        }
      }
      if (b > b1)
      {
        while (i < i3)
          l_buffer1[j++] = in_buffer[i++];
        k = i1;
        b1 = b;
        n = i2;
      }
      if (n >= 0)
      {
        i4 = l_buffer1[n] - 4;
        int i5 = l_buffer1[n + 2] << 8 | l_buffer1[n + 1];
        memcpy(&l_buffer1[n + i4], &l_buffer1[n + 3], j - n + 3);
        memcpy(&l_buffer1[n], &l_buffer1[i5], i4);
        j += i4 - 3;
        for (m = n + i4; m < j; m++)
        {
          if (l_buffer1[m] < 56 && l_buffer1[m] >= 8)
          {
            i5 = l_buffer1[m + 2] << 8 | l_buffer1[m + 1];
            if (i5 > n)
            {
              i5 += i4 - 3;
              l_buffer1[m + 1] = i5 & 0xFF;
              l_buffer1[m + 2] = i5 >> 8;
            }
            m += 2;
          }
        }
        if (k > n)
          k += i4 - 3;
      }
      if (b1 > 51)
      {
        printf("Match was too long.");
        b1 = 51;
      }
      l_buffer1[j++] = 8 + b1 - 4;
      l_buffer1[j++] = k & 0xFF;
      l_buffer1[j++] = k >> 8;
      i += b1;
      continue;
    }
    l_buffer1[j++] = in_buffer[i++];
  }
  l_buffer1[j++] = in_buffer[i++];

  memcpy(in_buffer, l_buffer1, j);

  return j;
}
