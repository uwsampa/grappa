/** @mainpage Combinatorial BLAS Library (MPI reference implementation)
*
* @authors <a href="http://gauss.cs.ucsb.edu/~aydin"> Aydın Buluç </a>, <a href="http://cs.ucsb.edu/~gilbert"> John R. Gilbert </a>, <a href="http://www.cs.ucsb.edu/~alugowski/">Adam Lugowski</a>
*
* <i> This material is based upon work supported by the National Science Foundation under Grant No. 0709385. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the National Science Foundation (NSF) </i>
*
*
* @section intro Introduction
* The Combinatorial BLAS is an extensible distributed-memory parallel graph library offering a small but powerful set of linear
* algebra primitives specifically targeting graph analytics. 
* - The Combinatorial BLAS is also the backend of the Python <a href="http://kdt.sourceforge.net">
* Knowledge Discovery Toolbox (KDT) </a>. 
* - It achieves scalability via its two dimensional distribution and coarse-grained parallelism. 
* - For an illustrative overview, check out <a href="http://gauss.cs.ucsb.edu/~aydin/talks/CombBLAS_Nov11.pdf">these slides</a>. 
* - Check out the <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/FAQ-combblas.html">Frequently asked questions about CombBLAS</a>.
*
* <b>Download</b> 
* - Read <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/release-notes.html">release notes</a>.
* - The latest CMake'd tarball (version 1.3.0, Feb 2013) <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/CombBLAS_beta_13_0.tar.gz"> here</a>. (NERSC users read <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/NERSC_INSTALL.html">this</a>). 
 The previous version (version 1.2.1, April 2012) is also available <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/CombBLAS_beta_12_1.tgz"> here </a> for backwards compatibility and benchmarking. 
* 	- To create sample applications
* and run simple tests, all you need to do is to execute the following three commands, in the given order, inside the main directory: 
* 		-  <i> cmake . </i>
* 		- <i> make </i>
* 		- <i> ctest -V </i> (you need the testinputs, see below)
* 	- Test inputs are separately downloadable <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/testdata_combblas1.2.tar.gz"> here</a>. Extract them inside the CombBLAS_vx.x directory with the command "tar -xzvf testdata_combblas1.2.tar.gz"
* - Alternatively (if cmake fails, or you just don't want to install it), you can just imitate the sample makefiles inside the ReleaseTests and Applications 
* directories. Those sample makefiles have the following format: makefile-<i>machine</i>. (example: makefile-neumann) 
* 
* <b>Requirements</b>: You need a recent 
* C++ compiler (gcc version 4.4+, Intel version 11.0+ and compatible), a compliant MPI implementation, and C++11 Standard library (libstdc++ that comes with g++ 
* has them). If not, you can use the boost library and pass the -DCOMBBLAS_BOOST option to the compiler (cmake will automatically do it for you); it will work if you just add boost's path to 
* $INCADD in the makefile. The recommended tarball uses the CMake build system, but only to build the documentation and unit-tests, and to automate installation. The chances are that you're not going to use any of our sample applications "as-is", so you can just modify them or imitate their structure to write your own application by just using the header files. There are very few binary libraries to link to, and no configured header files. Like many high-performance C++ libraries, the Combinatorial BLAS is mostly templated. 
* CombBLAS works successfully with GNU, Intel, and PGI compilers, using OpenMPI, MVAPICH, Cray's MPI (based on MPICH) and Intel MPI libraries.
* 
* <b>Documentation</b>:
* This is a reference implementation of the Combinatorial BLAS Library in C++/MPI.
* It is purposefully designed for distributed memory platforms though it also runs in uniprocessor and shared-memory (such as multicores) platforms. 
* It contains efficient implementations of novel data structures/algorithms
* as well as reimplementations of some previously known data structures/algorithms for convenience. More details can be found in the accompanying paper [1]. One of
* the distinguishing features of the Combinatorial BLAS is its decoupling of parallel logic from the
* sequential parts of the computation, making it possible to implement new formats and plug them in
* without changing the rest of the library.
*
* The implementation supports both formatted and binary I/O. The latter is much faster but no human readable. Formatted I/O uses a tuples format very similar to the Matrix Market.
* We encourage in-memory generators for faster benchmarking. A port to University of Florida Sparse Matrix Collection is under construction. More info on I/O formats are 
* <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/Input_File_Formats.pdf">here</a>
*
* The main data structure is a distributed sparse matrix ( SpParMat <IT,NT,DER> ) which HAS-A sequential sparse matrix ( SpMat <IT,NT> ) that 
* can be implemented in various ways as long as it supports the interface of the base class (currently: SpTuples, SpCCols, SpDCCols).
*
* For example, the standard way to declare a parallel sparse matrix A that uses 32-bit integers for indices, floats for numerical values (nonzeros),
* SpDCCols <int,float> for the underlying sequential matrix operations is: 
* - SpParMat<int, float, SpDCCols<int,float> > A; 
*
* The repetitions of int and float types inside the SpDCCols< > is a direct consequence of the static typing of C++
* and is akin to some STL constructs such as vector<int, SomeAllocator<int> >. If your compiler support "auto", then you can have the compiler infer the type.
*
* Sparse and dense vectors can be distributed either along the diagonal processor or to all processor. The latter is more space efficient and provides 
* much better load balance for SpMSV (sparse matrix-sparse vector multiplication) but the former is simpler and perhaps faster for SpMV 
* (sparse matrix-dense vector multiplication) 
*
* <b> New in version 1.3</b>: 
* - MPI C++ bindings are removed since they are no longer supported by the MPI-3 standard. All MPI calls are C-style. 
* - On-the-fly filtering support [7] illustrated through examples (FilteredBFS.cpp, FilteredMIS.cpp, and TwitterEdge.h). 
* - <a href="http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/FAQ-combblas.html">Frequently asked questions (FAQ)</a> added. 
* - Maximal Independent Set (MIS) application implememented in the language of linear algebra (FilteredMIS.cpp).
*
* The supported operations (a growing list) are:
* - Sparse matrix-matrix multiplication on a semiring SR: PSpGEMM()
* - Elementwise multiplication of sparse matrices (A .* B and A .* not(B) in Matlab): EWiseMult()
* - Unary operations on nonzeros: SpParMat::Apply()
* - Matrix-matrix and matrix-vector scaling (the latter scales each row/column with the same scalar of the vector) 
* - Reductions along row/column: SpParMat::Reduce()
* - Sparse matrix-dense vector multiplication on a semiring, SpMV()
* - Sparse matrix-sparse vector multiplication on a semiring, SpMV()
* - Generalized matrix indexing: SpParMat::operator(const FullyDistVec & ri, const FullyDistVec & ci)
* - Generalized sparse matrix assignment: SpParMat::SpAsgn (const FullyDistVec & ri, const FullyDistVec &ci, SpParMat & B)
* - Numeric type conversion through conversion operators
* - Elementwise operations between sparse and dense matrices: SpParMat::EWiseScale() and operator+=()  
* - BFS specific optimizations inside BFSFriends.h
* 
* All the binary operations can be performed on matrices with different numerical value representations.
* The type-traits mechanism will take care of the automatic type promotion, and automatic MPI data type determination.
*
* Some features it uses:
* - templates (for generic types, and for metaprogramming through "curiously recurring template pattern")
* - operator overloading 
* - compositors (to avoid intermediate copying) 
* - standard library whenever possible
* - Reference counting using shared_ptr for IITO (implemented in terms of) relationships
* - As external code, it utilizes 
*	- sequence heaps of <a href="http://www.mpi-inf.mpg.de/~sanders/programs/"> Peter Sanders </a>.
*	- a modified (memory efficient) version of the Viral Shah's <a href="http://www.allthingshpc.org/Publications/psort/psort.html"> PSort </a>.
*	- a modified version of the R-MAT generator from <a href="http://graph500.org"> Graph 500 reference implementation </a>
* 
* Important Sequential classes:
* - SpTuples		: uses triples format to store matrices, mostly used for input/output and intermediate tasks (such as sorting)
* - SpDCCols		: implements Alg 1B and Alg 2 [2], holds DCSC.

* Important Parallel classes:
* - SpParMat		: distributed memory MPI implementation 
	\n Each processor locally stores its submatrix (block) as a sequential SpDCCols object
	\n Uses a polyalgorithm for SpGEMM: For most systems this boils down to a BSP like Sparse SUMMA [3] algorithm.
* - FullyDistVec	: dense vector distributed to all processors
* - FullyDistSpVec:	: sparse vector distributed to all processors
*
* 
* <b> Applications </b>  implemented using Combinatorial BLAS:
* - BetwCent.cpp : Betweenness centrality computation on directed, unweighted graphs. Download sample input <a href=" http://gauss.cs.ucsb.edu/~aydin/CombBLAS_FILES/scale17_bc_inp.tar.gz"> here </a>.
* - MCL.cpp : An implementation of the MCL graph clustering algorithm.
* - Graph500.cpp: A conformant implementation of the <a href="http://graph500.org">Graph 500 benchmark</a>.
* - FilteredMIS.cpp: Filtered maximal independent set calculation on ER graphs using Luby's algorithm. 
* - FilteredBFS.cpp: Filtered breadth-first search on a twitter-like data set. 
*
* <b> Performance </b> results of the first two applications can be found in the design paper [1]; Graph 500 results are in a recent BFS paper [4]. The most
recent sparse matrix indexing, assignment, and multiplication results can be found in [5]. Performance of filtered graph algorithms (BFS and MIS) are reported in [7]. 
*
* A subset of test programs demonstrating how to use the library (under ReleaseTests):
* - TransposeTest.cpp : File I/O and parallel transpose tests
* - MultTiming.cpp : Parallel SpGEMM tests
* - IndexingTest.cpp: Various sparse matrix indexing usages
* - SpAsgnTiming.cpp: Sparse matrix assignment usage and timing. 
* - FindSparse.cpp : Parallel find/sparse routines akin to Matlab's.
* - GalerkinNew.cpp : Graph contraction or restriction operator (used in Algebraic Multigrid). 
*
* <b> Citation: </b> Please cite the design paper [1] if you end up using the Combinatorial BLAS in your research.
*
* - [1] Aydın Buluç and John R. Gilbert, <i> The Combinatorial BLAS: Design, implementation, and applications </i>. International Journal of High Performance Computing Applications (IJHPCA), 2011. <a href="http://www.cs.ucsb.edu/research/tech_reports/reports/2010-18.pdf"> Preprint </a>, <a href="http://hpc.sagepub.com/content/early/2011/05/11/1094342011403516.abstract">Link</a>
* - [2] Aydın Buluç and John R. Gilbert, <i> On the Representation and Multiplication of Hypersparse Matrices </i>. The 22nd IEEE International Parallel and Distributed Processing Symposium (IPDPS 2008), Miami, FL, April 14-18, 2008
* - [3] Aydın Buluç and John R. Gilbert, <i> Challenges and Advances in Parallel Sparse Matrix-Matrix Multiplication </i>. The 37th International Conference on Parallel Processing (ICPP 2008), Portland, Oregon, USA, 2008
* - [4] Aydın Buluç and Kamesh Madduri, <i> Parallel Breadth-First Search on Distributed-Memory Systems </i>. Supercomputing (SC'11), Seattle, USA. <a href="http://arxiv.org/abs/1104.4518">Extended preprint</a> <a href="http://gauss.cs.ucsb.edu/~aydin/sc11_bfs.pdf"> PDF </a>
* - [5] Aydın Buluç and John R. Gilbert. <i> Parallel Sparse Matrix-Matrix Multiplication and Indexing: Implementation and Experiments </i>. SIAM Journal of Scientific Computing, 2012. <a href="http://arxiv.org/abs/1109.3739"> Preprint </a>
 <a href="http://gauss.cs.ucsb.edu/~aydin/spgemm_sisc12.pdf"> PDF </a>
* - [6] Aydın Buluç. <i> Linear Algebraic Primitives for Computation on Large Graphs </i>. PhD thesis, University of California, Santa Barbara, 2010. <a href="http://gauss.cs.ucsb.edu/~aydin/Buluc_Dissertation.pdf"> PDF </a>
* - [7] Aydın Buluç, Erika Duriakova, Armando Fox, John Gilbert, Shoaib Kamil, Adam Lugowski, Leonid Oliker, Samuel Williams. <i> High-Productivity and High-Performance Analysis of Filtered Semantic Graphs </i> , International Parallel and Distributed Processing Symposium (IPDPS), 2013. <a href="http://gauss.cs.ucsb.edu/~aydin/ipdps13-kdtsejits.pdf"> PDF </a>
*/
