#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE* fIN;
FILE* fOUT;

#define BUF_SIZE        65536
#define MIN_LEN         4
#define MAX_LEN         51        // 47+4

#define PSG_SUBSTRING   0x08

unsigned char buf[BUF_SIZE];

int size;


int main(int argc, char* argv[])
{

  int i, offset;
  int output_size = 0;
  int length;

  if (argc != 3)
  {
    printf("Usage: psgdecomp inputfile.PSG outputfile.PSG\n");
    return (1);
  }

  fIN = fopen(argv[1], "rb");
  size = (int)fread(&buf, 1, BUF_SIZE, fIN);      // read input file
  fclose(fIN);

  printf("Info: input file size is %d bytes\n", size);
  fOUT = fopen(argv[2], "wb");

  for (i = 0; i < size; i++)
  {
    if ((buf[i] >= PSG_SUBSTRING) && (buf[i] <= PSG_SUBSTRING + MAX_LEN - MIN_LEN))
    {

      offset = buf[i + 1] + (buf[i + 2] * 256);
      length = buf[i] - PSG_SUBSTRING + MIN_LEN;
      output_size += length;
      fwrite(&buf[offset], 1, length, fOUT);
      i += 2;  // skip two additional bytes
    }
    else
    {
      fwrite(&buf[i], 1, 1, fOUT);
      output_size++;
    }

  }

  fclose(fOUT);

  printf("%d bytes written\n", output_size);
  printf("Info: done!\n");
  return(0);
}