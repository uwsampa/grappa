%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% HPC Scalable Graph Analysis Benchmark:  GRAPH ANALYSIS - MAIN PROGRAM 
%
% Main Program for the 2009 HPC Scalable Graph Analysis Benchmark.
% Note that this is only one of many possible 'correct' interpretations 
% of its Written Specification.
%
% For its complete list of files and instructions on how to run 
% this program please refer to the accompanying README.txt.
% 
% OBJECTIVE
% An Executable Specification is intended to provide a one-of-many-possible
% proof-of-concept implementation of its Written Specification. Its 
% primary purpose is to demonstrate that its Written Specification can be
% implemented with all of its desired properties.  In addition, the 
% Executable Specification is a tool for Implementers to use in their 
% own verification process. 
%
% This program generates scalable graph data and operates on this data
% through four different kernels, whose operation is timed.  It defines 
% global control and user configurable parameters, invokes the data 
% generator, invokes and times each of the four kernels and their  
% respective verification code (which perform minor code checks and 
% display relevant results).
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
% OPTIONS
% The file getUserParameters.m defines the user configurable options.
% 
% REVISION 
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

declareGlobals;


%--------------------------------------------------------------------------
% Preamble.
%--------------------------------------------------------------------------

% Start overall run time for RUN_graphAnalysis.m (not performance timing).
OverallStartTime = clock;  

format long g % set the terminal dump (float or fixed) to print 15 digits. 
fprintf('\nScalable Graph Analysis Benchmark Executable Specification Release 1.0,');
fprintf('\nbased on Written Spec. v1.0:  Running...\n\n');

% User Interface: Configurable parameters and global program control.
getUserParameters;


%--------------------------------------------------------------------------
% Scalable Data Generator.
%--------------------------------------------------------------------------

fprintf('\nScalable Data Generator - genScalData() beginning execution...\n');

startTime = clock;                % Start performance timing.

% Generate the edge set.
[E V] = genScalData(SCALE, SUBGR_PATH_LENGTH, K4approx, batchSize);

fprintf('\n\tgenScalData() completed execution.\n');

dispElapsedTime(startTime);       % End performance timing.


% Display results outside of performance timing.
if ENABLE_VERIF
  
    % Display results.
	verifGenScalData(SCALE, E, V);

end % of ENABLE_VERIF


%--------------------------------------------------------------------------
% Kernel 1 - Graph Construction.
%--------------------------------------------------------------------------

fprintf('\nKernel 1 - computeGraph() beginning execution...\n');

startTime = clock;                     % Start performance timing.

% From only the input edge set, construct the graph 'G'.
[G E] = computeGraph(SCALE, E);

fprintf('\n\tcomputeGraph() completed execution.\n');

dispElapsedTime(startTime);            % End performance timing. 

% Display results outside of performance timing.
if ENABLE_VERIF

    [G V] = verifComputeGraph(E, G, V);

end % of ENABLE_VERIF


%--------------------------------------------------------------------------
% Kernel 2 - Search Large Sets.
%--------------------------------------------------------------------------

if ENABLE_K2

    fprintf('\nKernel 2 - searchWeights() beginning execution...\n');
    startTime = clock;                  % Start performance timing.

    % Search the graph to find the vertex pair(s) with the maximum weight.
    [maxGraphWeight startSet] = searchWeights(G);

    fprintf('\n\tsearchWeights() completed execution.\n');
    
    dispElapsedTime(startTime);        % End performance timing.

    
    % Display results outside of performance timing.
    if ENABLE_VERIF

        verifSearchWeights(maxGraphWeight, startSet', E, V, G);

    end % of ENABLE_VERIF

end % of ENABLE_K2


%--------------------------------------------------------------------------
% Kernel 3 - Graph Extraction.
%--------------------------------------------------------------------------

if ENABLE_K3
    
    fprintf('\nKernel 3 - findSubgraphs() beginning execution...\n');
    
    startTime = clock;                  % Start performance timing.
    
    % Find subgraphs of a specified size given starting vertex pairs. 
    subGraphs = findSubgraphs(G, SUBGR_PATH_LENGTH, startSet);

    fprintf('\n\tfindSubgraphs() completed execution.\n');
    
    dispElapsedTime(startTime);        % End performance timing.
    
    
    % Display results outside of performance timing.
    if ENABLE_VERIF

        verifFindSubgraphs(G, subGraphs, startSet, SUBGR_PATH_LENGTH);

    end % of ENABLE_VERIF
    
end % of ENABLE_K3


%--------------------------------------------------------------------------
% Kernel 4 - Graph Clustering.
%--------------------------------------------------------------------------

if ENABLE_K4
    
    fprintf('\nKernel 4 - betwCentrality() beginning execution...\n');
    
    startTime = clock; % Start performance timing.
    
    tic;
     
    % Analyze the graph's connectivity, and generate the vertices' 
    % betweenness centrality metrics.
    BC = betwCentrality(G, K4approx, batchSize);
    
    timeK4 = toc;
    
    fprintf('\n\tbetwCentrality() completed execution.\n');
    
    dispElapsedTime(startTime); % End performance timing.

    % Display results outside of performance timing.
    if ENABLE_VERIF

        verifBetwCentrality(SCALE, timeK4, K4approx, BC, G, V);

    end % of ENABLE_VERIF

end % of ENABLE_K4


%--------------------------------------------------------------------------
% Postamble.
%--------------------------------------------------------------------------


fprintf('\nRUN_graphAnalysis() completed execution.\n');

dispElapsedTime(OverallStartTime); % End timing (not performance timing).

fprintf('\n\t[Timing of the main program, RUN_graphAnalysis(), is not required].\n\n');


fprintf('\nScalable Graph Analysis Benchmark Executable Specification Release 1.0,');
fprintf('\nbased on Written Spec. v1.0:  End of test.\n\n');




%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Copyright © 2009, Massachusetts Institute of Technology
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
