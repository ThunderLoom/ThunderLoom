octaves = 4;
scale = 4;
size = 1000;
persistance = 0.7;

cmd = sprintf('./pnoise %d %f %f %f', size, octaves, scale, persistance)
system(cmd);

fileID = fopen('out.txt','r');
A = fscanf(fileID,'%f');

%img = zeros(size,size);
%for i = 1:size
%    img(i,1:size) = A( ((i-1)*size+1):i*size );
%end

%img = zeros(size,1);
%img(1:size) = A( ((i-1)*size+1):i*size );

%image((img+1)*20);
%colormap(gray);


plot(1:length(A),A);

%% 1D