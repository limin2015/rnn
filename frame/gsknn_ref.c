/*
 * --------------------------------------------------------------------------
 * GSKNN (General Stride K-Nearest Neighbors)
 * --------------------------------------------------------------------------
 * Copyright (C) 2015, The University of Texas at Austin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * gsknn_ref.c
 *
 * Chenhan D. Yu - Department of Computer Science,
 *                 The University of Texas at Austin
 *
 *
 * Purpose:
 * implement reference gsknn using GEMM (optional) and max heap in C.
 *
 * Todo:
 *
 *
 * Modification:
 *
 * 
 * */

#include <omp.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

#include <gsknn.h>
#include <gsknn_ref.h>

#ifdef USE_BLAS
/* 
 * dgemm and sgemm prototypes
 *
 */ 
void dgemm_(char*, char*, int*, int*, int*, double*, double*, 
    int*, double*, int*, double*, double*, int*);
void sgemm_(char*, char*, int*, int*, int*, float*, float*, 
    int*, float*, int*, float*, float*, int*);
#endif


/*
 * This reference function will call BLAS.
 *
 */
void sgsknn_ref(
    int    m,
    int    n,
    int    k,
    int    r,
    float  *XA,
    float  *XA2,
    int    *alpha,
    float  *XB,
    float  *XB2,
    int    *beta,
    float  *D,
    int    *I
    )
{
  //printf( "sgsknn_ref(): Not implemented yet.\n" );
  int    i, j, p;
  double beg, time_collect, time_dgemm, time_square, time_heap;
  float  *As, *Bs, *Cs;
  float  fneg2 = -2.0, fzero = 0.0;

  // Sanity check for early return.
  if ( m == 0 || n == 0 || k == 0 || r == 0 ) return;

  // Allocate buffers.
  As = (float*)malloc( sizeof(float) * m * k );
  Bs = (float*)malloc( sizeof(float) * n * k );
  Cs = (float*)malloc( sizeof(float) * m * n );

  #include "sgsknn_ref_impl.h"

  // Pure C Max Heap implementation. 
  beg = omp_get_wtime();
  #pragma omp parallel for schedule( dynamic )
  for ( j = 0; j < n; j ++ ) {
    heapSelect_s( m, r, &Cs[ j * m ], alpha, &D[ j * r ], &I[ j * r ] );
  }
  time_heap = omp_get_wtime() - beg;

  //printf( "collect: %5.3lf, gemm: %5.3lf, square: %5.3lf, heap: %5.3lf sec\n", 
  //    time_collect, time_dgemm, time_square, time_heap );

  // Free  buffers
  free( As );
  free( Bs );
  free( Cs );
}


void dgsknn_ref(
    int    m,
    int    n,
    int    k,
    int    r,
    double *XA,
    double *XA2,
    int    *alpha,
    double *XB,
    double *XB2,
    int    *beta,
    double *D,
    int    *I
    )
{
  // Local variables.
  int    i, j, p;
  double beg, time_collect, time_dgemm, time_square, time_heap;
  double *As, *Bs, *Cs;
  double fneg2 = -2.0, fzero = 0.0;


  // Sanity check for early return.
  if ( m == 0 || n == 0 || k == 0 || r == 0 ) return;


  // Allocate buffers.
  As = (double*)malloc( sizeof(double) * m * k );
  Bs = (double*)malloc( sizeof(double) * n * k );
  Cs = (double*)malloc( sizeof(double) * m * n );

  #include "dgsknn_ref_impl.h"

  // Pure C Max Heap implementation. 
  beg = omp_get_wtime();
  #pragma omp parallel for schedule( dynamic )
  for ( j = 0; j < n; j ++ ) {
    heapSelect_d( m, r, &Cs[ j * m ], alpha, &D[ j * r ], &I[ j * r ] );
  }
  time_heap = omp_get_wtime() - beg;
  
  /*
  printf( "collect: %5.3lf, gemm: %5.3lf, square: %5.3lf, heap: %5.3lf sec\n", 
      time_collect, time_dgemm, time_square, time_heap );
  */

  // Free  buffers
  free( As );
  free( Bs );
  free( Cs );
}
