// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.googlecode.com.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#ifndef MFEM_SPARSEMAT
#define MFEM_SPARSEMAT

// Data types for sparse matrix

#include "../general/mem_alloc.hpp"
#include "../general/table.hpp"
#include "densemat.hpp"

#include "sight_ns.h"


class RowNode
{
public:
   RowNode *Prev;
   int Column;
   double Value;
};

/// Data type sparse matrix
class SparseMatrix : public Matrix
{
private:

   /** Arrays for the connectivity information in the CSR storage.
       I is of size "size+1", J is of size the number of nonzero entries
       in the Sparse matrix (actually stored I[size]) */
   int *I, *J, width;

   /// The nonzero entries in the Sparse matrix with size I[size].
   double *A;

   RowNode **Rows;

   int current_row;
   union { int *J; RowNode **Node; } ColPtr;

#ifdef MFEM_USE_MEMALLOC
   MemAlloc <RowNode, 1024> NodesMem;
#endif

   inline void SetColPtr(const int row);
   inline void ClearColPtr();
   inline double &SearchRow(const int col);
   inline void _Add_(const int col, const double a)
   { SearchRow(col) += a; }
   inline void _Set_(const int col, const double a)
   { SearchRow(col) = a; }
   inline double _Get_(const int col);

   inline double &SearchRow(const int row, const int col);
   inline void _Add_(const int row, const int col, const double a)
   { SearchRow(row, col) += a; }
   inline void _Set_(const int row, const int col, const double a)
   { SearchRow(row, col) = a; }

public:
   /// Creates sparse matrix.
   explicit SparseMatrix(int nrows, int ncols = 0);

   SparseMatrix(int *i, int *j, double *data, int m, int n)
      : Matrix (m), I(i), J(j), width(n), A(data)
   { Rows = NULL; ColPtr.J = NULL; }

   /// Return the array I
   inline int *GetI() const { return I; }
   /// Return the array J
   inline int *GetJ() const { return J; }
   /// Return element data
   inline double *GetData() const { return A; }
   /// Return the number of columns
   inline int Width() const { return width; }
   /// Returns the number of elements in row i
   int RowSize(int i);

   /// Returns reference to a_{ij}.  Index i, j = 0 .. size-1
   virtual double &Elem(int i, int j);

   /// Returns constant reference to a_{ij}.  Index i, j = 0 .. size-1
   virtual const double &Elem(int i, int j) const;

   /// Returns reference to A[i][j].  Index i, j = 0 .. size-1
   double &operator()(int i, int j);

   /// Returns reference to A[i][j].  Index i, j = 0 .. size-1
   const double &operator()(int i, int j) const;

   /// Matrix vector multiplication.
   virtual void Mult(const Vector &x, Vector &y) const;

   /// y += A * x (default)  or  y += a * A * x
   void AddMult(const Vector &x, Vector &y, const double a = 1.0) const;

   /// Multiply a vector with the transposed matrix. y = At * x
   void MultTranspose(const Vector &x, Vector &y) const;

   /// y += At * x (default)  or  y += a * At * x
   void AddMultTranspose(const Vector &x, Vector &y,
                         const double a = 1.0) const;

   void PartMult(const Array<int> &rows, const Vector &x, Vector &y);

   /// Compute y^t A x
   double InnerProduct(const Vector &x, const Vector &y) const;

   void GetRowSums(Vector &x) const;

   /// Returns a pointer to approximation of the matrix inverse.
   virtual MatrixInverse *Inverse() const;

   /// Eliminates a column from the transpose matrix.
   void EliminateRow(int row, const double sol, Vector &rhs);
   void EliminateRow(int row);
   void EliminateCol(int col);
   /// Eliminate all columns 'i' for which cols[i] != 0
   void EliminateCols(Array<int> &cols, Vector *x = NULL, Vector *b = NULL);

   /** Eliminates the column 'rc' to the 'rhs', deletes the row 'rc' and
       replaces the element (rc,rc) with 1.0; assumes that element (i,rc)
       is assembled if and only if the element (rc,i) is assembled.
       If d != 0 then the element (rc,rc) remains the same. */
   void EliminateRowCol(int rc, const double sol, Vector &rhs, int d = 0);

   /** Like previous one, but multiple values for eliminated dofs are accepted,
       and accordingly multiple right-hand-sides are used. */
   void EliminateRowColMultipleRHS(int rc, const Vector &sol,
                                   DenseMatrix &rhs, int d = 0);

   void EliminateRowCol(int rc, int d = 0);
   // Same as above + save the eliminated entries in Ae so that
   // (*this) + Ae is the original matrix
   void EliminateRowCol(int rc, SparseMatrix &Ae, int d = 0);

   /// If a row contains only one diag entry of zero, set it to 1.
   void SetDiagIdentity();
   /// If a row contains only zeros, set its diagonal to 1.
   void EliminateZeroRows();

   /// Gauss-Seidel forward and backward iterations over a vector x.
   void Gauss_Seidel_forw(const Vector &x, Vector &y) const;
   void Gauss_Seidel_back(const Vector &x, Vector &y) const;

   /// Determine appropriate scaling for Jacobi iteration
   double GetJacobiScaling() const;
   /** One scaled Jacobi iteration for the system A x = b.
       x1 = x0 + sc D^{-1} (b - A x0)  where D is the diag of A. */
   void Jacobi(const Vector &b, const Vector &x0, Vector &x1, double sc) const;

   /** x1 = x0 + sc D^{-1} (b - A x0) where \f$ D_{ii} = \sum_j |A_{ij}| \f$. */
   void Jacobi2(const Vector &b, const Vector &x0, Vector &x1,
                double sc = 1.0) const;

   /** Finalize the matrix initialization. The function should be called
       only once, after the matrix has been initialized. Internally, this
       method converts the matrix from row-wise linked list format into
       CSR (compressed sparse row) format. */
   virtual void Finalize(int skip_zeros = 1);

   int Finalized() { return (A != NULL); }

   /** Split the matrix into M x N blocks of sparse matrices in CSR format.
       The 'blocks' array is M x N (i.e. M and N are determined by its
       dimensions) and its entries are overwritten by the new blocks. */
   void GetBlocks(Array2D<SparseMatrix *> &blocks) const;

   void GetSubMatrix(const Array<int> &rows, const Array<int> &cols,
                     DenseMatrix &subm);

   void Set(const int i, const int j, const double a);
   void Add(const int i, const int j, const double a);

   void SetSubMatrix(const Array<int> &rows, const Array<int> &cols,
                     const DenseMatrix &subm, int skip_zeros = 1);

   void SetSubMatrixTranspose(const Array<int> &rows, const Array<int> &cols,
                              const DenseMatrix &subm, int skip_zeros = 1);

   void AddSubMatrix(const Array<int> &rows, const Array<int> &cols,
                     const DenseMatrix &subm, int skip_zeros = 1);

   void AddRow(const int row, const Array<int> &cols, const Vector &srow);

   void ScaleRow(const int row, const double scale);

   /** Add a sparse matrix to "*this" sparse marix
       Both marices should not be finilized */
   SparseMatrix &operator+=(SparseMatrix &B);

   SparseMatrix &operator=(double a);

   /// Prints matrix to stream out.
   void Print(ostream &out = sightN::dbg, int width = 4) const;

   /// Prints matrix in matlab format.
   void PrintMatlab(ostream &out = sightN::dbg) const;

   /// Prints matrix in Matrix Market sparse format.
   void PrintMM(ostream &out = sightN::dbg) const;

   /// Prints matrix to stream out in hypre_CSRMatrix format.
   void PrintCSR(ostream &out) const;

   /// Prints a sparse matrix to stream out in CSR format.
   void PrintCSR2(ostream &out) const;

   /// Walks the sparse matrix
   int Walk(int &i, int &j, double &a);

   /// Returns max_{i,j} |(i,j)-(j,i)| for a finalized matrix
   double IsSymmetric() const;

   /// (*this) = 1/2 ((*this) + (*this)^t)
   void Symmetrize();

   /// Returns the number of the nonzero elements in the matrix
   int NumNonZeroElems() const;

   double MaxNorm() const;

   /// Count the number of entries with |a_ij| < tol
   int CountSmallElems(double tol);

   /// Call this if data has been stolen.
   void LoseData() { I=0; J=0; A=0; }

   /// Destroys sparse matrix.
   virtual ~SparseMatrix();
};

/// Applies f() to each element of the matrix (after it is finalized).
void SparseMatrixFunction(SparseMatrix &S, double (*f)(double));


/// Transpose of a sparse matrix. A must be finalized.
SparseMatrix *Transpose(SparseMatrix &A);

/** Matrix product A.B.
    If OAB is not NULL, we assume it has the structure
    of A.B and store the result in OAB.
    If OAB is NULL, we create a new SparseMatrix to store
    the result and return a pointer to it.
    All matrices must be finalized.  */
SparseMatrix *Mult(SparseMatrix &A, SparseMatrix &B,
                   SparseMatrix *OAB = NULL);


/** RAP matrix product. ORAP is like OAB above.
    All matrices must be finalized.  */
SparseMatrix *RAP(SparseMatrix &A, SparseMatrix &R,
                  SparseMatrix *ORAP = NULL);

/** Matrix multiplication A^t D A.
    All matrices must be finalized.  */
SparseMatrix *Mult_AtDA(SparseMatrix &A, Vector &D,
                        SparseMatrix *OAtDA = NULL);


// Inline methods

inline void SparseMatrix::SetColPtr(const int row)
{
   if (Rows)
   {
      if (ColPtr.Node == NULL)
      {
         ColPtr.Node = new RowNode *[width];
         for (int i = 0; i < width; i++)
            ColPtr.Node[i] = NULL;
      }
      for (RowNode *node_p = Rows[row]; node_p != NULL; node_p = node_p->Prev)
      {
         ColPtr.Node[node_p->Column] = node_p;
      }
   }
   else
   {
      if (ColPtr.J == NULL)
      {
         ColPtr.J = new int[width];
         for (int i = 0; i < width; i++)
            ColPtr.J[i] = -1;
      }
      for (int j = I[row], end = I[row+1]; j < end; j++)
      {
         ColPtr.J[J[j]] = j;
      }
   }
   current_row = row;
}

inline void SparseMatrix::ClearColPtr()
{
   if (Rows)
      for (RowNode *node_p = Rows[current_row]; node_p != NULL;
           node_p = node_p->Prev)
      {
         ColPtr.Node[node_p->Column] = NULL;
      }
   else
      for (int j = I[current_row], end = I[current_row+1]; j < end; j++)
      {
         ColPtr.J[J[j]] = -1;
      }
}

inline double &SparseMatrix::SearchRow(const int col)
{
   if (Rows)
   {
      RowNode *node_p = ColPtr.Node[col];
      if (node_p == NULL)
      {
#ifdef MFEM_USE_MEMALLOC
         node_p = NodesMem.Alloc();
#else
         node_p = new RowNode;
#endif
         node_p->Prev = Rows[current_row];
         node_p->Column = col;
         node_p->Value = 0.0;
         Rows[current_row] = ColPtr.Node[col] = node_p;
      }
      return node_p->Value;
   }
   else
   {
      const int j = ColPtr.J[col];
      if (j == -1)
         mfem_error("SparseMatrix::SearchRow : entry is not allocated!");
      return A[j];
   }
}

inline double SparseMatrix::_Get_(const int col)
{
   if (Rows)
   {
      RowNode *node_p = ColPtr.Node[col];
      return (node_p == NULL) ? 0.0 : node_p->Value;
   }
   else
   {
      const int j = ColPtr.J[col];
      return (j == -1) ? 0.0 : A[j];
   }
}

inline double &SparseMatrix::SearchRow(const int row, const int col)
{
   if (Rows)
   {
      RowNode *node_p;

      for (node_p = Rows[row]; 1; node_p = node_p->Prev)
         if (node_p == NULL)
         {
#ifdef MFEM_USE_MEMALLOC
            node_p = NodesMem.Alloc();
#else
            node_p = new RowNode;
#endif
            node_p->Prev = Rows[row];
            node_p->Column = col;
            node_p->Value = 0.0;
            Rows[row] = node_p;
            break;
         }
         else if (node_p->Column == col)
         {
            break;
         }
      return node_p->Value;
   }
   else
   {
      int *Ip = I+row, *Jp = J;
      for (int k = Ip[0], end = Ip[1]; k < end; k++)
         if (Jp[k] == col)
            return A[k];
      mfem_error("SparseMatrix::SearchRow(...)");
   }
   return A[0];
}

#endif
