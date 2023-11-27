#include <stdio.h>

int main(){
   int id;
#include <stdbool.h>
   int n;
   int a = 0;
   int b = 1;
   int k = 0;
   scanf("%d, %d", &id, &n);
   int s = 0;
   for ( int i = 0; i < n; i++ ) {
      s += rand();
      if ( s > 10 ) {
         break;
      }
   }

   if ( a ) {
      printf( "a\n" ); 
   } else if ( b ) {
      printf( "b\n" );
   } else {
      printf( "c\n" );
   }

   while ( k <= 4 && b ) {
      k++;
   }
   printf( "id=%d; sum=%d\n", id, n );
}
