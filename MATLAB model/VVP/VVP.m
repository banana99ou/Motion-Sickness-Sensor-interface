%% 논문 알고리즘1 - 현재는 동영상 후처리 방식 (실시간 X)

% 동영상 파일 읽기
videoFile = 'sample_video1.mp4'; % 입력 동영상 파일
videoReader = VideoReader(videoFile);

% 초기 θ_vvp 설정
theta_vvp_t_minus1 = 90; % 초기 각도 설정

% 해상도 축소 비율
resizeFactor = 0.1; 

% Figure 생성
figure('Name', 'VVP 방향 및 히스토그램 시각화', 'NumberTitle', 'off');

% 프레임 처리
while hasFrame(videoReader)
    % 프레임 읽기
    A = readFrame(videoReader);
    
    % 해상도 줄이기
    A_resized = imresize(A, resizeFactor); % 해상도 축소
    
    %% 논문 알고리즘1 순서대로 구현

    % 1. 그레이스케일 변환
    I_gray = rgb2gray(A_resized);
    I_gray = double(I_gray); 

    % 2. 정규화
    I_gray_normalized = (I_gray - min(I_gray(:))) / (max(I_gray(:)) - min(I_gray(:))); % [0, 1] 정규화

    % 3. Sobel 필터 적용
    [Vx_t, Vy_t] = imgradientxy(I_gray_normalized, 'sobel'); % 논문의 ∇x_t, ∇y_t 계산

    % 4. 그래디언트 크기 M_t 계산
    M_t = sqrt(Vx_t.^2 + Vy_t.^2); % M_t = sqrt(∇x_t^2 + ∇y_t^2)

    % 5. 각도 Θ_t 계산
    Theta_t = atan2d(Vy_t, Vx_t); % Θ_t = atan2d(∇y_t, ∇x_t)
    Theta_t(Theta_t < 0) = Theta_t(Theta_t < 0) + 360; % [0, 360)로 변환

    % 6~11. 각도 정규화
    Theta_t_norm = Theta_t;
    Theta_t_norm(Theta_t >= 180 & Theta_t < 360) = Theta_t(Theta_t >= 180 & Theta_t < 360) - 180;
    Theta_t_norm(Theta_t == 360) = 0;

    % 12. 그래디언트 크기 정규화
    M_t = (M_t - min(M_t(:))) / (max(M_t(:)) - min(M_t(:))); % [0, 1]로 정규화

    % 13~15. 히스토그램 생성
    num_bins = 180; % 각도 범위: [0, 179]
    angle_histogram = zeros(1, num_bins);
    for d = 0:num_bins-1
        angle_histogram(d+1) = sum(sum((Theta_t_norm >= d & Theta_t_norm < d+1) .* M_t));
    end

    % 16~17. 상위 3개 값 정렬
    trim_start = 35; trim_end = 150; % 트림 범위
    [sorted_histogram, sort_indices] = sort(angle_histogram(trim_start:trim_end), 'descend'); 
    sorted_angles = trim_start:trim_end; 
    sorted_angles = sorted_angles(sort_indices); 
    c_best3 = sorted_histogram(1:3); % 상위 3개 히스토그램 값
    theta_best3 = sorted_angles(1:3); % 상위 3개 각도 값

    % 18~19. 가중 평균 계산
    theta_vvp = sum(c_best3 .* theta_best3) / sum(c_best3); % 가중 평균

    % 20~21. 안정화
    if abs(theta_vvp - theta_vvp_t_minus1) <= 4
        theta_vvp = 0.7 * theta_vvp + 0.3 * theta_vvp_t_minus1;
    else
        theta_vvp = 0.2 * theta_vvp + 0.8 * theta_vvp_t_minus1;
    end
    theta_vvp_t_minus1 = theta_vvp;

    % 22. 최종 VVP 벡터 계산
    vvp_x = -cosd(theta_vvp); % x 성분
    vvp_y = sind(theta_vvp); % y 성분

    %% 시각화
    subplot(1, 3, 1); % 원본 영상
    imshow(A_resized);
    title('원본 동영상');
    hold on;
    x_center = size(A_resized, 2) / 2;
    y_center = size(A_resized, 1) / 2;
    quiver(x_center, y_center, vvp_x * 50, -vvp_y * 50, 0, 'g', 'LineWidth', 2);
    hold off;

    subplot(1, 3, 2); % 외곽선
    imshow(M_t);
    title('엣지 검출');
    hold on;
    quiver(x_center, y_center, vvp_x * 50, -vvp_y * 50, 0, 'g', 'LineWidth', 2);
    hold off;

    subplot(1, 3, 3); % 히스토그램
    bar(0:179, angle_histogram, 'FaceColor', [0.85 0.33 0.10], 'EdgeColor', 'none');
    title('검출 각도 히스토그램');
    xlabel('Angle (degrees)');
    ylabel('Weighted Frequency');
    xlim([0 180]);
    ylim([0 max(angle_histogram) * 1.2]);
    grid on;
    hold on;
    % 최종 각도 강조
    plot([theta_vvp, theta_vvp], [0, max(angle_histogram) * 1.2], 'g', 'LineWidth', 2);
    % 트림된 범위 강조
    plot([trim_start, trim_start], [0, max(angle_histogram) * 1.2], 'b--', 'LineWidth', 1.5);
    plot([trim_end, trim_end], [0, max(angle_histogram) * 1.2], 'b--', 'LineWidth', 1.5);
    hold off;

    % 잠시 멈추기
    pause(1 / videoReader.FrameRate);
end

disp('동영상 처리가 완료되었습니다.');
