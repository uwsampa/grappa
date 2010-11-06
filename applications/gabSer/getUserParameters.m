%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% script getUserParameters.m 
%
% Parameters configured by the user of the Scalable Graph Benchmark
% Executable Specification.
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

declareGlobals; 


%--------------------------------------------------------------------------
%  Scalable Data Generator parameters.
%--------------------------------------------------------------------------

% Exponent of 2 of the number of max vertices; indirectly it also 
% scales the max number of edges, and the max edge weight.  NOTE: When 
% performance testing, SCALE should be set to values of 6 or higher. (At 
% values lower than 6, the SDG adjusts the num edges to ensure that the 
% resulting graph's sparsity is below 0.25). Integer '3' or greater.
SCALE = 10;

ENABLE_RANDOM_VERTICES = 0; 
                        % boolean; enable permutation of vertex nums.

ENABLE_DATA_TYPE = RMAT;     
                        % The scalable data can either be one of either:
                        % RMAT       - Random RMAT data 
                        % HALF_TORUS_1D  - A singly connected 1-D (half) 
                        %              torus with N vertices and M = N 
                        %              edges with verifiable BC.
                        % TORUS_1D   - Generates a doubly connected 1D
                        %              (full) torus with verifiable BC.
                        % TORUS_2D   - Generates a doubly connected 2D
                        %              (full) torus with verifiable BC.
                        % TORUS_3D   - Generates a doubly connected 3D
                        %              (full) torus. 
                        % TORUS_4D   - Generates a doubly connected 4D
                        %              (full) torus.  
                        
%--------------------------------------------------------------------------
% Kernel parameters.
%--------------------------------------------------------------------------

SUBGR_PATH_LENGTH = 3;  % Kernel 3: Max path length (in edges, from a 
                        % specified source vertex). Integer '1' or greater.

K4approx = SCALE;       % Kernel 4: Determines the number of iterations to
                        % use in the betweenness centrality computation.
                        % Integer between '10' and 'SCALE'. 

batchSize = 16;         % Kernel 4: Number of vertices to process at once 
                        % (vectorized processing). The space required by 
                        % the algorithm increases linearly in this 
                        % parameter.  While there is no theoretical 
                        % decrease in runtime by increasing this parameter, 
                        % in actual implementations performance may 
                        % increase by say 10 due to batch processing of the 
                        % operations. Integer between '1' and '2^K4approx',
                        % or processing all the vertices at the same time.
                       
%--------------------------------------------------------------------------
% Global program control parameters.
%--------------------------------------------------------------------------

% Enables for subsections of the benchmark.
ENABLE_K2 = 0;          % boolean; enables Kernel 2: searchWeights().   
ENABLE_K3 = 0;          % boolean; enables Kernel 3: findSubgraphs().
ENABLE_K4 = 1;          % boolean; enables Kernel 4: betwCentrality().


%--------------------------------------------------------------------------
% Global verification program control parameters.
%--------------------------------------------------------------------------

% WARNING: THESE MODES BELOW ARE _NOT_ SCALABLE HENCE SHOULD BE SET TO  
% '0' WHEN TESTING WITH LARGE SCALE DATA OR WHEN DOING TIMED PERFORMANCE 
% SCALABILITY STUDIES.

ENABLE_VERIF       = 0; % boolean; displays built-in code verification; 
                        % superceeds all enables below.                        
ENABLE_MDUMP       = 0; % boolean; displays dump of matrices onto the cmd
                        % window. Recommended only for small scale 
                        % problems. 
ENABLE_DEBUG       = 1; % boolean; reproduce random results when debugging.

ENABLE_PLOTS       = 1; % boolean; dissables all benchmark plots.

ENABLE_PLOT_SDG    = 1; % boolean; enables SDG adjacency matrix plot.
ENABLE_PLOT_K1     = 1; % boolean; enables Kernel 1 adjacency matrix plot.
ENABLE_PLOT_K3     = 0; % boolean; enables Kernel 3 subgraphs plots.
ENABLE_PLOT_K4     = 1; % boolean; enables Kernel 4 out degree plot.

ENABLE_K4_TEST3    = 0; % boolean; this may cause the code to break when 
                        % dialed to larger graphs, depending on the ability
                        % of the target system.




%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% WARNING: Code below this point is not to be modified by the user.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% Safeguard. Kernel 3 requires the start set that is output from Kernel 2.
if ENABLE_K3
    ENABLE_K2 = 1;
end


% Reproduce the results when debugging.
if ENABLE_DEBUG
    rand('state', 1);  
end 


if SCALE <= 2  
    SCALE = 3; % Dissallow SCALE of 2 or under.
end


% Set the torus's dimmensionality.
if (ENABLE_DATA_TYPE == HALF_TORUS_1D) || (ENABLE_DATA_TYPE == TORUS_1D)                       
    TORUS_DIM = 1;      
end
if (ENABLE_DATA_TYPE == TORUS_2D)                        
    TORUS_DIM = 2;      
end
if (ENABLE_DATA_TYPE == TORUS_3D)                       
    TORUS_DIM = 3;      
end
if (ENABLE_DATA_TYPE == TORUS_4D)                        
    TORUS_DIM = 4;    
end


%--------------------------------------------------------------------------
% Miscellanous program initializations. 
% WARNING: This code is not intended to be modified by the user.
%--------------------------------------------------------------------------

FIGNUM = 0; % Initialize global figure number count.
            
% The statement below checks to see if you are attempting to run this  
% benchmark under Octave; else it assumes that you are running under Matlab
try 
    OCTAVE_VERSION; 
    OCTAVE = 1; 
catch 
    OCTAVE = 0;
end

if (OCTAVE == 1)

    % Because of incompatibilities, figures are disabled under Octave.
    ENABLE_PLOTS       = 0;
    ENABLE_PLOT_SDG    = 0; 
    ENABLE_PLOT_K1     = 0; 
    ENABLE_PLOT_K3     = 0;
    ENABLE_PLOT_K4     = 0; 

    %page_output_immediately = 1; not longer needed for Octave 2.9.9+
    page_scroll_output = 0;
    fprintf('\trunning on Octave %s.\n\n', OCTAVE_VERSION);

else
    
    % Note that this software has only been tested under Matlab and Octave.
    fprintf('\trunning on Matlab %s.\n\n', version);

end




%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Copyright � 2009, Massachusetts Institute of Technology
% All rights reserved.
% 
% Redistribution and use in source and binary forms, with or without
% modification, are permitted provided that the following conditions are  
% met:
%    * Redistributions of source code must retain the above copyright
%      notice, this list of conditions and the following disclaimer.
%    * Redistributions in binary form must reproduce the above copyright
%      notice, this list of conditions and the following disclaimer in the
%      documentation and/or other materials provided with the distribution.
%    * Neither the name of the Massachusetts Institute of Technology nor  
%      the names of its contributors may be used to endorse or promote 
%      products derived from this software without specific prior written 
%      permission.
%
% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
% IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
% THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
% PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
% CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
% EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
% PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
% PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
% LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
% NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   
% SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
