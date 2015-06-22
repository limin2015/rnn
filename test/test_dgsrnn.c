#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <limits.h>
#include <rnn.h>


void bubble_sort(
    int    r,
    double *D,
    int    *I
    )
{
  int    i, j;

  for ( i = 0; i < r - 1; i ++ ) {
    for ( j = 0; j < r - 1 - i; j ++ ) {
       if ( D[ j ] > D[ j + 1 ] ) {
         double dtmp;
         double itmp;
         dtmp = D[ j ];
         D[ j ] = D[ j + 1 ];
         D[ j + 1 ] = dtmp;
         itmp = I[ j ];
         I[ j ] = I[ j + 1 ];
         I[ j + 1 ] = itmp;
       }
    }
  }
}

void compute_error(
    int    r,
    int    n,
    double *D,
    int    *I,
    double *D_gold,
    int    *I_gold
    )
{
  int    i, j;
  double *D1, *D2;
  int    *I1, *I2;

  D1 = (double*)malloc( sizeof(double) * r * n );
  D2 = (double*)malloc( sizeof(double) * r * n );
  I1 = (int*)malloc( sizeof(int) * r * n );
  I2 = (int*)malloc( sizeof(int) * r * n );


  // bubble sort
  for ( j = 0; j < n; j ++ ) {
    for ( i = 0; i < r; i ++ ) {
      D1[ j * r + i ] = D[ j * r + i ];
      I1[ j * r + i ] = I[ j * r + i ];
      D2[ j * r + i ] = D_gold[ j * r + i ];
      I2[ j * r + i ] = I_gold[ j * r + i ];
    }
    bubble_sort( r, &D1[ j * r ], &I1[ j * r ] );
    bubble_sort( r, &D2[ j * r ], &I2[ j * r ] );
  }

  for ( j = 0; j < n; j ++ ) {
    for ( i = 0; i < r; i ++ ) {
      if ( fabs( D1[ j * r + i ] - D2[ j * r + i ] ) > 0.0000000001 ) {
        printf( "D[ %d ][ %d ] != D_gold, %E, %E\n", i, j, D1[ j * r + i ], D2[ j * r + i ] );
        break;
      }
      //if ( I1[ j * r + i ] != I2[ j * r + i ] ) {
      //  printf( "I[ %d ][ %d ] != I_gold, %d, %d\n", i, j, I1[ j * r + i ], I2[ j * r + i ] );
      //  printf( "D[ %d ][ %d ] != D_gold, %E, %E\n", i, j, D1[ j * r + i ], D2[ j * r + i ] );
      //  break;
      //}
    }
  }


  free( D1 );
  free( D2 );
  free( I1 );
  free( I2 );
}



void test_dgsrnn(
    int m,
    int n,
    int k,
    int r
    ) 
{
  int    i, j, p, nx, iter, n_iter;
  int    *amap, *bmap, *I, *I_mkl;
  double *XA, *XB, *XA2, *XB2, *D, *D_mkl;
  double tmp, error, flops;
  double ref_beg, ref_time, dgsrnn_beg, dgsrnn_time;

  heap_t *heap = rnn_heapCreate( m, r, 1.79E+308 );

  nx = 4096 * 30;
  n_iter = 4;


  amap  = (int*)malloc( sizeof(int) * m );
  bmap  = (int*)malloc( sizeof(int) * n );
  I     = (int*)malloc( sizeof(int) * r * n );
  I_mkl = (int*)malloc( sizeof(int) * r * n );
  XA    = (double*)malloc( sizeof(double) * k * nx );
  XA2   = (double*)malloc( sizeof(double) * nx ); 
  D     = (double*)malloc( sizeof(double) * r * n );
  D_mkl = (double*)malloc( sizeof(double) * r * n );


  for ( i = 0; i < m; i ++ ) {
    amap[ i ] = 2 * i;
  }

  for ( j = 0; j < n; j ++ ) {
    bmap[ j ] = 2 * j + 1;
  }


  // random[ 0, 0.1 ]
  for ( i = 0; i < nx; i ++ ) {
    for ( p = 0; p < k; p ++ ) {
      XA[ i * k + p ] = (double)( rand() % 100 ) / 1000.0;	
    }
  }

  // Compute XA2
  for ( i = 0; i < nx; i ++ ) {
    tmp = 0.0;
    for ( p = 0; p < k; p ++ ) {
      tmp += XA[ i * k + p ] * XA[ i * k + p ];
    }
    XA2[ i ] = tmp;
  }


  // Use the same coordinate table
  XB  = XA;
  XB2 = XA2;


  // Initialize D to the maximum double and I to -1.
  for ( j = 0; j < n; j ++ ) {
    for ( i = 0; i < r; i ++ ) {
      D[ j * r + i ] = 1.79E+308;
      D_mkl[ j * r + i ] = 1.79E+308;
      I[ j * r + i ] = -1;
      I_mkl[ j * r + i ] = -1;
    }
  }


  // Call my implementation
  {
    dgsrnn_var3(
        m,
        n,
        k,
        r,
        XA,
        XA2,
        amap,
        XB,
        XB2,
        bmap,
        heap->D,
        //D,
        heap->I
        //I
        );
  }

  dgsrnn_beg = omp_get_wtime();
  for ( iter = 1; iter < n_iter; iter ++ ) {
    dgsrnn_var3(
        m,
        n,
        k,
        r,
        XA  + iter * m * k,
        XA2 + iter * m,
        amap,
        XB,
        XB2,
        bmap,
        heap->D,
        //D,
        heap->I
        //I
        );
  } 
  dgsrnn_time = omp_get_wtime() - dgsrnn_beg;


  {
    dgsrnn_ref(
        m,
        n,
        k,
        r,
        XA,
        XA2,
        amap,
        XB,
        XB2,
        bmap,
        D_mkl,
        I_mkl
        );
  }

  ref_beg = omp_get_wtime();
  // Call the reference function 
  for ( iter = 1; iter < n_iter; iter ++ ) {
    dgsrnn_ref(
        m,
        n,
        k,
        r,
        XA  + iter * m * k,
        XA2 + iter * m,
        amap,
        XB,
        XB2,
        bmap,
        D_mkl,
        I_mkl
        );
  }
  ref_time = omp_get_wtime() - ref_beg;



  //for ( j = 0; j < n; j ++ ) {
  //  for ( i = 0; i < r; i ++ ) {
  //    D[ j * r + i ] = heap->D[ j * heap->ldk + i + 3 ];
  //    I[ j * r + i ] = heap->I[ j * heap->ldk + i + 3 ];
  //  }
  //}


  //// Compute error
  //compute_error(
  //    r,
  //    n,
  //    D,
  //    I,
  //    D_mkl,
  //    I_mkl
  //    );



  ref_time    /= ( n_iter - 1 );
  dgsrnn_time /= ( n_iter - 1 );
  flops = ( m * n / ( 1024.0 * 1024.0 * 1024.0 ) )* ( 2 * k + 3 );


  printf( "%d, %d, %d, %d, %5.2lf, %5.2lf;\n", 
      m, n, k, r, flops / dgsrnn_time, flops / ref_time );
  //printf( "%d, %d, %d, %d, %5.2lf, %5.2lf;\n", 
  //    m, n, k, r, dgsrnn_time, ref_time );


  free( XA );
  free( XA2 );
  free( D );
  free( I );
  free( D_mkl );
  free( I_mkl );
}




// This this test case, D and I are m x r.
void test_dgsrnn_var2(
    int m,
    int n,
    int k,
    int r
    ) 
{
  int    i, j, p, nx, iter, n_iter;
  int    *amap, *bmap, *I, *I_mkl;
  double *XA, *XB, *XA2, *XB2, *D, *D_mkl;
  double tmp, error, flops;
  double ref_beg, ref_time, dgsrnn_beg, dgsrnn_time;

  nx     = 4096 * 30;
  n_iter = 4;


  amap  = (int*)malloc( sizeof(int) * m );
  bmap  = (int*)malloc( sizeof(int) * n );
  I     = (int*)malloc( sizeof(int) * r * m );
  I_mkl = (int*)malloc( sizeof(int) * r * m );
  XA    = (double*)malloc( sizeof(double) * k * nx );
  XA2   = (double*)malloc( sizeof(double) * nx ); 
  D     = (double*)malloc( sizeof(double) * r * m );
  D_mkl = (double*)malloc( sizeof(double) * r * m );


  heap_t *heap = rnn_heapCreate( m, r, 1.79E+308 );
  //printf( "rnn_heapCreate(): %d, %d, %d\n", 
  //    heap->I[ 0 ], heap->I[ 1 ], heap->I[ 2 ] );


  for ( i = 0; i < m; i ++ ) {
    amap[ i ] = i;
  }

  for ( j = 0; j < n; j ++ ) {
    bmap[ j ] = j;
  }


  // random[ 0, 0.1 ]
  for ( i = 0; i < nx; i ++ ) {
    for ( p = 0; p < k; p ++ ) {
      XA[ i * k + p ] = (double)( rand() % 1000 ) / 1000.0;	
    }
  }

  // Compute XA2
  for ( i = 0; i < nx; i ++ ) {
    tmp = 0.0;
    for ( p = 0; p < k; p ++ ) {
      tmp += XA[ i * k + p ] * XA[ i * k + p ];
    }
    XA2[ i ] = tmp;
  }

  // Use the same coordinate table
  XB  = XA;
  XB2 = XA2;


  // Initialize D to the maximum double and I to -1.
  for ( j = 0; j < m; j ++ ) {
    for ( i = 0; i < r; i ++ ) {
      D[ j * r + i ]     = 1.79E+308;
      D_mkl[ j * r + i ] = 1.79E+308;
      I[ j * r + i ]     = -1;
      I_mkl[ j * r + i ] = -1;
    }
  }

  // Other Initialization
  //for ( j = 0; j < m; j ++ ) {
  //  for ( i = 0; i < r; i ++ ) {
  //    D[ j * r + i ]     =  sqrt( k ) / ( i + 1 );
  //    D_mkl[ j * r + i ] =  sqrt( k ) / ( i + 1 );
  //    I[ j * r + i ]     = n + i;
  //    I_mkl[ j * r + i ] = n + i; 
  //  }
  //}


  {
    dgsrnn_var2(
        m,
        n,
        k,
        r,
        XA,
        XA2,
        amap,
        XB,
        XB2,
        bmap,
        D,
        //heap->D,
        I
        //heap->I
        );
  }


  dgsrnn_beg = omp_get_wtime();
  // Call my implementation 
  for ( iter = 1; iter < n_iter; iter ++ ) {
    dgsrnn_var2(
        m,
        n,
        k,
        r,
        XA,
        XA2,
        amap,
        XB  + iter * k * n,
        XB2 + iter * n,
        bmap,
        D,
        //heap->D,
        I
        //heap->I
        );
  }
  dgsrnn_time = omp_get_wtime() - dgsrnn_beg;



  {
    dgsrnn_ref(
        n,
        m,
        k,
        r,
        XB,
        XB2,
        bmap,
        XA,
        XA2,
        amap,
        D_mkl,
        I_mkl
        );
  }


  ref_beg = omp_get_wtime();
  // Call the reference function ( we use the transpose to solve the problem. )
  for ( iter = 1; iter < n_iter; iter ++ ) {
    dgsrnn_ref(
        n,
        m,
        k,
        r,
        XB  + iter * k * n,
        XB2 + iter * n,
        bmap,
        XA,
        XA2,
        amap,
        D_mkl,
        I_mkl
        );
  }
  ref_time = omp_get_wtime() - ref_beg;


  //for ( j = 0; j < m; j ++ ) {
  //  for ( i = 0; i < r; i ++ ) {
  //    D[ j * r + i ] = heap->D[ j * heap->ldk + i + 3 ];
  //    I[ j * r + i ] = heap->I[ j * heap->ldk + i + 3 ];
  //  }
  //}



  // Compute error
  compute_error(
      r,
      m,
      D,
      I,
      D_mkl,
      I_mkl
      );



  ref_time    /= ( n_iter - 1 );
  dgsrnn_time /= ( n_iter - 1 );
  flops = ( m * n / ( 1024.0 * 1024.0 * 1024.0 ) )* ( 2 * k + 3 );


  printf( "%d, %d, %d, %d, %5.2lf, %5.2lf;\n", 
      m, n, k, r, flops / dgsrnn_time, flops / ref_time );
  //printf( "%d, %d, %d, %d, %5.2lf, %5.2lf;\n", 
  //    m, n, k, r, dgsrnn_time, ref_time );


  free( XA );
  free( XA2 );
  free( D );
  free( I );
  free( D_mkl );
  free( I_mkl );
}







int main( int argc, char *argv[] )
{
  int    m, n, k, r; 
  //printf("start the test dgsknn!\n");
  fflush( stdout );



  if ( argc != 5 ) {
    printf("argc: %d\n", argc);
    printf("we need 4 arguments!\n");
    exit(0);
  }

  sscanf( argv[ 1 ], "%d", &m );
  sscanf( argv[ 2 ], "%d", &n );
  sscanf( argv[ 3 ], "%d", &k );
  sscanf( argv[ 4 ], "%d", &r );


  //printf("before call the function!\n");
  test_dgsrnn( m, n, k, r );
  //test_dgsrnn_var2( m, n, k, r );


  return 0;
}