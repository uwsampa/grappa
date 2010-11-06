function [G, V] = verifComputeGraph( E, G, V )

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Function verifComputeGraph() - verify computeGraph().
%
% First level checks on the functionality of the code in Kernel 1,
% Graph Construction. It compares the output of the genScalData()'s 
% known data struct of edges, 'E', to the output of computeGraph()'s 
% reconstructed graph, 'G' data structure.
%
% It prints results to the Matlab console and plots graphs.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
%
% INPUT
%
% E.               - [struct] list of weighted edge tuples (from SDG):
%   StartVertex    - [1xM int array] start vertices,
%   EndVertex      - [1xM int array] end vertices,
%   Weight         - [1xM int array] integer weights,
%                    (where M is total number of edges in the graph).
%
% G.               - [struct] graph (from kernel 1):
%   adjMatrix      - [?x?] sparse weighted adjacency matrix of the graph.

% V.               - [struct] with misc info for verif. (SDG and kernel 1):
%                    (from SDG and kernel 1):
%   lgN            - [int] rounded down log of number of vertices in set, 
%                    (only for TORUS data).
%   N              - [int] total number of vertices in the tuple set.
%   M              - [int] total number of edges in the tuple set.
%   C              - [int] maximum value of any of the weights.
%
%
% OUTPUT
%
% G.               - [struct] graph (from kernel 1) (see above).
%
% V.               - [struct] with misc info for verif. (SDG and kernel 1):
%                    (from SDG and kernel 1) (see above).
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 

declareGlobals;


fprintf('\n\tKernel 1 Graph Construction - Results:\n');

if ENABLE_MDUMP

    fprintf('\n\tThe edge tuple set (with no multiple edges) is: \n\n');
    for ii = 1:length(E.StartVertex)
        fprintf('\t\t%4d. (%d,%d) =\t %d\n', ...
            ii, E.StartVertex(ii), E.EndVertex(ii), E.Weight(ii));
    end
    fprintf('\n');

    fprintf('\tResulting adjacency matrix of the resulting graph is:\n');
    G.adjMatrix + 0           % Display the full adjacency matrix.
    
end % of if ENABLE_MDUMP


if ENABLE_PLOTS && ENABLE_PLOT_K1

    if ENABLE_RANDOM_VERTICES

        % Convert the newly generated graph into its corresponding edge 
        % set, to 'de-randomize' its vertices.
        [startV endV] = find(G.adjMatrix);

        adjacency = sparse(V.invRandVert(startV), ...
            V.invRandVert(endV), 1);

        graphTitle = ...
            sprintf(...
            ['K1 - Unpermutted Sparse Adjacency Matrix Rep. of the Graph\n' ...
            '(it should be identical to SDGs matrix)']);
    else
        adjacency = G.adjMatrix;

        graphTitle = sprintf(...
            ['K1 - Sparse Adjacency Matrix Rep. of the Graph\n' ...
            '(should be identical to the SDGs adj. matrix)']);
    end

    hold('on');
    fh = placeFigure();
    spy(adjacency);
    H = title(graphTitle);
    if ispc
        set(H, 'FontSize', 7);
    end
    ylabel('Vertex Number');
    % axis xy -- dissabled to correspond with console's (ENABLE_MDUMP).
    hold('off');
    
    fprintf('\n\t(See Figure: "K1 - Sparse Adjacency Matrix Rep. of the Graph".)\n\n');

else
    fprintf('\n\t(Kernel 1 plots dissabled).\n\n');
end % of ENABLE_PLOTS && ENABLE_PLOT_K1




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

