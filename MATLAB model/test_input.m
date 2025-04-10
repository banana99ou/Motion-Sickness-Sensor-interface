%% IMU 신호 생성 - 이동수단의 좌우 6회 회전 시나리오

% 시뮬레이션 시간 설정
dt = 0.01; % 샘플링 간격 (100Hz)
T = 600; % 총 시뮬레이션 시간 (초)
t = (0:dt:T)'; % 시간 벡터 (열 벡터)

% 회전 패턴 설정 (좌-우-좌-우-좌-우) 6회 반복
freq = 2 * pi / (T / 6); % 6번의 좌우 회전 주기
omega_x = 0.1 * sin(freq * t);  % 롤 (좌우 흔들림)
omega_y = 0.05 * cos(freq * t); % 피치 (앞뒤 기울기)
omega_z = 0.5 * sin(freq * t);  % 요 (좌우 회전)

% 가속도 (f) - 코너링 시 차량이 측면으로 받는 힘
f_x = 0.2 * cos(0.2 * t);  % 전/후 가속도 (브레이크 / 가속)
f_y = 0.5 * sin(freq * t);  % 코너링 시 좌/우 힘 (좌우 반복)
f_z = -9.81 + 0.05 * sin(0.3 * t); % 중력 + 노면 충격

% IMU 데이터 저장 (Simulink From Workspace 블록 입력용)
IMU_omega = [t, omega_x, omega_y, omega_z]; % 시간 + 각속도 [rad/s]
IMU_f = [t, f_x, f_y, f_z]; % 시간 + 가속도 [m/s^2]

% Simulink에 전달 (변수 등록)
assignin('base', 'IMU_omega', IMU_omega);
assignin('base', 'IMU_f', IMU_f);

%% 시각 기관 벡터(VV) 신호 생성 - 두 시나리오 중 선택
% 시나리오 선택: 'LAD' 또는 'WAD'
% LAD: 운전자가 정면을 바라보면서 차량의 회전에 따라 머리도 기울어짐 (즉, 시각 벡터가 차량 회전에 영향을 받음)
% WAD: 태블릿 사용으로 시각 정보가 고정되어 있음 (시각 벡터는 거의 90°에 고정됨)
scenario = 'WAD'; % 'LAD' 또는 'WAD'로 스위치

switch scenario
    case 'LAD'
        scaling_factor = 0.2;  % 기울임 정도 (예: 20% 반영)
        yaw_angle = omega_z * (180/pi); 
        theta_vv = 90 + scaling_factor * yaw_angle;
    case 'WAD'
        % WAD 시나리오: 태블릿 사용 -> 시각 기관 각도 고정 (90°)
        theta_vv = 90 * ones(size(t));
    otherwise
        error('시나리오 설정이 올바르지 않습니다.');
end

% 논문 공식 적용: VV 벡터 계산
VV_x = 9.81 * cos(theta_vv * pi / 180);  % X축 성분
VV_y = 9.81 * sin(theta_vv * pi / 180);  % Y축 성분
VV_z = zeros(size(t));                          % Z축 성분 (항상 0)

% From Workspace 블록에서 사용할 데이터 형식으로 변환
INPUT_VV = [t, VV_x, VV_y, VV_z]; 

% Simulink에 전달 (변수 등록)
assignin('base', 'VV_data', INPUT_VV);


%% 두 신호 애니메이션 (입력 VV와 내부모델 예측 OUTPUT_VV)

time_vec = VV_data(:,1);  % VV_data의 첫 열은 시간
OUTPUT_VV = out.OUTPUT_VV;
figure;

% Subplot 1: VV_data (입력 시각 벡터) 애니메이션
subplot(2,1,1);
hold on; grid on; axis equal;
xlim([-10, 10]); ylim([-10, 10]);
xlabel('X-axis (VV_x)');
ylabel('Y-axis (VV_y)');
title('시각 기관 벡터 VV');

% 초기 벡터 설정 (입력 VV)
quiver_in = quiver(0, 0, VV_data(1,2), VV_data(1,3), 'g', 'LineWidth', 2, 'MaxHeadSize', 1);
plot([-10, 10], [0, 0], 'k', 'LineWidth', 1.5); % X축
plot([0, 0], [-10, 10], 'k', 'LineWidth', 1.5); % Y축

% Subplot 2: OUTPUT_VV (내부모델 예측 시각 벡터) 애니메이션
subplot(2,1,2);
hold on; grid on; axis equal;
xlim([-10, 10]); ylim([-10, 10]);
xlabel('X-axis (OUTPUT\_VV_x)');
ylabel('Y-axis (OUTPUT\_VV_y)');
title('내부모델 예측 시각 벡터 OUTPUT\_VV');

% 초기 벡터 설정 (OUTPUT_VV)
quiver_out = quiver(0, 0, OUTPUT_VV(1,1), OUTPUT_VV(1,2), 'b', 'LineWidth', 2, 'MaxHeadSize', 1);
plot([-10, 10], [0, 0], 'k', 'LineWidth', 1.5); % X축
plot([0, 0], [-10, 10], 'k', 'LineWidth', 1.5); % Y축

%% 애니메이션 업데이트
for k = 1:50:length(time_vec)
    % 입력 VV 업데이트 (VV_data의 열 2,3: X, Y)
    subplot(2,1,1);
    quiver_in.UData = VV_data(k,2);
    quiver_in.VData = VV_data(k,3);
    
    % 내부모델 예측 VV 업데이트 (OUTPUT_VV의 열 1,2: X, Y)
    subplot(2,1,2);
    quiver_out.UData = 2*OUTPUT_VV(k,1);
    quiver_out.VData = OUTPUT_VV(k,2);
    
    drawnow;
    pause(0.01);  % 애니메이션 속도 조절
end

disp("✅ VV 및 OUTPUT_VV 애니메이션 완료");