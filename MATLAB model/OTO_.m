function [sys, x0, str, ts] = OTO_(t, x, u, flag)
    % OTO_ 블록 (Otolith Model)
    % 입력:  
    %   - f_hat [3x1] : 내부 모델 예측 가속도 벡터  
    % 출력:  
    %   - fs_hat [3x1] : 감지된 GIA 가속도 벡터  

    switch flag
        case 0 % 초기화
            sizes = simsizes;
            sizes.NumContStates  = 0;
            sizes.NumDiscStates  = 0;
            sizes.NumOutputs     = 3; % fs_hat (3x1 벡터)
            sizes.NumInputs      = 3; % f_hat (3x1 벡터)
            sizes.DirFeedthrough = 1;
            sizes.NumSampleTimes = 1;
            sys = simsizes(sizes);
            x0  = [];
            str = [];
            ts  = [0 0]; % Fixed-Step Auto 설정

        case 3 
            f_hat = u(1:3); % 입력 가속도
            
            % 논문 수식 적용
            T_oto = eye(3); % 3x3 단위 행렬
            fs_hat = T_oto * f_hat; 
            sys = fs_hat; % 출력

        otherwise
            sys = [];
    end
end
