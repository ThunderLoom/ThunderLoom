function [X, Y] = gen( b, num_steps )
%GEN Summary of this function goes here
%   Detailed explanation goes here
cmd = sprintf('./plot_von_mises %f %d',b,num_steps);
[status, val] = system(cmd);

A = sscanf(val, '%f,%f');

X = zeros(length(A)/2 ,1);
Y = zeros(length(A)/2 ,1);

for i = 1:(length(A)/2)
    X(i) = A(2*i-1);
    Y(i) = A(2*i);
end

end

