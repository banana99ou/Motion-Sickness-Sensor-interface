import cv2
import numpy as np
import pandas as pd
from cv2 import aruco

# --- 1. 비디오 파일 열기 및 프레임 속성 설정 ---
cap = cv2.VideoCapture('ar_video3.mp4')  # 동영상 파일 경로
if not cap.isOpened():
    print('[ERROR] 영상 파일을 열 수 없습니다.')
    exit()

fps   = cap.get(cv2.CAP_PROP_FPS) or 30    # FPS 정보 가져오기, 기본 30
delay = int(1000 / fps)                    # 프레임 간 대기 시간(ms)

# --- 비디오 저장 설정 (시각화 결과를 파일로 저장) ---
width  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out    = cv2.VideoWriter('tilt_visualization.mp4', fourcc, fps, (width, height))

# --- 2. AprilTag 36h11 딕셔너리 및 검출기 초기화 ---
aruco_dict = aruco.getPredefinedDictionary(aruco.DICT_APRILTAG_36h11)
params     = aruco.DetectorParameters()
try:
    detector     = aruco.ArucoDetector(aruco_dict, params)
    use_detector = True
except AttributeError:
    use_detector = False  # 구버전 폴백

# --- 3. 윈도우 및 텍스트 스타일 설정 ---
window_name   = 'Camera Tilt Vector'
cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
cv2.resizeWindow(window_name, width, height)

font        = cv2.FONT_HERSHEY_SIMPLEX
font_scale  = 2
thickness   = 3
color_text  = (0, 255, 0)
color_error = (0, 0, 255)
arrow_length= 200
tip_length  = 0.2

g = 9.81  # 중력 가속도

# --- 4. 데이터 저장용 리스트 초기화 ---
records = []  # [{'time':sec, 'vv_x':..., 'vv_y':..., 'vv_z':...}, ...]

# --- 5. 메인 루프: 각 프레임 처리 ---
while True:
    ret, frame = cap.read()
    if not ret:
        break

    # (A) 프레임 시간 계산 (초 단위)
    t = cap.get(cv2.CAP_PROP_POS_MSEC) / 1000.0

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # AprilTag 검출
    if use_detector:
        corners, ids, _ = detector.detectMarkers(gray)
    else:
        corners, ids, _ = aruco.detectMarkers(gray, aruco_dict, parameters=params)

    if ids is not None and corners:
        pts = corners[0].reshape(-1, 2).astype(int)

        # raw_angle → tilt
        dx, dy    = pts[1] - pts[0]
        raw_angle = (np.degrees(np.arctan2(dy, dx)) + 360) % 360
        tilt      = (90 - raw_angle + 360) % 360
        if tilt > 180:
            tilt = 360 - tilt

        # 시각 기울기 벡터
        theta = np.deg2rad(tilt)
        vv_x  = g * np.cos(theta)
        vv_y  = g * np.sin(theta)
        vv_z  = 0.0

        # 레코드 저장
        records.append({
            'time': t,
            'vv_x': vv_x,
            'vv_y': vv_y,
            'vv_z': vv_z
        })

        # 시각화
        cx, cy = pts.mean(axis=0).astype(int)
        end_x = int(cx + arrow_length * np.cos(theta))
        end_y = int(cy - arrow_length * np.sin(theta))
        cv2.polylines(frame, [pts], True, color_text, thickness)
        cv2.arrowedLine(frame, (cx, cy), (end_x, end_y),
                        color_text, thickness, tipLength=tip_length)
        cv2.putText(frame, f"tilt={tilt:.1f}deg", (cx-100,cy-50),
                    font, font_scale, color_text, thickness)
        cv2.putText(frame, f"vv=[{vv_x:.1f},{vv_y:.1f},0]",
                    (cx-100,cy+50), font, font_scale, color_text, thickness)
    else:
        cv2.putText(frame, "No AR marker", (20,50),
                    font, font_scale, color_error, thickness)

    # 화면 출력 & 파일 저장
    cv2.imshow(window_name, frame)
    out.write(frame)
    if cv2.waitKey(delay) & 0xFF == 27:
        break

# --- 6. 자원 해제 ---
cap.release()
out.release()
cv2.destroyAllWindows()

# --- 7. 0.01초 단위 정확 보간 & 엑셀 저장 ---
import math

# (a) 영상 전체 길이(초) 구하기
total_frames = cap.get(cv2.CAP_PROP_FRAME_COUNT)
duration     = total_frames / fps
n_seconds    = math.floor(duration)    # 예: 23.7초 → 23초로 자르려면 floor, 아니면 round 사용 가능
row_count    = n_seconds * 100         # 0.01초 간격 → 1초당 100개

# (b) 보간할 타임 그리드 생성
#    0.00, 0.01, 0.02, ..., (n_seconds-0.01)
new_t = np.linspace(0, n_seconds, row_count, endpoint=False)

# (c) 기존 기록된 레코드 DataFrame 생성
df = pd.DataFrame(records)
df = df.sort_values('time').reset_index(drop=True)

# (d) numpy.interp 으로 선형 보간
vvx_interp = np.interp(new_t, df['time'], df['vv_x'])
vvy_interp = np.interp(new_t, df['time'], df['vv_y'])
vvz_interp = np.interp(new_t, df['time'], df['vv_z'])

# (e) 보간된 결과 DataFrame
df_out = pd.DataFrame({
    'time': new_t,
    'vv_x': vvx_interp,
    'vv_y': vvy_interp,
    'vv_z': vvz_interp
})

# (f) 엑셀로 저장
output_path = 'tilt_vectors.xlsx'
df_out.to_excel(output_path, index=False)
print(f"[INFO] {n_seconds}s 영상 → {row_count}행 엑셀 '{output_path}' 저장 완료")

