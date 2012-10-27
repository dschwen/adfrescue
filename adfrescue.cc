#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

unsigned char *data;
const int BSIZE = 512;

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
      if( get4(i) == 2 && sectype == -3 && i==self ) {
        printf( "'%s'\n", getString(i+BSIZE-79,30) );
      }

      // data block
      if( get4(i) == 8 ) {
        // store sequence number
        bs[i/BSIZE] = get4(i+8);
      }
    } else {
      if( i/BSIZE > 1 ) 
        printf( "checksum fail at block %d\n", i/BSIZE );
    }
  }

  // 2nd PASS
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

}
