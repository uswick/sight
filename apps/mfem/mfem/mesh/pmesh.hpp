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

#ifndef MFEM_PMESH
#define MFEM_PMESH

#include "sight_ns.hpp"

/// Class for parallel meshes
class ParMesh : public Mesh
{
private:
   MPI_Comm MyComm;
   int NRanks, MyRank;

   Array<Element *> shared_edges;
   Array<Element *> shared_faces;

   /// Shared objects in each group.
   Table group_svert;
   Table group_sedge;
   Table group_sface;

   /// Shared to local index mapping.
   Array<int> svert_lvert;
   Array<int> sedge_ledge;
   Array<int> sface_lface;

   /// Return a number(0-1) identifying how the given edge has been split
   int GetEdgeSplittings(Element *edge, const DSTable &v_to_v, int *middle);
   /// Return a number(0-4) identifying how the given face has been split
   int GetFaceSplittings(Element *face, const DSTable &v_to_v, int *middle);

   /// Refine quadrilateral mesh.
   virtual void QuadUniformRefinement();

   /// Refine a hexahedral mesh.
   virtual void HexUniformRefinement();

   virtual void NURBSUniformRefinement();

public:
   ParMesh(MPI_Comm comm, Mesh &mesh, int *partitioning_ = NULL,
           int part_method = 1);

   MPI_Comm GetComm() { return MyComm; }
   int GetNRanks() { return NRanks; }
   int GetMyRank() { return MyRank; }

   GroupTopology gtopo;

   int GetNGroups() { return gtopo.NGroups(); }

   // next 6 methods do not work for the 'local' group 0
   int GroupNVertices(int group) { return group_svert.RowSize(group-1); }
   int GroupNEdges(int group)    { return group_sedge.RowSize(group-1); }
   int GroupNFaces(int group)    { return group_sface.RowSize(group-1); }

   int GroupVertex(int group, int i)
   { return svert_lvert[group_svert.GetJ()[group_svert.GetI()[group-1]+i]]; }
   void GroupEdge(int group, int i, int &edge, int &o);
   void GroupFace(int group, int i, int &face, int &o);

   /// See the remarks for the serial version in mesh.hpp
   virtual void ReorientTetMesh();

   /// Refine the marked elements.
   virtual void LocalRefinement(const Array<int> &marked_el, int type = 3);

   /// Update the groups after tet refinement
   void RefineGroups(const DSTable &v_to_v, int *middle);

   /** Print the part of the mesh in the calling processor adding the interface
       as boundary (for visualization purposes) using the default format. */
   virtual void Print(ostream &out = sightN::dbg) const;

   /** Print the part of the mesh in the calling processor adding the interface
       as boundary (for visualization purposes) using Netgen/Truegrid format .*/
   virtual void PrintXG(ostream &out = sightN::dbg) const;

   /** Write the mesh to the stream 'out' on Process 0 in a form
       suitable for visualization: the mesh is written as a disjoint
       mesh and the shared boundary is added to the actual boundary;
       both the element and boundary attributes are set to the
       precessor number.  */
   void PrintAsOne(ostream &out = sightN::dbg);

   /// Old mesh format (Netgen/Truegrid) version of 'PrintAsOne'
   void PrintAsOneXG(ostream &out = sightN::dbg);

   /// Print various parallel mesh stats
   void PrintInfo(ostream &out = sightN::dbg);

   virtual ~ParMesh();
};

#endif
