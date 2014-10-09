#include <stdio.h>
#include <stdlib.h>
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

int main( int argc, char *argv[] ) {
  // check size
  struct stat filestatus;
  stat( argv[1], &filestatus );
  int size = filestatus.st_size;
  data = (unsigned char*)calloc(size,1);

  // block chain tables
  int *bs = (int*)calloc(size/BSIZE,sizeof(int)); // sequence number
  char *ck = (char*)calloc(size/BSIZE,1); // verified checksum

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

  // 1st PASS
  // scan blocks
  int sectype, self;
  for( int i=0; i<size; i+=BSIZE ) {
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
        // store sequence number
        bs[i/BSIZE] = get4(i+8);
      }
    } else {
      if( i/BSIZE > 1 ) 
        ck[i/BSIZE] = 0;
    }
  }

  // 2nd PASS check files
  for (int j=0; j<nfile; ++j)
  {
    int i = fileheaders[j];

    int nblocks = get4(i+8);
    printf( "%d: '%s' (%d blocks, first=%d)\n", i/BSIZE, getString(i+BSIZE-79,30), nblocks, get4(i+16) );

    int broken = 0;
    for (int k=0; k<nblocks; ++k)
    {
      int dblock = get4(i+BSIZE-204-k*4);
      if (ck[dblock] == 0) broken++;
    }
    if (broken==0)
      printf("ALL OK!\n");
    else
      printf("contains %d broken blocks out of %d :-(\n", broken, nblocks);
  }

/*

  // write chains
  for( int i=0; i<size/BSIZE; ++i ) {
    if( bs[i] == 1 ) {
      int j = i;
      do {
        // write get4(j*BSIZE+12) bytes
        
        // next block
        j = get4(j*BSIZE+16);
      } while( j != 0 );
    }
  }
*/
}
