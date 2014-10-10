#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

unsigned char *data;
const int BSIZE = 512;

const int maxfile = 1000;
int nfile = 0;
int fileheaders[maxfile];

// function for fetching BigEndian data at a given offset
char* getString( int pos, int len ) {
  char *ret = (char*)calloc(len+1,1);
  for( int i=0; i<len; ++i ) ret[i]=(char)data[i+pos];
  ret[len]=0;
  return ret;
}
int get2( int pos ) {
  int d = int(data[pos])*256 + int(data[pos+1]);
  return d; // negative numbers?!
}
int get4( int pos ) {
  int d = ( ( int(data[pos])*256 + int(data[pos+1]) )*256 + int(data[pos+2]) ) * 256 + int(data[pos+3]);
  return d; // negative numbers?!
}

int checksum( int block ) {
  int o = block*BSIZE;
  u_int32_t ns = 0;
  for( int i=0; i<BSIZE; i+=4 )
    ns += i==20?0:get4(o+i);
  ns = -ns;
  return ns;
}

int *bs, *next;
char *ck, *used;

void followBlockChain(int block, FILE * out)
{
  // write this block
  int localsize = get4(block*BSIZE+12);
  fwrite(&data[block*BSIZE+24], sizeof(char), localsize, out);
  used[block] = 1;

  // jump to next block
  if(bs[next[block]] == bs[block]+1 && used[next[block]] == 0 && ck[next[block]] != 0)
    followBlockChain(next[block], out);
}

int main( int argc, char *argv[] ) {
  // check size
  struct stat filestatus;
  stat( argv[1], &filestatus );
  int size = filestatus.st_size;
  data = (unsigned char*)calloc(size,1);

  // block chain tables
  bs = (int*)calloc(size/BSIZE,sizeof(int)); // sequence number
  next = (int*)calloc(size/BSIZE,sizeof(int)); // next datablock
  ck = (char*)calloc(size/BSIZE,1); // verified checksum
  used = (char*)calloc(size/BSIZE,1); // is this block used by any file?

  // read entire file
  FILE *in = fopen(argv[1],"rb");
  if(!in) {
    printf("could open file!\n");
    return 1;
  }
  if( fread(data,1,size,in) != size ) {
    printf("could not read entire file!\n");
    return 1;
  }
  fclose(in);

  // check filesystem type
  int fstype = get4(0);
  if( fstype != 0x444f5300 ) {
    switch(fstype) {
      case 0x444f5301 : printf("Fast file system"); break;
      case 0x444f5302 : printf("Inter DOS"); break;
      case 0x444f5303 : printf("Inter FFS"); break;
      case 0x444f5304 : printf("Fastdir OFS"); break;
      case 0x444f5305 : printf("Fastdir FFS"); break;
      case 0x4b49434b : printf("Kickstart disk"); break;
    }
    printf( " not supported yet. Exiting.\n" );
    return 1;
  }
  printf( "OFS disk filesystem detected. Continuing.\n");

  // 1st PASS scan all blocks
  printf("\nPASS1: Scan all blocks on disk.\n");
  int sectype, self;
  for( int i=0; i<size; i+=BSIZE ) {
    used[i/BSIZE] = 0;
    next[i/BSIZE] = -1;
    bs[i/BSIZE] = 0;

    sectype = get4(i+BSIZE-4);
    self = get4(i+4)*BSIZE;

    // verify checksum
    if( get4(i+20) == checksum(i/BSIZE) ) {
      ck[i/BSIZE] = 1;

      // file header block
      if( get4(i) == 2 && sectype == -3 && i==self )
        fileheaders[nfile++] = i;

      // data block
      if( get4(i) == 8 ) {
        // store sequence number (1...nblocks)
        bs[i/BSIZE] = get4(i+8);
        // next data block in chain
        next[i/BSIZE] = get4(i+16);
      }
    } else {
      if( i/BSIZE > 1 )
        ck[i/BSIZE] = 0;
    }
  }

  // 2nd PASS check and dump files for which we found header blocks
  printf("\nPASS2: Dump files using the header blocks.\n");
  for (int j=0; j<nfile; ++j)
  {
    int i = fileheaders[j];
    int broken = 0;
    int filesize = get4(i+BSIZE-188);
    char fname[1000];
    snprintf(fname, 1000, "%d_%s", j, getString(i+BSIZE-79,30) );
    printf( "%d: '%s' (%d bytes)\n", i/BSIZE, fname, filesize);

    // reserve space to collect file data
    unsigned char *filedata = (unsigned char*)calloc(filesize, 1);
    int fileptr = 0;

    do
    {
      int nblocks = get4(i+8);
      int nextext = get4(i+BSIZE-8);
      //printf("  %d %d\n", nblocks, nextext);

      // loop over data block table
      for (int k=0; k<nblocks; ++k)
      {
        int dblock = get4(i+BSIZE-204-k*4);
        if (ck[dblock] == 0)
        {
          // datablock is broken, assume data is just zeroes
          broken++;
          fileptr += BSIZE - 24;
          if (fileptr > filesize) fileptr = filesize;
        }
        else
        {
          // copy localsize bytes
          int localsize = get4(dblock*BSIZE+12);
          if (fileptr+localsize > filesize)
          {
            printf("More data than expected (%d/%d)!\n", fileptr+localsize, filesize);
            break;
          }
          memcpy(&filedata[fileptr], &data[dblock*BSIZE+24], localsize);
          fileptr += localsize;
        }

        // flag this block as used
        used[dblock] = 1;
      }

      // is this linking to a file extension block with another data block list?
      if (nextext != 0)
      {
        if (ck[nextext] == 0)
        {
          printf("Broken extension block (need to implement switch to block chain following)\n");
          break;
        }
        i = nextext * BSIZE;
      }
      else
        break;
    } while(true);

    if (broken==0)
      printf("ALL OK!\n");
    else
      printf("contains %d broken blocks :-(\n", broken);

    if (fileptr < filesize)
      printf("Not enough data found (%d/%d)!\n", fileptr, filesize);

    FILE *out = fopen(fname, "wb");
    if (!out)
    {
      printf("Could not open '%s' for writing.\n", fname);
      return 1;
    }
    fwrite (filedata, sizeof(char), filesize, out);
    fclose(out);

    free(filedata);
  }

  // 3rd PASS check and dump files for which we found NO header blocks
  printf("\nPASS3: Collect unclaimed block chains.\n");
  for( int i=0; i<size/BSIZE; ++i )
  {
    if(bs[i] == 1 && used[i] == 0 && ck[i] != 0)
    {
      char fname[1000];
      snprintf(fname, 1000, "chain.%d", i);
      FILE *out = fopen(fname, "wb");
      followBlockChain(i, out);
      fclose(out);
    }
  }

  printf("\ndone.\n");
}
