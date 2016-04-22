n = 80;

close all
figure

[X,Y] = gen(10,n);
hp = polar(X,Y,':');
set(hp,'linewidth',2);

hold on

[X,Y] = gen(5,n);
hp = polar(X,Y,'-.');
set(hp,'linewidth',2);

[X,Y] = gen(0.1,n);
hp = polar(X,Y,'-');
set(hp,'linewidth',2);

legend('b = 10','b = 5','b = 0.1');

%axis('equal');
%axis([-1 1 -1 1])


%% Old timey plot
title({'Von meisis distribution with changing paramter $b$'},'interpreter','latex','FontSize',12,'FontName','Times');
%xlabel({'Temperatur  $[^\circ C]$'},'interpreter','latex','FontSize',12,'FontName','Times');
%ylabel({'V\"armefl\"ode $[mW]$'},'interpreter','latex','FontSize',12,'FontName','Times');
%set(gca,'Units','normalized','YTick',0:0.2:0.8,'FontUnits','points','FontWeight','normal','FontSize',12,'FontName','Times');
set(gcf, 'PaperPosition', [.25 .25 4 3]);
set(gcf, 'PaperPositionMode', 'manual');
set(gcf, 'PaperUnits', 'inches');
set(gcf, 'PaperPosition', [0 0 4 4]);
set(gcf,'color','w');
print -deps ludvig_von_mises.eps