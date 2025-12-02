#!/usr/bin/env python3
import cv2
import numpy as np
import subprocess
import shlex
import gi

gi.require_version('Gst', '1.0')
from gi.repository import Gst

def rotate_image(image, angle):
    center = tuple(np.array(image.shape[1::-1]) / 2)
    rot_mat = cv2.getRotationMatrix2D(center, angle, 1.0)
    return cv2.warpAffine(image, rot_mat, image.shape[1::-1], flags=cv2.INTER_LINEAR)

# GStreamer 초기화 및 전송 파이프라인 구성
Gst.init(None)


send_pipeline_str = (
    "appsrc name=mysrc is-live=true block=true format=time do-timestamp=true "
    "caps=\"image/jpeg,format=(string)JPEG,width=(int)1920,height=(int)1080,framerate=(fraction)30/1\" ! "
    "queue ! jpegdec ! videoconvert ! "
    "videobalance contrast=1.0 brightness=0.1 saturation=1.5 hue=0.1 ! "
    "video/x-raw,format=(string)I420 ! "
    "x264enc tune=zerolatency bitrate=2000 speed-preset=superfast ! "
    "rtph264pay config-interval=1 pt=96 ! "
    "queue ! multiudpsink clients=\"192.168.50.242:5000,192.168.50.66:5555\" sync=false"
)



send_pipeline = Gst.parse_launch(send_pipeline_str)
appsrc = send_pipeline.get_by_name("mysrc")
send_pipeline.set_state(Gst.State.PLAYING)

# libcamera-vid를 이용하여 MJPEG 스트림 읽기
cmd = 'libcamera-vid --inline --nopreview -t 0 --codec mjpeg --width 1920 --height 1080 --framerate 30 -o - --camera 0'
process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

try:
    buffer = b""
    while True:
        # libcamera-vid로부터 데이터 읽기
        buffer += process.stdout.read(4096)
        a = buffer.find(b'\xff\xd8')  # JPEG 시작 마커
        b = buffer.find(b'\xff\xd9')  # JPEG 종료 마커

        if a != -1 and b != -1:
            jpg = buffer[a:b+2]
            buffer = buffer[b+2:]
            bgr_frame = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)
            if bgr_frame is not None:
                # 로컬 화면에 영상 출력
                cv2.imshow('Camera Stream', bgr_frame)

                # GStreamer AppSrc를 통해 JPEG 데이터를 전송
                gst_buffer = Gst.Buffer.new_allocate(None, len(jpg), None)
                gst_buffer.fill(0, jpg)
                ret = appsrc.emit("push-buffer", gst_buffer)
                if ret != Gst.FlowReturn.OK:
                    print("GStreamer 파이프라인으로 버퍼 전송 중 오류 발생:", ret)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
finally:
    process.terminate()
    send_pipeline.set_state(Gst.State.NULL)
    cv2.destroyAllWindows()
