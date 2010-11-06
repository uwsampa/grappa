% ## Copyright (C) 2005 John W. Eaton
% ##
% ## This file is part of Octave.
% ##
% ## Octave is free software; you can redistribute it and/or modify it
% ## under the terms of the GNU General Public License as published by
% ## the Free Software Foundation; either version 2, or (at your option)
% ## any later version.
% ##
% ## Octave is distributed in the hope that it will be useful, but
% ## WITHOUT ANY WARRANTY; without even the implied warranty of
% ## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
% ## General Public License for more details.
% ##
% ## You should have received a copy of the GNU General Public License
% ## along with Octave; see the file COPYING.  If not, write to the Free
% ## Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
% ## 02110-1301, USA.

% ## -*- texinfo -*-
% ## @deftypefn {Function File} {} unidrnd (@var{mx});
% ## @deftypefnx {Function File} {} unidrnd (@var{mx}, @var{v});
% ## @deftypefnx {Function File} {} unidrnd (@var{mx}, @var{m}, @var{n}, @dots{});
% ## Return random values from discrete uniform distribution, with maximum
% ## value(s) given by the integer @var{mx}, which may be a scalar or
% ## multidimensional array.
% ##
% ## If @var{mx} is a scalar, the size of the result is specified by
% ## the vector @var{v}, or by the optional arguments @var{m}, @var{n},
% ## @dots{}.  Otherwise, the size of the result is the same as the size
% ## of @var{mx}.
% ## @end deftypefn

% ## Author: jwe

function retval = unidrnd (n, varargin)
if (nargin == 1)
    dims = size (n);
elseif (nargin == 2)
    if (rows (varargin{1}) == 1 && columns (varargin{1}) > 1)
        dims = varargin{1};
    else
        error ('unidrnd: invalid dimension vector');
    end
elseif (nargin > 2)
    for i = 1:nargin-1
        if (~isscalar (varargin{i}))
            error ('unidrnd: expecting scalar dimensions');
        end
    end
    dims = [varargin{:}];
else
    error ('unidrnd (n, ...)');
end
if (isscalar (n) ...
        || (length (size (n)) == length (dims) && all (size (n) == dims)))
    retval = ceil (rand (dims) .* n);
else
    error ('unidrnd: dimension mismatch');
end

