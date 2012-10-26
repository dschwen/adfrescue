#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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


int main( int argc, char *argv[] ) {
  // check size
  struct stat filestatus;
  stat( argv[1], &filestatus );
  int size = filestatus.st_size;
  data = (unsigned char*)calloc(size,1);

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

  // scan blocks
  int sectype, self;
  for( int i=0; i<size; i+=BSIZE ) {
    sectype = get4(i+BSIZE-4);
    self = get4(i+4)*BSIZE;
    if( get4(i) == 2 && sectype == -3 && i==self ) {
      printf( "'%s'\n", getString(i+BSIZE-79,30) );
    }
  }

  fclose(in);


}
