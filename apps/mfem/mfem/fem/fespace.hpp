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

#ifndef MFEM_FESPACE
#define MFEM_FESPACE

/* Class FiniteElementSpace - responsible for providing FEM view of the mesh
   (mainly managing the set of degrees of freedom). */

/** The ordering method used when the number of unknowns per mesh
    node (vector dimension) is bigger than 1. */
class Ordering
{
public:
   /** Ordering methods:
       byNODES - loop first over the nodes then over the vector dimension,
       byVDIM  - loop first over the vector dimension then over the nodes  */
   enum Type { byNODES, byVDIM };
};

/// Type of refinement (int, a tree, etc.)
typedef int RefinementType;

/// Data kept for every type of refinement
class RefinementData {
public:
   /// Refinement type
   RefinementType type;
   /// Number of the fine elements
   int num_fine_elems;
   /// Number of the fine dofs on the coarse element (fc)
   int num_fine_dofs;
   /// (local dofs of) fine element <-> fine dofs on the coarse element
   Table * fl_to_fc;
   /// Local interpolation matrix
   DenseMatrix * I;
   /// Releases the allocated memory
   ~RefinementData() { delete fl_to_fc; delete I;}
};

class NURBSExtension;

/// Abstract finite element space.
class FiniteElementSpace
{
protected:
   /// The mesh that FE space lives on.
   Mesh *mesh;

   /// Vector dimension (number of unknowns per degree of freedom).
   int vdim;

   /// Number of degrees of freedom. Number of unknowns are ndofs*vdim.
   int ndofs;

   /** Type of ordering of dofs.
       Ordering::byNODES - first nodes, then vector dimension,
       Ordering::byVDIM  - first vector dimension, then nodes  */
   int ordering;

   const FiniteElementCollection *fec;
   int nvdofs, nedofs, nfdofs, nbdofs;
   int *fdofs, *bdofs;

   /// Collection of currently known refinement data
   Array<RefinementData *> RefData;

   Table *elem_dof;
   Table *bdrElem_dof;
   Array<int> dof_elem_array, dof_ldof_array;

   NURBSExtension *NURBSext;
   int own_ext;

   void UpdateNURBS();

   void Constructor();
   void Destructor();   // does not destroy 'RefData'

   /* Create a FE space stealing all data (except RefData) from the
      given FE space. This is used in SaveUpdate() */
   FiniteElementSpace(FiniteElementSpace &);

   /// Constructs new refinement data using coarse element k as a template
   void ConstructRefinementData (int k, int cdofs, RefinementType type);

   /// Generates the local interpolation matrix for coarse element k
   DenseMatrix * LocalInterpolation (int k, int cdofs,
                                     RefinementType type,
                                     Array<int> &rows);

public:
   FiniteElementSpace(Mesh *m, const FiniteElementCollection *f,
                      int dim = 1, int order = Ordering::byNODES);

   /// Returns the mesh
   inline Mesh * GetMesh() const { return mesh; };

   NURBSExtension *GetNURBSext() { return NURBSext; }

   /// Returns vector dimension.
   inline int GetVDim() const { return vdim; };

   /// Returns the order of the i'th finite element
   int GetOrder(int i) const;

   /// Returns number of degrees of freedom.
   inline int GetNDofs() const { return ndofs; };

   inline int GetVSize() const { return vdim * ndofs; };

   /// Return the ordering method.
   inline int GetOrdering() const { return ordering; };

   const FiniteElementCollection *FEColl() const { return fec; };

   int GetNVDofs() const { return nvdofs; }
   int GetNEDofs() const { return nedofs; }
   int GetNFDofs() const { return nfdofs; }

   /// Returns number of elements in the mesh.
   inline int GetNE() const { return mesh -> GetNE(); };

   /// Returns number of nodes in the mesh.
   inline int GetNV() const { return mesh -> GetNV(); };

   /// Returns number of boundary elements in the mesh.
   inline int GetNBE() const { return mesh -> GetNBE(); };

   /// Returns the type of element i.
   inline int GetElementType (int i) const
   { return mesh -> GetElementType(i); }

   /// Returns the vertices of element i.
   inline void GetElementVertices (int i, Array<int> &vertices) const
   { mesh -> GetElementVertices(i, vertices); }

   /// Returns the type of boundary element i.
   inline int GetBdrElementType (int i) const
   { return mesh -> GetBdrElementType(i); }

   /// Returns ElementTransformation for the i'th element.
   ElementTransformation * GetElementTransformation(int i) const
   { return mesh -> GetElementTransformation(i); };

   /** Returns the transformation defining the i-th element in the user-defined
       variable. */
   void GetElementTransformation(int i, IsoparametricTransformation *ElTr)
   { mesh->GetElementTransformation(i, ElTr); }

   /// Returns ElementTransformation for the i'th boundary element.
   ElementTransformation * GetBdrElementTransformation(int i) const
   { return mesh -> GetBdrElementTransformation(i); };

   int GetAttribute(int i) { return mesh -> GetAttribute(i); };

   int GetBdrAttribute(int i) { return mesh -> GetBdrAttribute(i); };

   /// Returns indexes of degrees of freedom in array dofs for i'th element.
   virtual void GetElementDofs (int i, Array<int> &dofs) const;

   /// Returns indexes of degrees of freedom for i'th boundary element.
   virtual void GetBdrElementDofs (int i, Array<int> &dofs) const;

   /** Returns the indexes of the degrees of freedom for i'th face
       including the dofs for the edges and the vertices of the face. */
   void GetFaceDofs (int i, Array<int> &dofs) const;

   /** Returns the indexes of the degrees of freedom for i'th edge
       including the dofs for the vertices of the edge. */
   void GetEdgeDofs (int i, Array<int> &dofs) const;

   void GetElementInteriorDofs (int i, Array<int> &dofs) const;

   void GetEdgeInteriorDofs (int i, Array<int> &dofs) const;

   void DofsToVDofs (Array<int> &dofs) const;

   void DofsToVDofs (int vd, Array<int> &dofs) const;

   int DofToVDof (int dof, int vd) const;

   int VDofToDof(int vdof) const
   { return (ordering == Ordering::byNODES) ? (vdof%ndofs) : (vdof/vdim); }

   static void AdjustVDofs (Array<int> &vdofs);

   /// Returns indexes of degrees of freedom in array dofs for i'th element.
   void GetElementVDofs (int i, Array<int> &dofs) const;

   /// Returns indexes of degrees of freedom for i'th boundary element.
   void GetBdrElementVDofs (int i, Array<int> &dofs) const;

   /// Returns indexes of degrees of freedom for i'th face element.
   void GetFaceVDofs (int iF, Array<int> &dofs) const;

   /// Returns indexes of degrees of freedom for i'th edge.
   void GetEdgeVDofs (int iE, Array<int> &dofs) const;

   void GetElementInteriorVDofs (int i, Array<int> &vdofs) const;

   void GetEdgeInteriorVDofs (int i, Array<int> &vdofs) const;

   void BuildElementToDofTable();

   void BuildDofToArrays();

   const Table &GetElementToDofTable() const { return *elem_dof; }

   int GetElementForDof  (int i) { return dof_elem_array[i]; }
   int GetLocalDofForDof (int i) { return dof_ldof_array[i]; }

   /// Returns pointer to the FiniteElement associated with i'th element.
   const FiniteElement * GetFE (int i) const;

   /// Returns pointer to the FiniteElement for the i'th boundary element.
   const FiniteElement * GetBE (int i) const;

   /** Return the restriction matrix from this FE space to the coarse FE space
       'cfes'. Both FE spaces must use the same FE collection and be defined on
       the same Mesh which must be in TWO_LEVEL_* state.  When vdim > 1,
       'one_vdim' specifies whether the restriction matrix built should be the
       scalar restriction (one_vdim=1) or the full vector restriction
       (one_vdim=0); if one_vdim=-1 then the behavior depends on the ordering of
       this FE space: if ordering=byNodes then the scalar restriction matrix is
       built and if ordering=byVDim -- the full vector restriction matrix.  */
   SparseMatrix * GlobalRestrictionMatrix (FiniteElementSpace *cfes,
                                           int one_vdim = -1);

   /// Determine the boundary degrees of freedom
   virtual void GetEssentialVDofs(Array<int> &bdr_attr_is_ess,
                                  Array<int> &ess_dofs);

   void EliminateEssentialBCFromGRM (FiniteElementSpace *cfes,
                                     Array<int> &bdr_attr_is_ess,
                                     SparseMatrix *R);

   /// Generate the global restriction matrix with eliminated essential bc
   SparseMatrix * GlobalRestrictionMatrix(FiniteElementSpace *cfes,
                                          Array<int> &bdr_attr_is_ess,
                                          int one_vdim = -1);

   /** Generate the global restriction matrix from a discontinuous
       FE space to the continuous FE space of the same polynomial degree. */
   SparseMatrix * D2C_GlobalRestrictionMatrix(FiniteElementSpace *cfes);

   /** Generate the global restriction matrix from a discontinuous
       FE space to the piecewise constant FE space. */
   SparseMatrix * D2Const_GlobalRestrictionMatrix(FiniteElementSpace *cfes);

   /** Construct the restriction matrix from the FE space given by
       (*this) to the lower degree FE space given by (*lfes) which
       is defined on the same mesh. */
   SparseMatrix * H2L_GlobalRestrictionMatrix (FiniteElementSpace *lfes);

   virtual void Update();
   /// Return a copy of the current FE space and update
   virtual FiniteElementSpace *SaveUpdate();

   void Save (ostream &out) const;

   virtual ~FiniteElementSpace();
};

#endif
