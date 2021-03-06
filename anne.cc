//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC//    Version 1.0; svn revision $LastChangedRevision: 1097 $
//LIC//
//LIC// $LastChangedDate: 2015-12-17 11:53:17 +0000 (Thu, 17 Dec 2015) $
//LIC// 
//LIC// Copyright (C) 2006-2016 Matthias Heil and Andrew Hazel
//LIC// 
//LIC// This library is free software; you can redistribute it and/or
//LIC// modify it under the terms of the GNU Lesser General Public
//LIC// License as published by the Free Software Foundation; either
//LIC// version 2.1 of the License, or (at your option) any later version.
//LIC// 
//LIC// This library is distributed in the hope that it will be useful,
//LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
//LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//LIC// Lesser General Public License for more details.
//LIC// 
//LIC// You should have received a copy of the GNU Lesser General Public
//LIC// License along with this library; if not, write to the Free Software
//LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//LIC// 02110-1301  USA.
//LIC// 
//LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
//LIC// 
//LIC//====================================================================
// Driver for Anne's MSc project

// The oomphlib headers
#include "generic.h"
#include "navier_stokes.h"

// The mesh
#include "meshes/rectangular_quadmesh.h"
#include "vorticity_smoother.h"

using namespace std;
using namespace oomph;


#ifdef OOMPH_HAS_MUMPS
//=============================================================================
/// helper method for the block diagonal F block preconditioner to allow 
/// mumps to be used for as a subsidiary block preconditioner
//=============================================================================
namespace Mumps_Subsidiary_Preconditioner_Helper
{
 Preconditioner* set_mumps_preconditioner()
 {
  return new NewMumpsPreconditioner;
 }
}
#endif


//===start_of_namespace=================================================
/// Namespace for global parameters
//======================================================================
namespace Global_Parameters
{
 /// Reynolds number
 double Re=4000.0;

 /// Left end of computational domain
 double X_left=-3.63;
 
 /// Right end of computational domain
 double X_right=3.63;

 /// Height of computational domain
 double Height=5.13;




 // Mesh parameters:
 //-----------------

 // The parameters below work fine for basic layout.
 // Overall number of elements can be controlled by scalng
 // factor
 
 /// Overall scaling factor for number of elements
 double Mesh_scaling_factor=0.5;

 /// Percentage of elements squashed into "boundary layer"
 double Percentage_of_elements_in_bl=50.0;

 /// Exponential factor mesh squashing in BL region
 double Alpha_bl=5.0;

 // Mesh spacing in vortex region
 double H_vortex=0.05;

 // Mesh spacing in vortex region
 double Hx_outside_vortex=0.2;

 /// Thickness of the middle region into which we squash
 /// the elements
 double X_mid=4.5;



 // Parameters for vortex
 //----------------------

 /// x position of vortex
 double X_vortex=0.0;

 /// x position of vortex
 double Y_vortex=1.0;

 /// Uniform background flow
 double K=0.02812;

 /// Initial condition for velocity
 void initial_condition(const Vector<double>& x, Vector<double>& u)
 {
  // Parameters from Morten's thesis
  double a=0.3;
  double omega_0=-1.25;

  // Top vortex
  double r1=sqrt(pow(x[0]-X_vortex,2)+
                pow(x[1]-Y_vortex,2));
  double theta1=atan2(x[1]-Y_vortex,x[0]-X_vortex);
  double u_theta1=omega_0*a*a/(2.0*r1)*(1.0-exp(-r1*r1/(a*a)));

  // Bottom vortex
  double r2=sqrt(pow(x[0]-X_vortex,2)+
                 pow(x[1]+Y_vortex,2));
  double theta2=atan2(x[1]+Y_vortex,x[0]-X_vortex);
  double u_theta2=omega_0*a*a/(2.0*r2)*(1.0-exp(-r2*r2/(a*a)));

  u[0]=-u_theta1*sin(theta1)+u_theta2*sin(theta2)+K;
  u[1]= u_theta1*cos(theta1)-u_theta2*cos(theta2);
 }


 /// sin/cos velocity field for validation of vorticity projection
 void sin_cos_velocity_field(const Vector<double>& x, Vector<double>& veloc)
 {
  double omega_x=2.0*MathematicalConstants::Pi/(-X_left+X_right);
  double phi=2.0*MathematicalConstants::Pi*X_left/(-X_left+X_right);
  double omega_y=2*MathematicalConstants::Pi/Height;

  veloc[0]=sin(omega_x*x[0]+phi)*cos(omega_y*x[1]);
  veloc[1]=sin(omega_x*x[0]+phi)*sin(omega_y*x[1]);
 }


 /// vorticity (and derivs) associated with sin/cos velocity for
 /// validation of vorticity
 void sin_cos_vorticity(const Vector<double>& x, 
                        double& vort,
                        Vector<double>& dvort_dx,
                        Vector<double>& dvort_dxdy,
                        Vector<double>& dvort_dxdxdy,
                        Vector<double>& dveloc_dx)
 {
  double omega_x=2.0*MathematicalConstants::Pi/(-X_left+X_right);
  double phi=2.0*MathematicalConstants::Pi*X_left/(-X_left+X_right);
  double omega_y=2*MathematicalConstants::Pi/Height;

  dveloc_dx[0]= omega_x*cos(omega_x*x[0]+phi)*cos(omega_y*x[1]);
  dveloc_dx[1]=-omega_y*sin(omega_x*x[0]+phi)*sin(omega_y*x[1]);

  dveloc_dx[2]=omega_x*cos(omega_x*x[0]+phi)*sin(omega_y*x[1]);
  dveloc_dx[3]=omega_y*sin(omega_x*x[0]+phi)*cos(omega_y*x[1]);

  vort=
   omega_y*sin(omega_x*x[0]+phi)*sin(omega_y*x[1])+
   omega_x*cos(omega_x*x[0]+phi)*sin(omega_y*x[1]);


  dvort_dx[0]=
   omega_x*omega_y*cos(omega_x*x[0]+phi)*sin(omega_y*x[1])-
   omega_x*omega_x*sin(omega_x*x[0]+phi)*sin(omega_y*x[1]);

  dvort_dx[1]=
   omega_y*omega_y*sin(omega_x*x[0]+phi)*cos(omega_y*x[1])+
   omega_x*omega_y*cos(omega_x*x[0]+phi)*cos(omega_y*x[1]);


  dvort_dxdy[0]=-omega_x*omega_x*vort;

  dvort_dxdy[1]=
   omega_x*omega_y*omega_y*cos(omega_x*x[0]+phi)*cos(omega_y*x[1])-
   omega_x*omega_x*omega_y*sin(omega_x*x[0]+phi)*cos(omega_y*x[1]);

  dvort_dxdy[2]=-omega_y*omega_y*vort;


  dvort_dxdxdy[0]=
   -omega_x*omega_x*omega_x*omega_y*cos(omega_x*x[0]+phi)*sin(omega_y*x[1])+
   omega_x*omega_x*omega_x*omega_x*sin(omega_x*x[0]+phi)*sin(omega_y*x[1]);
  
  dvort_dxdxdy[1]=
   -omega_x*omega_x*omega_y*sin(omega_x*x[0]+phi)*omega_y*cos(omega_y*x[1])
   -omega_x*omega_x*omega_x*cos(omega_x*x[0]+phi)*omega_y*cos(omega_y*x[1]);


  dvort_dxdxdy[2]=
   -omega_x*omega_y*cos(omega_x*x[0]+phi)*omega_y*omega_y*sin(omega_y*x[1])+
   omega_x*omega_x*sin(omega_x*x[0]+phi)*omega_y*omega_y*sin(omega_y*x[1]);

  dvort_dxdxdy[3]=
   -omega_y*sin(omega_x*x[0]+phi)*omega_y*omega_y*omega_y*cos(omega_y*x[1])
   -omega_x*cos(omega_x*x[0]+phi)*omega_y*omega_y*omega_y*cos(omega_y*x[1]);

 }


 /// Synthetic velocity field for validation
 void synthetic_velocity_field(const Vector<double>& xx, 
                               Vector<double>& veloc)
 {
  sin_cos_velocity_field(xx,veloc);
 }



 /// Synthetic vorticity field and derivs for validation
 void synthetic_vorticity(const Vector<double>& xx, 
                          double& vort,
                          Vector<double>& dvort_dx,
                          Vector<double>& dvort_dxdy,
                          Vector<double>& dvort_dxdxdy,
                          Vector<double>& dveloc_dx)
 {
  sin_cos_vorticity(xx,
                    vort,                        
                    dvort_dx,
                    dvort_dxdy,
                    dvort_dxdxdy,
                    dveloc_dx);
 }






} // end of namespace


//===start_of_problem_class=============================================
/// Problem class for Anne's MSc problem
//======================================================================
template<class ELEMENT>
class AnneProblem : public Problem
{
public:

 /// Constructor:
 AnneProblem();

 //Update before solve is empty
 void actions_before_newton_solve() {}

 /// \short Update after solve is empty
 void actions_after_newton_solve() {}
 
 /// Actions before adapt: empty
 void actions_before_adapt(){} 

 /// After adaptation
 void actions_after_adapt()
  {
   complete_problem_setup();
  }
   
 /// Doc the solution
 void doc_solution(DocInfo& doc_info);

 /// Impose no slip and re-assign eqn numbers
 void impose_no_slip_on_bottom_boundary();

/// Assign synthetic flow field
 void assign_synthetic_veloc_field();

 /// Check vorticity smoothing
 void check_smoothed_vorticity(DocInfo& doc_info);

private:

 /// Complete problem setup
 void complete_problem_setup();

 /// oomph-lib iterative linear solver
 IterativeLinearSolver* Solver_pt;
 
 /// Preconditioner
 NavierStokesSchurComplementPreconditioner* Prec_pt;

 /// Inexact solver for P block
 Preconditioner* P_matrix_preconditioner_pt;

 /// Inexact solver for F block
 Preconditioner* F_matrix_preconditioner_pt;

 /// Vorticity recoverer
 VorticitySmoother<ELEMENT>*  Vorticity_recoverer_pt;

}; // end of problem_class


//===start_of_constructor=============================================
/// Problem constructor
//====================================================================
template<class ELEMENT>
AnneProblem<ELEMENT>::AnneProblem()
{

 // Make an instance of the vorticity recoverer
 unsigned nrecovery_order=2;
 Vorticity_recoverer_pt=new VorticitySmoother<ELEMENT>(nrecovery_order);


 //Allocate the timestepper
 add_time_stepper_pt(new BDF<2>); 

 // Number of elements in x direction.
 // purely nominal initial value
 unsigned nx=10;

 // Number of elements in y direction
 // purely nominal initial value
 unsigned ny=10; 

 // Left end of computational domain
 double x_min=Global_Parameters::X_left;
 
 // Right end of computational domain
 double x_max=Global_Parameters::X_right;
 
 // Botton of computational domain
 double y_min=0.0;

 // Height of computational domain
 double y_max=Global_Parameters::Height;
 
 // Parameters for mesh squashing:
 //-------------------------------

 // Mesh spacing in vortex region
 double dy_vortex=Global_Parameters::H_vortex;

 // Width of central region containing vortex
 double x_mid=Global_Parameters::X_mid;

 // Make approximately square elements in vortex region
 // make sure it's an even number
 unsigned nx_mid=2*unsigned(0.5*x_mid/dy_vortex);

 // Mesh spacing in side bits -- make sure that in total we
 // have an even number of elements
 double n_side=2*unsigned(0.5*(x_max-x_min-x_mid)/
                          Global_Parameters::Hx_outside_vortex);

 // Mesh squashing exponential factor
 double alpha=Global_Parameters::Alpha_bl;

 // Identify point in current mesh where we switch from exponential
 // to uniform spacing
 double percentage_el_in_bl=Global_Parameters::Percentage_of_elements_in_bl;
 unsigned ny_bl=unsigned(double(ny)*percentage_el_in_bl/100.0);
 double y_bl_orig=double(ny_bl)/double(ny)*y_max;


 // Constants related to mapping functions
 double dfdy_at_y_bl_orig=alpha*exp(alpha)/(y_bl_orig*(exp(alpha)-1.0));
 double normalising_factor=1.0+dfdy_at_y_bl_orig*(y_max-y_bl_orig);

 oomph_info << "outer edge of exponential region: " 
            << y_max/normalising_factor << std::endl;

 // Now re-assign the actual number of elements in y direction
 oomph_info << "Changed ny from: " << ny;
 double current_dy_vortex=(y_max-y_max/normalising_factor)/
  (double(ny)*(100.0-percentage_el_in_bl)/100.0);
 ny*=current_dy_vortex/dy_vortex;
 oomph_info << " to: " << ny << std::endl;

 // Scale it
 nx_mid=2*unsigned(0.5*double(nx_mid)*Global_Parameters::Mesh_scaling_factor);
 n_side=2*unsigned(0.5*double(n_side)*Global_Parameters::Mesh_scaling_factor);
 ny=unsigned(double(ny)*Global_Parameters::Mesh_scaling_factor);
                  
 // How many in total?
 nx=nx_mid+n_side;


 // Ignore all this for vorticity convergence check
 if (CommandLineArgs::command_line_flag_has_been_set("--validate_projection"))
  {
   nx=10;
   ny=10;
  }

 oomph_info << "And finally building mesh with " << nx << " x " << ny 
            << " elements." << std::endl;

 //Now create the mesh 
 mesh_pt() = 
  new RefineableRectangularQuadMesh<ELEMENT>(nx,ny,x_min,x_max,y_min,y_max,
                                             time_stepper_pt());

 // No squashing if projection validation
 if (!CommandLineArgs::command_line_flag_has_been_set("--validate_projection"))
  {
   // Adjust nodes vertically
   unsigned nnod=mesh_pt()->nnode();
   for (unsigned j=0;j<nnod;j++)
    {
     double y=mesh_pt()->node_pt(j)->x(1);
     
     if (y<=y_bl_orig)
      {
       mesh_pt()->node_pt(j)->x(1)=
        y_max*(exp(alpha*(y/y_bl_orig))-1.0)/(exp(alpha)-1.0)/
        normalising_factor;
      }
     else 
      {
       mesh_pt()->node_pt(j)->x(1)=y_max*
        (1.0+dfdy_at_y_bl_orig*(y-y_bl_orig))/normalising_factor;
      }
    }
   
   // Squash it to middle region
   double dx_uniform=(x_max-x_min)/double(nx);
   double x_left_orig=-double(nx_mid/2)*dx_uniform;
   double x_right_orig=-x_left_orig;
   unsigned no_nod=mesh_pt()->nnode();
   for (unsigned j=0;j<no_nod;j++)
    {
     double x=mesh_pt()->node_pt(j)->x(0);
     if (x<=x_right_orig && x>=x_left_orig)
      {
       mesh_pt()->node_pt(j)->x(0)=x_mid*x/(x_right_orig-x_left_orig);
      }
     else if (x<=x_left_orig)
      {
       mesh_pt()->node_pt(j)->x(0)=-x_mid/2.0+
        (x_min+x_mid/2.0)*(x-x_left_orig)/(x_min-x_left_orig);
      }
     else 
      {
       mesh_pt()->node_pt(j)->x(0)=x_mid/2.0+
        (x_max-x_mid/2.0)*(x-x_right_orig)/(x_max-x_right_orig);
      }
    }
  }
 

 // Output mesh
 ofstream some_file;
 char filename[100]; 
  sprintf(filename,"mesh.dat");
 some_file.open(filename);
 unsigned npts=2;
 mesh_pt()->output(some_file,npts);
 some_file.close();

 // Check enumeration of boundaries
 mesh_pt()->output_boundaries("boundaries.dat");
 
 // Set the boundary conditions for this problem: All nodes are
 // free by default -- just pin the ones that have Dirichlet conditions
 // here
 unsigned num_bound=mesh_pt()->nboundary();
 for(unsigned ibound=0;ibound<num_bound;ibound++)
  {
   unsigned num_nod=mesh_pt()->nboundary_node(ibound);
   for (unsigned inod=0;inod<num_nod;inod++)
    {
     // Imposed velocity on top (2)
     if ((ibound==2)) 
      {
       mesh_pt()->boundary_node_pt(ibound,inod)->pin(0);
       mesh_pt()->boundary_node_pt(ibound,inod)->pin(1);
      }
     // Horizontal outflow on the left (3) and right (1) and no penetration
     // at bottom (0)
     else if ((ibound==0)||(ibound==1)|| (ibound==3) ) 
      {
       mesh_pt()->boundary_node_pt(ibound,inod)->pin(1);
      }
    }
  } // end loop over boundaries

 
 //Complete the problem setup to make the elements fully functional
 complete_problem_setup();

 //Assign equation numbers
 assign_eqn_numbers();

 // Setup initial condition
 //------------------------
 
 // Loop over nodes
 Vector<double> x(2);
 Vector<double> u(2);
 unsigned num_nod = mesh_pt()->nnode();
 for (unsigned n=0;n<num_nod;n++)
  {
   // Get nodal coordinates
   x[0]=mesh_pt()->node_pt(n)->x(0);
   x[1]=mesh_pt()->node_pt(n)->x(1);
   
   // Get initial velocity field
   Global_Parameters::initial_condition(x,u);
   
   // Assign solution
   mesh_pt()->node_pt(n)->set_value(0,u[0]);
   mesh_pt()->node_pt(n)->set_value(1,u[1]);
  }


 // Linear solver
 //--------------
 if (CommandLineArgs::command_line_flag_has_been_set("--use_oomph_gmres"))
  {
   // Use GMRES
   Solver_pt=new GMRES<CRDoubleMatrix>;   
   linear_solver_pt()=Solver_pt;
   
   // Set preconditioner
   Prec_pt=new NavierStokesSchurComplementPreconditioner(this);
   Prec_pt->set_navier_stokes_mesh(this->mesh_pt());  
   Solver_pt->preconditioner_pt()=Prec_pt;


#ifdef OOMPH_HAS_MUMPS

   // Schur complement preconditioner
   P_matrix_preconditioner_pt = new NewMumpsPreconditioner;
   Prec_pt->set_p_preconditioner(P_matrix_preconditioner_pt);

#endif



// No good with squashed elements

// #ifdef OOMPH_HAS_HYPRE

//    bool use_hypre_for_pressure=true;
//    if (use_hypre_for_pressure)
//     {
//      P_matrix_preconditioner_pt = new HyprePreconditioner;
     
//      // Set parameters for use as preconditioner on Poisson-type problem
//      Hypre_default_settings::set_defaults_for_2D_poisson_problem(
//       static_cast<HyprePreconditioner*>(P_matrix_preconditioner_pt));
     
//      // Use Hypre for the Schur complement block
//      Prec_pt->set_p_preconditioner(P_matrix_preconditioner_pt);
     
//      // Shut up!
//      static_cast<HyprePreconditioner*>(P_matrix_preconditioner_pt)->
//       disable_doc_time();
//     }

// #endif



   // Create internal preconditioners used on momentum block
   //--------------------------------------------------------
   bool use_block_diagonal_for_momentum=true;
   if (use_block_diagonal_for_momentum)
    {
     F_matrix_preconditioner_pt = 
      new BlockDiagonalPreconditioner<CRDoubleMatrix>;

#ifdef OOMPH_HAS_MUMPS

     // Use mumps for block solves
     dynamic_cast<BlockDiagonalPreconditioner<CRDoubleMatrix>* >
      (F_matrix_preconditioner_pt)->set_subsidiary_preconditioner_function
      (Mumps_Subsidiary_Preconditioner_Helper::set_mumps_preconditioner);

#endif

// #ifdef OOMPH_HAS_HYPRE
//      if (use_hypre_for_momentum)
//       {
//        dynamic_cast<BlockDiagonalPreconditioner<CRDoubleMatrix>* >
//         (F_matrix_preconditioner_pt)->set_subsidiary_preconditioner_function
//         (Hypre_Subsidiary_Preconditioner_Helper::set_hypre_preconditioner);
//       }
// #endif
     
     // Use Hypre for momentum block 
     Prec_pt->set_f_preconditioner(F_matrix_preconditioner_pt);
    }

  }
 
 
} // end of constructor




//==start_of_doc_solution=================================================
/// Doc the solution
//========================================================================
template<class ELEMENT>
void AnneProblem<ELEMENT>::doc_solution(DocInfo& doc_info)
{ 

 // Reconstruct smooth vorticity
 Vorticity_recoverer_pt->recover_vorticity(mesh_pt());
 
 ofstream some_file;
 char filename[100];

 // Number of plot points
 unsigned npts=5; 

 // Output solution 
 sprintf(filename,"%s/soln%i.dat",doc_info.directory().c_str(),
         doc_info.number());
 some_file.open(filename);
 mesh_pt()->output(some_file,npts);
 some_file.close();

 // Output analytical vorticity and derivs -- uses fake (zero) data for
 // veloc and pressure
 if (CommandLineArgs::command_line_flag_has_been_set("--validate_projection"))
  {
   sprintf(filename,
           "%s/analytical_vorticity_and_indicator%i.dat",
           doc_info.directory().c_str(),
           doc_info.number());
   some_file.open(filename);
   unsigned nel=mesh_pt()->nelement();
   for (unsigned e=0;e<nel;e++)
    {
     ELEMENT* el_pt=dynamic_cast<ELEMENT*>(mesh_pt()->element_pt(e));
     el_pt->output_analytical_veloc_and_vorticity(some_file,npts);
    }
   some_file.close();
  }

} // end_of_doc_solution   



//========================================================================
/// Complete problem setup
//========================================================================
template<class ELEMENT>
void AnneProblem<ELEMENT>::complete_problem_setup()
{

 // Unpin all pressure dofs
 RefineableNavierStokesEquations<2>::
  unpin_all_pressure_dofs(mesh_pt()->element_pt());
 
 //Loop over the elements
 unsigned n_el = mesh_pt()->nelement();
 for(unsigned e=0;e<n_el;e++)
  {
   //Cast to a fluid element
   ELEMENT *el_pt = dynamic_cast<ELEMENT*>(mesh_pt()->element_pt(e));

   //Set the Reynolds number
   el_pt->re_pt() = &Global_Parameters::Re;   
   el_pt->re_st_pt() = &Global_Parameters::Re;

   // Set exact solution for vorticity and derivs (for validation)
   el_pt->exact_vorticity_fct_pt()=&Global_Parameters::synthetic_vorticity;

   // Pin smoothed vorticity
   el_pt->pin_smoothed_vorticity();
  }

  // Pin redudant pressure dofs
  RefineableNavierStokesEquations<2>::
   pin_redundant_nodal_pressures(mesh_pt()->element_pt());
  
}




//========================================================================
/// Impose no slip and re-assign eqn numbers
//========================================================================
template<class ELEMENT>
void AnneProblem<ELEMENT>::impose_no_slip_on_bottom_boundary()
{


 // Pin horizontal velocity at bottom boundary and apply correction
 unsigned ibound=0;
 unsigned num_nod=mesh_pt()->nboundary_node(ibound);
 for (unsigned inod=0;inod<num_nod;inod++)
  {
   mesh_pt()->boundary_node_pt(ibound,inod)->pin(0);
   mesh_pt()->boundary_node_pt(ibound,inod)->set_value(0,Global_Parameters::K);
  }

 // Re-assign equation numbers
 oomph_info << std::endl 
            << "ndofs before applying no slip at bottom boundary: "
            << ndof() << std::endl;
 assign_eqn_numbers();
 oomph_info << "ndofs after applying no slip at bottom boundary : "
            << ndof() << std::endl << std::endl;
 
}


//==Start_of_assign_synth_flow_field======================================
/// Assign synthetic flow field
//========================================================================
template<class ELEMENT>
void AnneProblem<ELEMENT>::assign_synthetic_veloc_field()
{ 
 Vector<double> x(2);
 Vector<double> u(2);

 // Loop over all nodes
 unsigned nnod=mesh_pt()->nnode();
 for (unsigned j=0;j<nnod;j++)
  {
   Node* nod_pt=mesh_pt()->node_pt(j);
   x[0]=nod_pt->x(0);
   x[1]=nod_pt->x(1);
   Global_Parameters::synthetic_velocity_field(x,u);
   nod_pt->set_value(0,u[0]);
   nod_pt->set_value(1,u[1]);
  }
}


//==start_check_smoothed_vort=============================================
/// Check the smoothed vorticity
//========================================================================
template<class ELEMENT>
void AnneProblem<ELEMENT>::check_smoothed_vorticity(DocInfo& doc_info)
{ 
 ofstream some_file;
 char filename[10000];
 sprintf(filename,"%s/vorticity_convergence.dat",
         doc_info.directory().c_str());
 some_file.open(filename);
 some_file << "VARIABLES=\"nel\",\"sqrt(1/nel)\","
           << "\"Error(vort)\","
           << "\"Error(dvort/dx)\","
           << "\"Error(dvort/dy)\","
           << "\"Error(d^2vort/dx^2)\","
           << "\"Error(d^2vort/dxdy)\","
           << "\"Error(d^2vort/dy^2)\","
           << "\"Error(d^3vort/dx^3)\","
           << "\"Error(d^3vort/dx^2dy)\","
           << "\"Error(d^3vort/dxdy^2)\","
           << "\"Error(d^3vort/dy^3)\","
           << "\"Error(du/dx)\","
           << "\"Error(du/dy)\","
           << "\"Error(dv/dx)\","
           << "\"Error(dv/dy)\","
           << "\"Area\"\n";
 
 // Uniform mesh refinements
 unsigned n=5;
 for (unsigned ii=0;ii<n;ii++)
  {      
   // Assign synthetic velocity field
   assign_synthetic_veloc_field();

   // Smooth it!
   Vorticity_recoverer_pt->recover_vorticity(mesh_pt());
   
   // Get error in projection
   double full_area=0.0;
   Vector<double> full_error(14,0.0);
   unsigned nel=mesh_pt()->nelement();
   some_file << nel << " " << sqrt(1.0/double(nel)) << " ";
   for (unsigned e=0;e<nel;e++)
    {
     ELEMENT* el_pt=dynamic_cast<ELEMENT*>(mesh_pt()->element_pt(e));
     double size=el_pt->size();
     full_area+=size;
     for (unsigned i=0;i<14;i++)
      {
       double el_error=el_pt->vorticity_error_squared(i);
       full_error[i]+=el_error;
      }
    }
   
   for (unsigned i=0;i<14;i++)
    {
     some_file << sqrt(full_error[i]) << " ";
    }
   
   some_file << full_area << " " 
             << std::endl;
   

   doc_solution(doc_info);
   doc_info.number()++;

   // Refine 
   if (ii!=(n-1))
    {
     refine_uniformly();
    }
  }
 
 some_file.close();
 
 // Done!
 exit(0);
}



//===start_of_main======================================================
/// Driver code for Anne channel problem
//======================================================================
int main(int argc, char* argv[]) 
{

#ifdef OOMPH_HAS_MPI
 MPI_Helpers::init(argc,argv);
#endif

 // Store command line arguments
 CommandLineArgs::setup(argc,argv);

 // Validation of projection?
 CommandLineArgs::specify_command_line_flag("--validate_projection");

 // Reynolds number
 CommandLineArgs::specify_command_line_flag("--re",
                                            &Global_Parameters::Re);

 // Scaling factor for number of elements in mesh
 CommandLineArgs::specify_command_line_flag(
  "--mesh_scaling_factor",
  &Global_Parameters::Mesh_scaling_factor);

 // Use gmres?
 CommandLineArgs::specify_command_line_flag("--use_oomph_gmres");

 // Parse command line
 CommandLineArgs::parse_and_assign(); 
 
 // Doc what has actually been specified on the command line
 CommandLineArgs::doc_specified_flags();

 // Set up doc info
 DocInfo doc_info;
 doc_info.set_directory("RESLT");

 //Set up problem
 AnneProblem<VorticitySmootherElement<ProjectableTaylorHoodElement<RefineableQTaylorHoodElement<2> > > > 
  problem;
  
 // Check vorticity smoothing then stop
 if (CommandLineArgs::command_line_flag_has_been_set("--validate_projection"))
  {
   problem.check_smoothed_vorticity(doc_info);
  }
 
// Initialise all history values for an impulsive start
 double dt=0.1; 
 problem.initialise_dt(dt);
 problem.assign_initial_values_impulsive();

 // Number of timesteps until switch-over to no slip
 unsigned ntsteps=10;

 // Doc initial condition
 problem.doc_solution(doc_info);
 
 // increment counter
 doc_info.number()++;

 //Loop over the timesteps
 for(unsigned t=1;t<=ntsteps;t++)
  {
   oomph_info << "TIMESTEP " << t << std::endl;
   
   //Take one fixed timestep
   problem.unsteady_newton_solve(dt);

   //Output the time
   oomph_info << "Time is now " << problem.time_pt()->time() << std::endl;

   // Doc solution
   problem.doc_solution(doc_info);

   // increment counter
   doc_info.number()++;
  }

 // Now do no slip
 problem.impose_no_slip_on_bottom_boundary();

 //Loop over the remaining timesteps
 ntsteps=1300;
 for(unsigned t=1;t<=ntsteps;t++)
  {
   oomph_info << "TIMESTEP " << t << std::endl;
   
   //Take one fixed timestep
   problem.unsteady_newton_solve(dt);

   //Output the time
   oomph_info << "Time is now " << problem.time_pt()->time() << std::endl;

   // Doc solution
   problem.doc_solution(doc_info);

   // increment counter
   doc_info.number()++;
  }



#ifdef OOMPH_HAS_MPI
MPI_Helpers::finalize();
#endif


} // end of main
