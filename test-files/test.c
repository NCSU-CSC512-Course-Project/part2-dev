#include <stdio.h>
#include <stdbool.h>

int main(){
   int id;
   int n;
   int k = 0;
   bool a = false;
   bool b = true;
   scanf("%d, %d", &id, &n);
   int s = 0;
   for ( int i = 0; i < n; i++ ) {
      s += rand();
   }

   if ( a ) {
      printf( "a\n" ); 
   } else if ( b ) {
      printf( "b\n" );
   } else {
      printf( "c\n" );
   }

   while ( k < 2 ) {
      k++;
   }
   printf( "id=%d; sum=%d\n", id, n );
}
