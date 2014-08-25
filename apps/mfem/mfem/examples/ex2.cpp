//                                MFEM Example 2
//
// Compile with: make ex2
//
// Sample runs:  ex2 ../data/beam-tri.mesh
//               ex2 ../data/beam-quad.mesh
//               ex2 ../data/beam-tet.mesh
//               ex2 ../data/beam-hex.mesh
//               ex2 ../data/beam-quad-nurbs.mesh
//               ex2 ../data/beam-hex-nurbs.mesh
//
// Description:  This example code solves a simple linear elasticity problem
//               describing a multi-material cantilever beam.
//
//               Specifically, we approximate the weak form of -div(sigma(u))=0
//               where sigma(u)=lambda*div(u)*I+mu*(grad*u+u*grad) is the stress
//               tensor corresponding to displacement field u, and lambda and mu
//               are the material Lame constants. The boundary conditions are
//               u=0 on the fixed part of the boundary with attribute 1, and
//               sigma(u).n=f on the remainder with f being a constant pull down
//               vector on boundary elements with attribute 2, and zero
//               otherwise. The geometry of the domain is assumed to be as
//               follows:
//
//                                 +----------+----------+
//                    boundary --->| material | material |<--- boundary
//                    attribute 1  |    1     |    2     |     attribute 2
//                    (fixed)      +----------+----------+     (pull down)
//
//               The example demonstrates the use of high-order and NURBS vector
//               finite element spaces with the linear elasticity bilinear form,
//               meshes with curved elements, and the definition of piece-wise
//               constant and vector coefficient objects.
//
//               We recommend viewing example 1 before viewing this example.

#include <fstream>
#include "sight.h"
#include "mfem.h"
#include "mfem.hpp"

using namespace sight;

int main (int argc, char *argv[])
{
   Mesh *mesh;

   SightInit(argc, argv, "ex1", "dbg.MFEM.ex2");

   if (argc == 1)
   {
      cout << "\nUsage: ex2 <mesh_file>\n" << endl;
      return 1;
   }

   // 1. Read the mesh from the given mesh file. We can handle triangular,
   //    quadrilateral, tetrahedral or hexahedral elements with the same code.
   ifstream imesh(argv[1]);
   if (!imesh)
   {
      cerr << "\nCan not open mesh file: " << argv[1] << '\n' << endl;
      return 2;
   }
   mesh = new Mesh(imesh, 1, 1);
   imesh.close();

   if (mesh->attributes.Max() < 2 || mesh->bdr_attributes.Max() < 2)
   {
      cerr << "\nInput mesh should have at least two materials and "
           << "two boundary attributes! (See schematic in ex2.cpp)\n"
           << endl;
      return 3;
   }

   int dim = mesh->Dimension();

   // 2. Select the order of the finite element discretization space. For NURBS
   //    meshes, we increase the order by degree elevation.
   int p;
   if(argc==3) p = atoi(argv[2]);
   else {
     cout << "Enter finite element space order --> " << flush;
     cin >> p;
   }

   if (mesh->NURBSext && p > mesh->NURBSext->GetOrder())
      mesh->DegreeElevate(p - mesh->NURBSext->GetOrder());

   // 3. Refine the mesh to increase the resolution. In this example we do
   //    'ref_levels' of uniform refinement. We choose 'ref_levels' to be the
   //    largest number that gives a final mesh with no more than 5,000
   //    elements.
   {
      int ref_levels =
         (int)floor(log(5000./mesh->GetNE())/log(2.)/dim);
      for (int l = 0; l < ref_levels; l++)
         mesh->UniformRefinement();
   }

   // 4. Define a finite element space on the mesh. Here we use vector finite
   //    elements, i.e. dim copies of a scalar finite element space. The vector
   //    dimension is specified by the last argument of the FiniteElementSpace
   //    constructor. For NURBS meshes, we use the (degree elevated) NURBS space
   //    associated with the mesh nodes.
   FiniteElementCollection *fec;
   FiniteElementSpace *fespace;
   if (mesh->NURBSext)
   {
      fec = NULL;
      fespace = mesh->GetNodes()->FESpace();
   }
   else
   {
      fec = new H1_FECollection(p, dim);
      fespace = new FiniteElementSpace(mesh, fec, dim);
   }
   dbg << "Number of unknowns: " << fespace->GetVSize() << endl
        << "Assembling: " << flush;

   // 5. Set up the linear form b(.) which corresponds to the right-hand side of
   //    the FEM linear system. In this case, b_i equals the boundary integral
   //    of f*phi_i where f represents a "pull down" force on the Neumann part
   //    of the boundary and phi_i are the basis functions in the finite element
   //    fespace. The force is defined by the VectorArrayCoefficient object f,
   //    which is a vector of Coefficient objects. The fact that f is non-zero
   //    on boundary attribute 2 is indicated by the use of piece-wise constants
   //    coefficient for its last component.
   VectorArrayCoefficient f(dim);
   for (int i = 0; i < dim-1; i++)
      f.Set(i, new ConstantCoefficient(0.0));
   {
      Vector pull_force(mesh->bdr_attributes.Max());
      pull_force = 0.0;
      pull_force(1) = -1.0e-2;
      f.Set(dim-1, new PWConstCoefficient(pull_force));
   }

   LinearForm *b = new LinearForm(fespace);
   b->AddBoundaryIntegrator(new VectorBoundaryLFIntegrator(f));
   dbg << "r.h.s. ... " << flush;
   b->Assemble();

   // 6. Define the solution vector x as a finite element grid function
   //    corresponding to fespace. Initialize x with initial guess of zero,
   //    which satisfies the boundary conditions.
   GridFunction x(fespace);
   x = 0.0;

   // 7. Set up the bilinear form a(.,.) on the finite element space
   //    corresponding to the linear elasticity integrator with piece-wise
   //    constants coefficient lambda and mu. The boundary conditions are
   //    implemented by marking only boundary attribute 1 as essential. After
   //    assembly and finalizing we extract the corresponding sparse matrix A.
   Vector lambda(mesh->attributes.Max());
   lambda = 1.0;
   lambda(0) = lambda(1)*50;
   PWConstCoefficient lambda_func(lambda);
   Vector mu(mesh->attributes.Max());
   mu = 1.0;
   mu(0) = mu(1)*50;
   PWConstCoefficient mu_func(mu);

   BilinearForm *a = new BilinearForm(fespace);
   a->AddDomainIntegrator(new ElasticityIntegrator(lambda_func,mu_func));
   dbg << "matrix ... " << flush;
   a->Assemble();
   Array<int> ess_bdr(mesh->bdr_attributes.Max());
   ess_bdr = 0;
   ess_bdr[0] = 1;
   a->EliminateEssentialBC(ess_bdr, x, *b);
   a->Finalize();
   dbg << "done." << endl;
   const SparseMatrix &A = a->SpMat();

   // 8. Define a simple symmetric Gauss-Seidel preconditioner and use it to
   //    solve the system Ax=b with PCG.
   GSSmoother M(A);
   PCG(A, M, *b, x, 1, 500, 1e-8, 0.0);

   // 9. For non-NURBS meshes, make the mesh curved based on the finite element
   //    space. This means that we define the mesh elements through a fespace
   //    based transformation of the reference element. This allows us to save
   //    the displaced mesh as a curved mesh when using high-order finite
   //    element displacement field. We assume that the initial mesh (read from
   //    the file) is not higher order curved mesh compared to the chosen FE
   //    space.
   if (!mesh->NURBSext)
      mesh->SetNodalFESpace(fespace);

   // 10. Save the displaced mesh and the inverted solution (which gives the
   //     backward displacements to the original grid). This output can be
   //     viewed later using GLVis: "glvis -m displaced.mesh -g sol.gf".
   {
      GridFunction *nodes = mesh->GetNodes();
      *nodes += x;
      x *= -1;
      ofstream mesh_ofs("displaced.mesh");
      mesh_ofs.precision(8);
      mesh->Print(mesh_ofs);
      ofstream sol_ofs("sol.gf");
      sol_ofs.precision(8);
      x.Save(sol_ofs);
   }

   // 11. (Optional) Send the above data by socket to a GLVis server. Use the
   //     "n" and "b" keys in GLVis to visualize the displacements.
   /*char vishost[] = "localhost";
   int  visport   = 19916;
   osockstream sol_sock(visport, vishost);
   sol_sock << "solution\n";
   sol_sock.precision(8);
   mesh->Print(sol_sock);
   x.Save(sol_sock);
   sol_sock.send();*/
   #if defined(VNC_ENABLED)
   mfem::emitMesh(mesh, &x);
   #endif

   // 12. Free the used memory.
   delete a;
   delete b;
   if (fec)
   {
      delete fespace;
      delete fec;
   }
   delete mesh;

   return 0;
}
