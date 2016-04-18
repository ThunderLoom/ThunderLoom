clc;clf;
filename = 'out.txt';
system('plot_halton');
M = csvread(filename);
figure(1); clf;
plot(M(:,2), M(:,3), 'o');