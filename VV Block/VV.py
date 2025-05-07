import matplotlib.pyplot as plt
import cv2
import numpy as np
import pandas as pd
import sys, getopt
import datetime
import time
import socket
import struct

# === 추가된 부분: 실시간 전송을 위한 UDP 송신 설정 (전역 변수) ===
udp_ip = '127.0.0.1'   # Simulink가 실행 중인 머신의 IP (예: localhost)
udp_port = 25000       # 사용 포트 번호
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
###########################
device = cv2.ocl.Device_getDefault()
print(f"Vendor ID: {device.vendorID()}")
print(f"Vendor name: {device.vendorName()}")
print(f"Name: {device.name()}")

print("Have OpenCL:")
print(cv2.ocl.haveOpenCL())
cv2.ocl.setUseOpenCL(True)
print("Using OpenCL:")
print(cv2.ocl.useOpenCL())
################################
ISOTIMEFORMAT = '%Y%m%d_%H%M%S'

def HOG_Canny(img, bin_n=180):
    # 1. 그레이스케일 변환 및 블러링
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    gray = cv2.GaussianBlur(gray, (11, 11), 3)
    # 정규화: [0,1] 범위
    gray_norm = cv2.normalize(gray, None, alpha=0, beta=1, 
                              norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F)

    # 2. Canny 엣지 검출 (입력은 0~255 범위의 uint8 이미지)
    edges = cv2.Canny((gray_norm * 255).astype(np.uint8), 20, 40)

    # 3. 모폴로지 연산 적용 (먼저 수행)
    # Canny 결과를 [0,1] 범위의 마스크로 변환
    edge_mask = edges.astype(np.float32) / 255.0
    # 세로 방향 커널: 폭 1, 높이 3 (세로 성분 강조)
    vertical_kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (1,3))
    # 침식과 팽창을 차례로 적용하여 노이즈 제거 및 세로 엣지 강화
    edge_mask = cv2.erode(edge_mask, vertical_kernel, iterations=1)
    edge_mask = cv2.dilate(edge_mask, vertical_kernel, iterations=3)

    # 모폴로지 처리 결과를 다시 0~255 범위의 uint8 이미지로 변환 (허프 변환에 사용)
    morph_edges = (edge_mask * 255).astype(np.uint8)


    # 4. 모폴로지 후 허프 변환 적용
    # 허프 변환 적용 여부 (True로 설정하면 허프 변환을 적용합니다.)
    apply_hough = False
    if apply_hough:
        # 허프 변환 파라미터 (필요에 따라 조정)
        min_line_length = 10   # 최소 직선 길이
        max_line_gap = 10      # 최대 간격
        hough_threshold = 20   # 직선 검출 임계값
        # 허프 변환 실행
        lines = cv2.HoughLinesP(morph_edges, 1, np.pi/180, threshold=hough_threshold, 
                                minLineLength=min_line_length, maxLineGap=max_line_gap)
        hough_mask = np.zeros_like(morph_edges)
        if lines is not None:
            for line in lines:
                x1, y1, x2, y2 = line[0]
                # 검출된 직선을 두께 2로 그립니다.
                cv2.line(hough_mask, (x1, y1), (x2, y2), 255, thickness=2)
        # 허프 변환 결과를 최종 엣지 마스크로 사용 (0~1 범위로 정규화)
        edge_mask = hough_mask.astype(np.float32) / 255.0

    # 5. Sobel 연산자로 그라디언트 계산 (각도 정보를 위해)
    gx = cv2.Sobel(gray_norm, cv2.CV_32F, 1, 0)
    gy = -cv2.Sobel(gray_norm, cv2.CV_32F, 0, 1)

    # 6. 그라디언트 크기와 각도 계산
    mag, rad = cv2.cartToPolar(gx, gy)
    mag = cv2.normalize(mag, None, alpha=0, beta=1, 
                        norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F)

    # 모폴로지 및 허프 변환된 엣지 마스크를 히스토그램 계산의 가중치로 사용
    mag_filter = edge_mask

    # 7. 각도를 0~180도로 변환하여 히스토그램 계산
    ang = np.int32(rad * 180 / np.pi)
    ang = np.where(ang == 360, 0, ang)
    ang = np.where(ang >= 180, ang - 180, ang)

    # np.bincount를 사용해 각도 히스토그램 계산 (길이 bin_n)
    hist = np.bincount(ang.ravel(), weights=mag_filter.ravel(), minlength=bin_n)[:bin_n]

    return np.reshape(gy, (gy.shape[0], gy.shape[1], 1)), np.array(hist, np.float32), mag, mag_filter

def weighted_median(values, weights):
    # values와 weights는 numpy array여야 함
    sorter = np.argsort(values)
    values_sorted = values[sorter]
    weights_sorted = weights[sorter]
    cumulative_weights = np.cumsum(weights_sorted)
    cutoff = cumulative_weights[-1] / 2.0
    median_idx = np.where(cumulative_weights >= cutoff)[0][0]
    return values_sorted[median_idx]



def VV(filename, scale):
    
    if filename == "camera":
        theTime = datetime.datetime.now().strftime(ISOTIMEFORMAT)
        csv_name = "./camera_" + str(theTime) + ".csv"
        video_name = "./camera_" + str(theTime) + ".mp4"
        cap = cv2.VideoCapture(CAMERA_NO, cv2.CAP_DSHOW)
    else:
        csv_name = "./VV_" + filename[2:] + ".csv"
        video_name = "./VV_Video_" + filename[2:] + ".mp4"
        cap = cv2.VideoCapture(filename)

    if not cap.isOpened():
        print("Error opening video file")
        return

    width = int(cap.get(3))
    height = int(cap.get(4))
    VV_all = []

    fourcc = cv2.VideoWriter_fourcc('m', 'p', '4', 'v')
    video_out = cv2.VideoWriter(video_name,
                                fourcc,
                                cap.get(cv2.CAP_PROP_FPS),
                                (int((2 * width) / scale), int((height * 2.6) / scale))
                                )
    prev_time = time.time()
    VV_m_1 = 90
    frame_t = 0

    # FPS 계산을 위한 변수 초기화
    fps_list = []         # 각 프레임의 FPS를 저장할 리스트
    prev_time = time.time()  # 첫 프레임 전에 시간 기록

    while cap.isOpened():
        ret, input_img = cap.read()
        if ret:
            # 프레임 크기 조절
            input_img = cv2.resize(input_img,
                                   (int(input_img.shape[1] / scale / 1.5),
                                    int(input_img.shape[0] / scale / 1.5)))

            # HOG_Canny 함수를 통해 Canny 기반 엣지 검출 수행 (모폴로지 처리 포함)
            hog_x_img, hog_hist_180, hog_mag, hog_mag_filter = HOG_Canny(img=input_img)

            # # VV 각도 계산 (히스토그램의 특정 구간에서 최고값 3개 사용) 원래 논문 알고리즘 
            # VV_best_1 = np.argsort(hog_hist_180[30:151])[-1]
            # VV_best_2 = np.argsort(hog_hist_180[30:151])[-2]
            # VV_best_3 = np.argsort(hog_hist_180[30:151])[-3]

            # Sum_VV_samples = (hog_hist_180[VV_best_1] +
            #                 hog_hist_180[VV_best_2] +
            #                 hog_hist_180[VV_best_3])
            # VV = (VV_best_1 * (hog_hist_180[VV_best_1] / Sum_VV_samples) +
            #     VV_best_2 * (hog_hist_180[VV_best_2] / Sum_VV_samples) +
            #     VV_best_3 * (hog_hist_180[VV_best_3] / Sum_VV_samples)) + 30

            # if np.isnan(VV):
            #     VV = VV_m_1
            # VV = (0.7 * VV + 0.3 * VV_m_1)

            # 관심 구간: 30 ~ 150도에 해당하는 bin 사용 (히스토그램 인덱스 30부터 150)
            sel_angles = np.arange(30, 151)      # 30, 31, ..., 150
            sel_hist = hog_hist_180[30:151]

            # 가중 중앙값 계산 (극단치에 덜 민감)
            raw_VV = weighted_median(sel_angles, sel_hist)

            # NaN 처리: 계산 결과가 NaN이면 이전 값 사용
            if np.isnan(raw_VV):
                raw_VV = VV_m_1

            # (선택사항) 부드러운 변화 적용: 이전 값과 새로운 값의 선형 보간을 사용하여 갑작스런 변화 완화
            VV = 0.75 * raw_VV + 0.1 * VV_m_1

            VV_all = np.append(VV_all, VV)
            VV_m_1 = VV

            VV_all = np.append(VV_all, VV)
            VV_m_1 = VV

            VV_rad = np.radians(VV)
            
            # VV 각도에 따른 3x1 벡터 전송 (x, y, z)
            VV_rad = np.radians(VV)
            vv_x = 9.8 * np.cos(VV_rad)
            vv_y = 9.8 * np.sin(VV_rad)
            vv_z = 0.0  # z 성분은 0으로 설정

            # 3개의 double 데이터 (3x1 배열)를 바이너리 형식으로 패킹 후 전송
            data_packet = struct.pack('3d', vv_x, vv_y, vv_z)
            sock.sendto(data_packet, (udp_ip, udp_port))
            

            center = [int(input_img.shape[1] / 2), int(input_img.shape[0] / 2)]
            M = cv2.getRotationMatrix2D(center, 90 - VV, 1)
            Caribreated_img = cv2.warpAffine(input_img, M, (input_img.shape[1], input_img.shape[0]))

            try:
                cv2.putText(input_img, "VV_dig=" + str(int(VV)), (10, 30),
                            cv2.FONT_HERSHEY_PLAIN, 2, (0, 255, 0), 2, cv2.LINE_AA)
            except:
                pass


            # FPS 계산: 현재 프레임 처리 시간을 측정하고 리스트에 저장
            current_time = time.time()
            fps = 1.0 / (current_time - prev_time)
            fps_list.append(fps)
            prev_time = current_time

            # FPS를 빨간색으로 화면에 출력 (위치: (10,60))
            cv2.putText(input_img, "FPS: " + str(int(fps)), (10, 60),
                        cv2.FONT_HERSHEY_PLAIN, 2, (0, 0, 255), 2, cv2.LINE_AA)

            # 처리 해상도 텍스트 추가 (FPS 아래에 표시)
            res_text = "Res: {}x{}".format(input_img.shape[1], input_img.shape[0])
            cv2.putText(input_img, res_text, (10, 90),
                        cv2.FONT_HERSHEY_PLAIN, 2, (255, 255, 0), 2, cv2.LINE_AA)

            cv2.line(input_img, (0, int(input_img.shape[0] / 2)),
                     (input_img.shape[1], int(input_img.shape[0] / 2)), (0, 0, 0),
                     thickness=2, lineType=cv2.LINE_4)
            cv2.line(input_img, (int(input_img.shape[1] / 2), int(input_img.shape[0] / 2)),
                     (int(input_img.shape[1] / 2), 0), (0, 0, 0),
                     thickness=2, lineType=cv2.LINE_4)

            try:
                cv2.arrowedLine(input_img, (int(input_img.shape[1] / 2), int(input_img.shape[0] / 2)),
                                (int(input_img.shape[1] / 2) + int((input_img.shape[0] / 3) * np.cos(VV_rad)),
                                 int(input_img.shape[0] / 2) - int((input_img.shape[0] / 3) * np.sin(VV_rad))),
                                (0, 255, 0), thickness=4)
                cv2.ellipse(input_img, (int(input_img.shape[1] / 2), int(input_img.shape[0] / 2)),
                            (int(input_img.shape[0] / 6), int(input_img.shape[0] / 6)), 0, 0, -VV,
                            (0, 255, 0), thickness=2)
            except:
                pass

            cv2.line(Caribreated_img, (0, int(Caribreated_img.shape[0] / 2)),
                     (Caribreated_img.shape[1], int(Caribreated_img.shape[0] / 2)),
                     (0, 0, 0), thickness=2, lineType=cv2.LINE_4)

            imghstack = np.hstack((input_img, Caribreated_img))
            img2 = np.hstack((hog_mag, hog_mag_filter))

            hoghstack = np.zeros_like(imghstack)
            hoghstack[:, :, 0] = (img2.astype("float32") * 255)
            hoghstack[:, :, 1] = (img2.astype("float32") * 255)
            hoghstack[:, :, 2] = (img2.astype("float32") * 255)

            imgvstack = np.vstack((imghstack, hoghstack))

            frame_t += 1

            cv2.imshow('Video', imgvstack)
            video_out.write(imgvstack)
            c = cv2.waitKey(1)
            if c == 27:
                break
        else:
            break
    sock.close()
    
    # 루프 종료 후 평균 FPS 계산 및 출력
    if fps_list:
        avg_fps = sum(fps_list) / len(fps_list)
        print("Average FPS: {:.2f}".format(avg_fps))

    VV_all_rad = (VV_all.astype("float32") * np.pi / 180)
    VV_x_all = 9.8 * np.cos(VV_all_rad)
    VV_y_all = 9.8 * np.sin(VV_all_rad)

    save_data = pd.DataFrame({'VV_acc_x[m/s^2]': VV_x_all,
                              'VV_acc_y[m/s^2]': VV_y_all,
                              'VV_acc_rad': VV_all_rad,
                              'VV_acc_dig': VV_all})
    print(csv_name)
    save_data.to_csv(csv_name)

    cap.release()
    video_out.release()
    cv2.destroyAllWindows()


# 예시: main 함수에서 인자 처리 후 VV 함수 호출
def main(argv):
    global filename, scale, CAMERA, CAMERA_NO
    
    # UDP 송신 설정 (Simulink에서 수신할 IP 및 포트)
    udp_ip = '127.0.0.1'   # Simulink가 실행 중인 머신의 IP (예: localhost)
    udp_port = 25000       # 사용 포트 번호
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    if CAMERA == True:
        filename = "camera"
    else:
        try:
            opts, args = getopt.getopt(argv, "hi:c:cp:s:", ["inputfile=", "camera=", "camera_port=", "scale="])
        except getopt.GetoptError:
            print('VV_for_SVC.py -i <inputfile> -camera <True/False> -camera_port <port number>')
            sys.exit(2)
        for opt, arg in opts:
            if opt == '-h':
                print('VV_for_SVC.py -i <inputfile> -camera <True/False> -camera_port <port number>')
                print('Exmple 1: VV_for_SVC.py -i ./test.mp4 --scale 2')
                print('Exmple 2: VV_for_SVC.py --camera True --camera_port 0 --scale 1')
                sys.exit()
            elif opt in ("-i", "--inputfile"):
                filename = arg
            elif opt in ("-c", "--camera"):
                if arg == "True":
                    filename = "camera"
            elif opt in ("-s", "--scale"):
                scale = int(arg)
    VV(filename, scale)

if __name__ == "__main__":
    CAMERA = False
    CAMERA_NO = 0
    scale = 2
    kernel = np.ones((3, 3), np.uint8)
    filename = "./test2.mp4"
    main(sys.argv[1:])

# # 실시간 카메라 받기
# if __name__ == "__main__":
#     CAMERA = True     # 카메라 사용 설정
#     CAMERA_NO = 0     # 기본 카메라 포트 (여러 개 있을 경우 원하는 번호로 변경)
#     scale = 1
#     kernel = np.ones((3, 3), np.uint8)
#     filename = "camera"  # 파일 이름 대신 'camera' 문자열을 사용하여 카메라 branch로 진입
#     main(sys.argv[1:])