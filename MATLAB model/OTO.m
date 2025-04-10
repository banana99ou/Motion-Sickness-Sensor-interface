function [sys, x0, str, ts] = OTO(t, x, u, flag)
    % OTO 블록: IMU에서 측정한 가속도를 전정기관이 느낀 가속도로 변환
    % 입력: 
    %   - f [3x1] : IMU에서 측정된 관성 + 중력 가속도 벡터
    % 출력:
    %   - fs [3x1] : 전정기관이 감지한 가속도 벡터

    switch flag
        case 0 % 초기화
            sizes = simsizes;
            sizes.NumContStates  = 0;
            sizes.NumDiscStates  = 0;
            sizes.NumOutputs     = 3; % fs (3x1 벡터)
            sizes.NumInputs      = 3; % f (3x1 벡터)
            sizes.DirFeedthrough = 1;
            sizes.NumSampleTimes = 1;
            sys = simsizes(sizes);
            x0  = [];
            str = [];
            ts  = [0 0]; % Continuous sample time

        case 3 
            % 입력
            f = u(1:3); % 입력 벡터 (3x1)

            % 논문 수식 적용
            T_oto = eye(3); % 3x3 단위 행렬       
            fs = T_oto * f; 
            sys = fs; % 출력

        otherwise
            sys = [];
    end
end
