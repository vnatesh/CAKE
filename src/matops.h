#ifndef __MATOPS_H__
#define __MATOPS_H__

#include <testbench/nvhls_rand.h>
#include "arch.h"



// define types for tile matmul. same types as in Packet class
typedef NVINT32  AccumType;
typedef typename nvhls::nv_scvector<nvhls::nv_scvector <AccumType, tile_sz>, tile_sz> VectorType;

// module load ~/cs148/catapult

template<typename T> vector<vector<T>> GetMat(int rows, int cols) {

  vector<vector<T>> mat(rows, vector<T>(cols)); 

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      mat[i][j] = (nvhls::get_rand<8>()) % 4; // 8 bit numbers for weight and activation
    }
  }

  return mat;
}


template<typename T, typename U>
vector<vector<U>> MatMul(vector<vector<T>> mat_A, vector<vector<T>> mat_B) {

  int _N = (int) mat_A.size();
  int _M = (int) mat_A[0].size();
  int _P = (int) mat_B[0].size();
  assert(_M == (int) mat_B.size());
  vector<vector<U>> mat_C(_N, vector<U>(_P, 0)); 

  for (int i = 0; i < _N; i++) {
    for (int j = 0; j < _P; j++) {
      mat_C[i][j] = 0;
      for (int k = 0; k < _M; k++) {
        mat_C[i][j] += mat_A[i][k]*mat_B[k][j];
      }
    }
  }
  return mat_C;
}



template<typename T> void PrintMat(vector<vector<T>> mat) {
  int rows = (int) mat.size();
  int cols = (int) mat[0].size();
  for (int i = 0; i < rows; i++) {
    cout << "\t";
    for (int j = 0; j < cols; j++) {
      cout << mat[i][j] << "\t";
    }
    cout << "\n";
  }
  cout << "\n";
}



VectorType TileMul(VectorType mat_A, VectorType mat_B) {
  
  VectorType mat_C; 

  for (int i = 0; i < tile_sz; i++) {
    for (int j = 0; j < tile_sz; j++) {
      mat_C[i][j] = 0;
      for (int k = 0; k < tile_sz; k++) {
        mat_C[i][j] += mat_A[i][k]*mat_B[k][j];
      }
    }
  }
  return mat_C;
}



#endif
