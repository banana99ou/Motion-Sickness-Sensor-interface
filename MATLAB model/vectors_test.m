% ------------------------------------------
% Figure 1: out.f, out.w, out.v_s, out.a_s 애니메이션
% ------------------------------------------
figure(1); clf;
hold on; grid on;
xlabel('X'); ylabel('Y'); zlabel('Z');
axis equal;
axis([-10 10 -10 10 -10 10]);  % 데이터 범위에 맞게 조정하세요.
title('3D Vector Animation: out.f, out.w, out.v\_s, out.a\_s');
rotate3d on;

% 초기 화살표 플롯 (원점에서 시작)
h_f   = quiver3(0, 0, 0, out.f(1,1),   out.f(1,2),   out.f(1,3),   0, 'r', 'LineWidth', 2);
h_w   = quiver3(0, 0, 0, out.w(1,1),   out.w(1,2),   out.w(1,3),   0, 'b', 'LineWidth', 2);
h_vs  = quiver3(0, 0, 0, out.v_s(1,1), out.v_s(1,2), out.v_s(1,3), 0, 'g', 'LineWidth', 2);
h_as  = quiver3(0, 0, 0, out.a_s(1,1), out.a_s(1,2), out.a_s(1,3), 0, 'm', 'LineWidth', 2);
legend('f', 'w', 'v\_s', 'a\_s');

% ------------------------------------------
% Figure 2: out.g, out.a 애니메이션 (새 Figure)
% ------------------------------------------
figure(2); clf;
hold on; grid on;
xlabel('X'); ylabel('Y'); zlabel('Z');
axis equal;
axis([-10 10 -10 10 -10 10]);  % 데이터 범위에 맞게 조정하세요.
title('3D Vector Animation: out.g, out.a');
rotate3d on;

% 초기 화살표 플롯 (원점에서 시작)
h_g = quiver3(0, 0, 0, out.g(1,1), out.g(1,2), out.g(1,3), 0, 'c', 'LineWidth', 2);
h_a = quiver3(0, 0, 0, out.a(1,1), out.a(1,2), out.a(1,3), 0, 'k', 'LineWidth', 2);
legend('g', 'a');

% ------------------------------------------
% 두 Figure를 동시에 업데이트하는 애니메이션 루프
% ------------------------------------------
% (각 변수의 행 수가 다를 수 있으므로 최소 행 수로 설정)
N1 = size(out.f, 1);
N2 = size(out.g, 1);
N = min(N1, N2);

for i = 1:N
    % Figure 1 업데이트
    set(h_f,  'UData', out.f(i,1),   'VData', out.f(i,2),   'WData', out.f(i,3));
    set(h_w,  'UData', out.w(i,1),   'VData', out.w(i,2),   'WData', out.w(i,3));
    set(h_vs, 'UData', out.v_s(i,1), 'VData', out.v_s(i,2), 'WData', out.v_s(i,3));
    set(h_as, 'UData', out.a_s(i,1), 'VData', out.a_s(i,2), 'WData', out.a_s(i,3));
    
    % Figure 2 업데이트
    set(h_g, 'UData', out.g(i,1), 'VData', out.g(i,2), 'WData', out.g(i,3));
    set(h_a, 'UData', out.a(i,1), 'VData', out.a(i,2), 'WData', out.a(i,3));
    
    drawnow;      % 업데이트 반영
    pause(0.05);  % 애니메이션 속도 조절 (필요시 조절)
end
