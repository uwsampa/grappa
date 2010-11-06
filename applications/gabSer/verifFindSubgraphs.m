function verifFindSubgraphs(G, subGraphs, startSet, pathLength)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Function verifFindSubgraphs() - verify findSubgraphs().
%
% First level verification of the functionality the code in Kernel 3 on
% Graph Extraction.
%
% Prints results to the Matlab console and plots graphs.
%
% For a detailed description of the scalable graph analysis algorithm, 
% please see Scalable Graph Benchmark Written Specification, V1.0.
%
%
% INPUT
%
% G                 - [struct] graph (from Kernel 1).
%   adjMatrix       - [?x?] sparse weighted adjacency matrix of the graph.
%
% subGraphs         - [Array of G structs] of subgraphs, each of data type 
%                     'G' (from Kernel 3).
%
% startSet          - [2x? int array] one or more vertex pairs with
%                     which to start the subgraph(s) (from Kernel 3).
%
% pathLength        - [int] desired edge length of the subgraph (user).
%
%
% REVISION
% 22-Feb-09   1.0 Release   MIT Lincoln Laboratory.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 

declareGlobals;


fprintf('\n\tKernel 3 Graph Extraction - Results:\n');

if ENABLE_PLOTS && ENABLE_PLOT_K3
    fprintf('\n\t(See Figures: "K3 - SubGraph ... ".)\n\n');

else
    fprintf('\n\t(Kernel 3 plots dissabled).\n\n');
end % of ENABLE_PLOTS && ENABLE_PLOT_K3


%--------------------------------------------------------------------------
% Verify subgraphs.
%--------------------------------------------------------------------------

% Terminal dump and plot figure of each of the subgraphs' adjacenty matrix.
for i_subG = 1:size(subGraphs,2) 
    
    if ENABLE_MDUMP
        fprintf('\n\tAdjacency Matrix for Subgraph %.0f:', ...
            i_subG);

        % For terminal dump, convert from a sparse to a full matrix.
        subG_adjacencyMatrix = subGraphs{i_subG} + 0
        fprintf('\n\n');
    else
        subG_adjacencyMatrix = subGraphs{i_subG};
    end % of ENABLE_MDUMP

    if ENABLE_PLOTS && ENABLE_PLOT_K3

        graphTitle = sprintf( ...
            ['K3 - SubGraph %d, start set (%d,%d), ' ...
            'and length = %d.'], i_subG, ...
            startSet(i_subG), startSet(i_subG), pathLength);

        hold('on');
        fh = placeFigure();
        spy(subG_adjacencyMatrix);
        H = title(graphTitle);
        ylabel('Vertex Number');
        % axis xy -- dissabled to correspond with console's (ENABLE_MDUMP).
        hold('off');

    end % of ENABLE_PLOTS && ENABLE_PLOT_K3

end
 

% NOTE: if SUBGR_PATH_LENGTH is size of the graph, you should be able to  
%     retrieve the entire graph.  But this will
%     not be true if its not entirely connected.
%     in debug mode, control size of problem to be able to display
%
% NOTE: when SUBGR_PATH_LENGTH is large enough, AND, ENABLE_RANDOM_VERTICES
% is 0, plot will be the same as K1's.




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
