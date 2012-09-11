NAS Parallel Benchmarks Version 3.3 (NPB3.3)
------------------------------------------------

  NAS Parallel Benchmarks Team
  NASA Ames Research Center
  Mail Stop: T27A-1
  Moffett Field, CA   94035-1000

  E-mail:  npb@nas.nasa.gov                                      
  Fax:     (650) 604-3957                                        
  http://www.nas.nasa.gov/Software/NPB/


================================================
INSTALLATION

  For documentation on installing and running the NAS Parallel
  Benchmarks, refer to subdirectory README files.


================================================
BACKGROUND

  Information on NPB 3.3, including the technical reports, the          
  original specifications, source code, results and information        
  on how to submit new results, is available at:                       

     http://www.nas.nasa.gov/Software/NPB/                              


================================================
Summary of New Features and Improvements
 (Details are given in Changes.log.)


 in NPB3.3.1 from NPB3.3:

  - Bug fixes for:
      MPI/FT - non-portable way of broadcasting input parameters
      {OMP,SER}/DC - access to out-of-bound array elements
      {OMP,SER}/UA - use of uninitialized array

  - Code clean up in MPI/LU: avoid using MPI_ANY_SOURCE and delete
      unused codes

  - Additional timers are included in the MPI version

  - Executables produced for OMP and SER now use ".x" as an extension


 in NPB3.3 from NPB3.2.1:

  - Introduction of the Class E problem in seven of the benchmarks
    (BT, SP, LU, CG, MG, FT, and EP) to stress larger size parallel 
    computers.

  - Class D added to the IS benchmark in all three implementations.

  - Enable the Bucket sort option for OMP/IS.

  - Introduction of the "twiddle" array in the OpenMP FT benchmark
    to improve performance

  - Array padding in MPI/SP was adjusted to improve performance

  - Merge the vector codes for the BT and LU benchmarks into this
    release.

  - The hyperplane version of LU (LU-HP) is no longer included 
    in the distribution.  Download NPB3.2.1 if needed.


 in NPB3.2.1 from NPB3.2:

  - A number of bug fixes for the MPI versions of {FT, LU, MG, BT} and 
    the OpenMP version of LU

  - Improvements on the OpenMP versions of {EP, IS, UA}
    (see *OMP/UA/README for a special note on UA)


 in NPB3.2 from NPB3.1:

  - Serial DC was converted to C from C++ (only classes S, W, A and B
    are available)

  - OpenMP version of DC was added (only classes S, W, A and B
    are available)

  - Inclusion of the new DT benchmark (MPI)


 in NPB3.1 from NPB3.0 & NPB2.4:

  - MPI, OpenMP, and Serial versions are now merged into one package

  - Inclusion of the Class D problem in both serial and OpenMP versions

  - Inclusion of the new UA benchmark (Serial & OpenMP)

  - Inclusion of "LU-HP" in the OpenMP version

  - Inclusion of the new DC benchmark (Serial)

  - Use of relative errors for verification in both CG and MG

  - Change in problem parameters for MG Class W


The NPB IO benchmark is part of NPB3.3-MPI.  Check the README file
in that subdirectory for additional information.

The Java and HPF implementations are not included in this distribution.
Please use the NPB3.0 distribution.

