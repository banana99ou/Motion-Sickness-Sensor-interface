function [sys, x0, str, ts] = VIS_g(t, x, u, flag)
    % VIS_g 블록 (Visual Perception 처리)
    % 입력:
    %   - vv [3x1] : 감지된 시각적 속도 벡터
    % 출력:
    %   - vv_s [3x1] : 변환된 시각적 속도 벡터

    switch flag
        case 0  % 초기화
            sizes = simsizes;
            sizes.NumContStates  = 0;  
            sizes.NumDiscStates  = 0; 
            sizes.NumOutputs     = 3;  % vv_s (3x1)
            sizes.NumInputs      = 3;  % vv (3x1)
            sizes.DirFeedthrough = 1;  
            sizes.NumSampleTimes = 1;
            sys = simsizes(sizes);
            x0  = [];  
            str = [];
            ts  = [0 0];  % Fixed-Step Auto 설정

        case 3  % 주요 연산 (출력)
            vv = u(1:3);  % 감지된 시각적 속도 벡터

            % 변환 행렬 
            T_vis = eye(3);  % 3x3 단위 행렬

            % 논문 수식 적용
            vv_s = T_vis * vv;
            sys = vv_s; % 출력

        otherwise
            sys = [];
    end
end
