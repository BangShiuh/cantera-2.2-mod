function zz = z(d, n)
% Z  Get the grid points.
% zz = z(d, n)
% This function is deprecated in favor of the function :mat:func:`gridPoints`,
% and will be removed after Cantera 2.3.
%
% :param d:
%     Instance of class :mat:func:`Domain1D`
% :param n:
%     Optional. Indices of grid points to get.
%     Defaults to getting all of the grid points.
% :return:
%     Vector of grid points.
%

warning('This function is deprecated, and will be removed after Cantera 2.3. Use gridPoints instead.');
if nargin == 1
    zz = zeros(1, nPoints(d));
    for i = 1:nPoints(d)
        zz(i) = domain_methods(d.dom_id, 19, i);
    end
else
    m = length(n);
    zz = zeros(1, m);
    for i = 1:m
        zz(i) = domain_methods(d.dom_id, 19, n(i));
    end
end
