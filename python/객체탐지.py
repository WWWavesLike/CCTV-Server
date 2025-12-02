#!/usr/bin/env python3
import os
import time
import gi
import cv2
import numpy as np
from ultralytics import YOLO
import socket  # ← TCP 소켓 추가

# 로그 억제
os.environ['GST_DEBUG'] = '0'

gi.require_version('Gst', '1.0')
from gi.repository import Gst

Gst.init(None)

def main():
    # 1) 모델 로드
    MODEL_PATH     = 'yolov8x.pt'
    CONF_THRESHOLD = 0.7
    model = YOLO(MODEL_PATH, verbose=False)
    classes = model.names

    # 1.1) TCP 소켓 초기화 및 서버 연결
    SERVER_IP   = '192.168.50.242'   # ← 실제 서버 IP로 변경
    SERVER_PORT = 12345         # ← 실제 서버 포트로 변경
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER_IP, SERVER_PORT))

    # 2) GStreamer 파이프라인
    pipeline_str = (
        'udpsrc port=5555 caps="application/x-rtp,media=video,clock-rate=90000,'
        'encoding-name=H264,payload=96" ! '
        'rtpjitterbuffer latency=200 drop-on-latency=true ! '
        'rtph264depay ! h264parse ! avdec_h264 ! '
        'videoconvert ! videoscale ! '
        'video/x-raw,format=BGR,width=1920,height=1080 ! '
        'appsink name=appsink sync=false max-buffers=1 drop=true'
    )
    pipeline = Gst.parse_launch(pipeline_str)
    appsink  = pipeline.get_by_name('appsink')
    pipeline.set_state(Gst.State.PLAYING)

    # 로그 스로틀 변수
    last_log_time = 0.0
    log_interval  = 1.0  # 1초

    try:
        while True:
            sample = appsink.emit("pull-sample")
            if not sample:
                continue

            buf    = sample.get_buffer()
            caps   = sample.get_caps().get_structure(0)
            width  = caps.get_int('width')[1]
            height = caps.get_int('height')[1]

            data  = buf.extract_dup(0, buf.get_size())
            frame = np.frombuffer(data, dtype=np.uint8).reshape((height, width, 3)).copy()

            # 3) 추론
            results = model(frame, conf=CONF_THRESHOLD, verbose=False)[0]

            # 4) 1초마다 로그 (갯수 포함) 및 TCP 전송
            now = time.time()
            if now - last_log_time >= log_interval:
                count = len(results.boxes)
                labels = [
                    f"{classes[int(b.cls[0])]}({float(b.conf[0])*100:.1f}%)"
                    for b in results.boxes
                ]

                if count:
                    log_msg = (
                        f"[{time.strftime('%H:%M:%S')}] Detected {count} objects: "
                        + ", ".join(labels)
                    )
                else:
                    log_msg = f"[{time.strftime('%H:%M:%S')}] Detected 0 objects"

                # 콘솔 출력
                print(log_msg)
                # TCP 소켓으로 전송 (UTF-8 인코딩, 개행 문자 포함)
                try:
                    sock.sendall((log_msg + "\n").encode('utf-8'))
                except Exception as e:
                    # 전송 실패 시 에러 출력 (필요 시 재연결 로직 추가)
                    print(f"Socket send error: {e}")

                last_log_time = now

            # 5) 화면 표시
            for box in results.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())
                cls_id = int(box.cls[0])
                conf   = float(box.conf[0])
                label  = classes.get(cls_id, str(cls_id))

                cv2.rectangle(frame, (x1, y1), (x2, y2), (0,255,0), 2)
                cv2.putText(
                    frame,
                    f"{label}:{conf:.2f}",
                    (x1, y1-10),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5, (0,255,0), 2
                )

            cv2.imshow('YOLOv8 Detection', frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        pass
    finally:
        pipeline.set_state(Gst.State.NULL)
        cv2.destroyAllWindows()
        sock.close()  # ← 소켓 닫기

if __name__ == '__main__':
    main()
