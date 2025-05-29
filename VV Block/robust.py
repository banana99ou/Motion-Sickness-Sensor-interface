import cv2
import numpy as np
import matplotlib.pyplot as plt

# 사용할 이미지 파일 경로 (예시)
images = [
    "1.png",
    "2.jpg",
    "3.jpg"
]

# 각 이미지별 신뢰도 점수 (예시)
confidence_scores = [0.85, 0.72, 0.93]

# 서브플롯 생성 (3개의 이미지를 가로로 배치)
fig, axs = plt.subplots(1, 3, figsize=(15, 5))

for i, (img_path, conf) in enumerate(zip(images, confidence_scores)):
    # 1) 이미지 읽기
    img = cv2.imread(img_path)
    if img is None:
        print(f"이미지를 불러올 수 없습니다: {img_path}")
        # 없을 경우 흰색 바탕으로 대체
        img = np.ones((200, 200, 3), dtype=np.uint8) * 255

    # 2) 그레이스케일 변환 후, Canny 엣지 검출
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    edges = cv2.Canny(gray, 100, 200)

    # 3) 검출된 엣지를 원본 이미지 위에 빨간색으로 표시
    overlay = img.copy()
    overlay[edges != 0] = [0, 0, 255]  # 엣지 위치를 빨간색(BGR)으로 표시

    # 4) 신뢰도 점수를 이미지에 표시 (OpenCV 텍스트)
    text = f"Confidence: {conf*100:.1f}%"
    cv2.putText(
        overlay, text, (10, 30),
        cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 2, cv2.LINE_AA
    )

    # 5) Matplotlib 시각화를 위해 BGR -> RGB 변환
    overlay_rgb = cv2.cvtColor(overlay, cv2.COLOR_BGR2RGB)

    # 6) 서브플롯에 표시
    axs[i].imshow(overlay_rgb)
    axs[i].axis("off")
    axs[i].set_title(f"Image {i+1}")

plt.tight_layout()
plt.show()
