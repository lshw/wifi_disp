#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

uint32_t crc32c ( uint8_t *message, uint32_t bytes)
{
  uint8_t Byte;
  uint32_t mask;
  uint32_t index;
  int8_t shift;
  uint32_t crc=0xffffffff;
  for ( index = 0 ; index < bytes ; index++ )
  {
    // Get Next Byte
    Byte = message[index];
    // XOR with Next Byte
    crc = crc ^ Byte;
    // Shift
    for (shift = 7; shift >= 0; shift--)
    {
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

void crc32_gen_array( uint32_t crc, int32_t pos, uint8_t *buf, int32_t len )
{
  assert (pos >= 0);
  assert (buf != 0);
  assert (pos + 4 <= len);
  uint32_t rem = 0xFFFFFFFF;
  for (uint32_t i = 0; i < pos; ++i)
  {
    rem ^= buf[i];
    for (int j = 0; j < 8; ++j)
    {
      rem = (rem >> 1) ^ (rem & 0x00000001 ? 0xEDB88320 : 0);
    }
  }
  for (int i = 0; i < 4; ++i)
  {
    buf[pos + i] = (rem >> (8 * i)) & 0xFF;
  }
  rem = ~crc;
  for (uint32_t i = len - 1; i >= pos; --i)
  {
    for (int j = 0; j < 8; ++j)
    {
      rem = (rem << 1) ^ (rem & 0x80000000 ? 0xDB710641 : 0);
    }
    rem ^= buf[i];
  }
  for (int i = 0; i < 4; ++i)
  {
    buf[pos + i] = (rem >> (8 * i)) & 0xFF;
  }
  return;
}
void main(int argc,char * argv[])
{
  FILE * fp;
  uint32_t crc32;
  char crc32_str[10];
  //  char buf[500000];
  if(argc!=3) {
    printf("%s filename crc32_value\r\n",argv[0]);
    return;
  }
  crc32=atol(argv[2]);
  sprintf(crc32_str,"%ld",crc32);
  if(strcmp(crc32_str,argv[2])!=0) {
    printf("crc32_value [%s] is not a number\r\n",argv[2]);
    return;
  }
  if(access(argv[1],R_OK)!=0) {
    printf("file [%s] read error\r\n",argv[1]);
    return;
  }
  struct stat statbuf;
  stat(argv[1],&statbuf);
  uint32_t size=statbuf.st_size;
  printf("size=%d\r\n",size);
  char *buf;
  buf=malloc(size+4);
  fp=fopen(argv[1],"a+");
  if(!fp) {
    printf("fopen error\r\n");
    free(buf);
    return;
  }
  uint32_t size0;
  size0=fread(buf,1,size,fp);

  if(crc32c(buf,size)!=crc32)
  {
    crc32_gen_array (crc32, size, buf, size+4);
    for (uint32_t i = size; i < size+4; i++)
    {
      printf ("%02X", (uint8_t)buf[i]);
    }
    printf(" crc ok\r\n");
  }
  fseek(fp,0,SEEK_END);
  fwrite(&buf[size],1,4,fp);
  fclose(fp);
  free(buf);
  return;
}
