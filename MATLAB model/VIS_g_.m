function [sys, x0, str, ts] = VIS_g_(t, x, u, flag)
    % VIS_g_ 블록 (Visual Perception Internal Model)
    % 입력:
    %   - g_hat [3x1] : 내부 모델에서 예측한 중력 벡터
    % 출력:
    %   - vv_s_hat [3x1] : 내부 모델에서 예측한 시각적 수직 벡터
    %
    % 이 블록은 g_hat에 대해 X-Y 평면으로 투영한 후, y 성분을
    % 9.81로 강제 정규화하여 시각 신호에서 수직(중력) 성분을 일정하게 유지한다.
    
    switch flag
        case 0  % 초기화
            sizes = simsizes;
            sizes.NumContStates  = 0;
            sizes.NumDiscStates  = 0;
            sizes.NumOutputs     = 3;  % vv_s_hat (3x1)
            sizes.NumInputs      = 3;  % g_hat (3x1)
            sizes.DirFeedthrough = 1;
            sizes.NumSampleTimes = 1;
            sys = simsizes(sizes);
            x0  = [];
            str = [];
            ts  = [0 0];  % Fixed-Step Auto 설정
            
        case 3  % 출력 계산
            % 내부 모델에서 예측한 중력 벡터 g_hat 입력
            g_hat = u(1:3);
            
            % 변환 행렬: X-Y 평면에 투영 (Z 성분 제거)
            T_vis = [1 0 0;
                     0 1 0;
                     0 0 0];
                 
            % VV 벡터 계산: 투영 후
            vv_s_hat = T_vis * g_hat;
            
            % 강제 정규화: Y 성분을 9.81로 설정 (X 성분은 그대로 유지)
            % 이 부분이 없으면 중력 벡터의 크기 자체가 변한다.
            vv_s_hat(2) = 9.81;
            vv_s_hat(1) = vv_s_hat(1)*10;
            sys = vv_s_hat;
            
        otherwise
            sys = [];
    end
end
