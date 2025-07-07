from libs.PipeLine import PipeLine, ScopedTiming
from libs.AIBase import AIBase
from libs.AI2D import Ai2d
import os
import ujson
from media.media import *
from time import *
import nncase_runtime as nn
import ulab.numpy as np
import time
import utime
import image
import random
import gc
import sys
import aicube
from machine import Pin
from machine import FPIOA
from machine import UART

# Custom Hand Detection App
class HandDetApp(AIBase):
    def __init__(self, kmodel_path, model_input_size, anchors, confidence_threshold=0.2, nms_threshold=0.5, nms_option=False, strides=[8,16,32], rgb888p_size=[1920,1080], display_size=[1920,1080], debug_mode=0):
        super().__init__(kmodel_path, model_input_size, rgb888p_size, debug_mode)
        self.kmodel_path = kmodel_path
        self.model_input_size = model_input_size
        self.confidence_threshold = confidence_threshold
        self.nms_threshold = nms_threshold
        self.anchors = anchors
        self.strides = strides
        self.nms_option = nms_option
        self.rgb888p_size = [ALIGN_UP(rgb888p_size[0],16), rgb888p_size[1]]
        self.display_size = [ALIGN_UP(display_size[0],16), display_size[1]]
        self.debug_mode = debug_mode
        self.ai2d = Ai2d(debug_mode)
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)

    def config_preprocess(self, input_image_size=None):
        with ScopedTiming("set preprocess config", self.debug_mode > 0):
            ai2d_input_size = input_image_size if input_image_size else self.rgb888p_size
            top, bottom, left, right = self.get_padding_param()
            self.ai2d.pad([0, 0, 0, 0, top, bottom, left, right], 0, [114, 114, 114])
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            self.ai2d.build([1,3,ai2d_input_size[1],ai2d_input_size[0]], [1,3,self.model_input_size[1],self.model_input_size[0]])

    def postprocess(self, results):
        with ScopedTiming("postprocess", self.debug_mode > 0):
            dets = aicube.anchorbasedet_post_process(results[0], results[1], results[2], self.model_input_size, self.rgb888p_size, self.strides, 1, self.confidence_threshold, self.nms_threshold, self.anchors, self.nms_option)
            return dets

    def get_padding_param(self):
        dst_w = self.model_input_size[0]
        dst_h = self.model_input_size[1]
        input_width = self.rgb888p_size[0]
        input_high = self.rgb888p_size[1]
        ratio_w = dst_w / input_width
        ratio_h = dst_h / input_high
        if ratio_w < ratio_h:
            ratio = ratio_w
        else:
            ratio = ratio_h
        new_w = int(ratio * input_width)
        new_h = int(ratio * input_high)
        dw = (dst_w - new_w) / 2
        dh = (dst_h - new_h) / 2
        top = int(round(dh - 0.1))
        bottom = int(round(dh + 0.1))
        left = int(round(dw - 0.1))
        right = int(round(dw + 0.1))
        return top, bottom, left, right

# Custom Hand Recognition App
class HandRecognitionApp(AIBase):
    def __init__(self, kmodel_path, model_input_size, labels, rgb888p_size=[1920,1080], display_size=[1920,1080], debug_mode=0):
        super().__init__(kmodel_path, model_input_size, rgb888p_size, debug_mode)
        self.kmodel_path = kmodel_path
        self.model_input_size = model_input_size
        self.labels = labels
        self.rgb888p_size = [ALIGN_UP(rgb888p_size[0],16), rgb888p_size[1]]
        self.display_size = [ALIGN_UP(display_size[0],16), display_size[1]]
        self.crop_params = []
        self.debug_mode = debug_mode
        self.ai2d = Ai2d(debug_mode)
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)

    def config_preprocess(self, det, input_image_size=None):
        with ScopedTiming("set preprocess config", self.debug_mode > 0):
            ai2d_input_size = input_image_size if input_image_size else self.rgb888p_size
            self.crop_params = self.get_crop_param(det)
            self.ai2d.crop(self.crop_params[0], self.crop_params[1], self.crop_params[2], self.crop_params[3])
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            self.ai2d.build([1,3,ai2d_input_size[1],ai2d_input_size[0]], [1,3,self.model_input_size[1],self.model_input_size[0]])

    def postprocess(self, results):
        with ScopedTiming("postprocess", self.debug_mode > 0):
            result = results[0].reshape(results[0].shape[0]*results[0].shape[1])
            x_softmax = self.softmax(result)
            idx = np.argmax(x_softmax)
            confidence = round(x_softmax[idx], 2)  # 获取最高置信度
            text = " " + self.labels[idx] + ": " + str(confidence)
            return text, idx, confidence  # 返回文本、索引和置信度

    def get_crop_param(self, det_box):
        x1, y1, x2, y2 = det_box[2], det_box[3], det_box[4], det_box[5]
        w, h = int(x2 - x1), int(y2 - y1)
        w_det = int(float(x2 - x1) * self.display_size[0] // self.rgb888p_size[0])
        h_det = int(float(y2 - y1) * self.display_size[1] // self.rgb888p_size[1])
        x_det = int(x1*self.display_size[0] // self.rgb888p_size[0])
        y_det = int(y1*self.display_size[1] // self.rgb888p_size[1])
        length = max(w, h)/2
        cx = (x1+x2)/2
        cy = (y1+y2)/2
        ratio_num = 1.26*length
        x1_kp = int(max(0, cx-ratio_num))
        y1_kp = int(max(0, cy-ratio_num))
        x2_kp = int(min(self.rgb888p_size[0]-1, cx+ratio_num))
        y2_kp = int(min(self.rgb888p_size[1]-1, cy+ratio_num))
        w_kp = int(x2_kp - x1_kp + 1)
        h_kp = int(y2_kp - y1_kp + 1)
        return [x1_kp, y1_kp, w_kp, h_kp]

    def softmax(self, x):
        x -= np.max(x)
        x = np.exp(x) / np.sum(np.exp(x))
        return x

# Custom Fall Detection App
class FallDetectionApp(AIBase):
    def __init__(self, kmodel_path, model_input_size, labels, anchors, confidence_threshold=0.2, nms_threshold=0.5, nms_option=False, strides=[8,16,32], rgb888p_size=[224,224], display_size=[1920,1080], debug_mode=0):
        super().__init__(kmodel_path, model_input_size, rgb888p_size, debug_mode)
        self.kmodel_path = kmodel_path
        self.model_input_size = model_input_size
        self.labels = labels
        self.anchors = anchors
        self.strides = strides
        self.confidence_threshold = confidence_threshold
        self.nms_threshold = nms_threshold
        self.nms_option = nms_option
        self.rgb888p_size = [ALIGN_UP(rgb888p_size[0], 16), rgb888p_size[1]]
        self.display_size = [ALIGN_UP(display_size[0], 16), display_size[1]]
        self.debug_mode = debug_mode
        self.color = [(255,0, 0, 255), (255,0, 255, 0), (255,255,0, 0), (255,255,0, 255)]
        self.ai2d = Ai2d(debug_mode)
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)
        
        # Initialize LED control
        fpioa = FPIOA()
        fpioa.set_function(6, FPIOA.GPIO6)
        fpioa.set_function(42, FPIOA.GPIO42)
        self.LED1 = Pin(6, Pin.OUT)
        self.LED2 = Pin(42, Pin.OUT)
        self.fall_detected_count = 0
        self.fall_threshold = 5  # Number of consecutive frames to confirm fall
        self.led_state = False
        self.last_led_toggle_time = 0
        
        # Initialize UART
        fpioa.set_function(11, FPIOA.UART2_TXD)
        fpioa.set_function(12, FPIOA.UART2_RXD)
        self.uart = UART(UART.UART2, 115200)
        self.last_sos_time = 0
        self.hand_sos_sent = False
        self.last_hand_sos_time = 0  # 新增：记录上次手势报警时间
        self.hand_sos_interval = 10000  # 新增：手势报警间隔10秒(毫秒)

    def config_preprocess(self, input_image_size=None):
        with ScopedTiming("set preprocess config", self.debug_mode > 0):
            ai2d_input_size = input_image_size if input_image_size else self.rgb888p_size
            top, bottom, left, right = self.get_padding_param()
            self.ai2d.pad([0, 0, 0, 0, top, bottom, left, right], 0, [0,0,0])
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            self.ai2d.build([1,3,ai2d_input_size[1],ai2d_input_size[0]], [1,3,self.model_input_size[1],self.model_input_size[0]])

    def postprocess(self, results):
        with ScopedTiming("postprocess", self.debug_mode > 0):
            dets = aicube.anchorbasedet_post_process(results[0], results[1], results[2], self.model_input_size, self.rgb888p_size, self.strides, len(self.labels), self.confidence_threshold, self.nms_threshold, self.anchors, self.nms_option)
            return dets

    def draw_result(self, pl, dets, hand_det_res=None, hand_rec_res=None):
        with ScopedTiming("display_draw", self.debug_mode > 0):
            current_fall_detected = False
            
            # Draw fall detection results
            if dets:
                pl.osd_img.clear()
                for det_box in dets:
                    x1, y1, x2, y2 = det_box[2], det_box[3], det_box[4], det_box[5]
                    w = (x2 - x1) * self.display_size[0] // self.rgb888p_size[0]
                    h = (y2 - y1) * self.display_size[1] // self.rgb888p_size[1]
                    x1 = int(x1 * self.display_size[0] // self.rgb888p_size[0])
                    y1 = int(y1 * self.display_size[1] // self.rgb888p_size[1])
                    x2 = int(x2 * self.display_size[0] // self.rgb888p_size[0])
                    y2 = int(y2 * self.display_size[1] // self.rgb888p_size[1])
                    pl.osd_img.draw_rectangle(x1, y1, int(w), int(h), color=self.color[det_box[0]], thickness=2)
                    pl.osd_img.draw_string_advanced(x1, y1-50, 32, " " + self.labels[det_box[0]] + " " + str(round(det_box[1],2)), color=self.color[det_box[0]])
                    
                    # Check for fall state
                    if det_box[0] == 0 and det_box[1] >= 0.7:  # 0 is "Fall" class
                        current_fall_detected = True
            else:
                pl.osd_img.clear()
            
            # Draw hand detection and recognition results
            if hand_det_res and hand_rec_res:
                for k in range(len(hand_det_res)):
                    det_box = hand_det_res[k]
                    res = hand_rec_res[k]
                    x1, y1, x2, y2 = det_box[2], det_box[3], det_box[4], det_box[5]
                    w_det = int(float(x2 - x1) * self.display_size[0] // self.rgb888p_size[0])
                    h_det = int(float(y2 - y1) * self.display_size[1] // self.rgb888p_size[1])
                    x_det = int(x1*self.display_size[0] // self.rgb888p_size[0])
                    y_det = int(y1*self.display_size[1] // self.rgb888p_size[1])
                    pl.osd_img.draw_rectangle(x_det, y_det, w_det, h_det, color=(255, 0, 255, 0), thickness=2)
                    pl.osd_img.draw_string_advanced(x_det, y_det-50, 32, res[0], color=(255,0, 255, 0))
            
            # Update fall detection state counter
            if current_fall_detected:
                self.fall_detected_count = min(self.fall_detected_count + 1, self.fall_threshold)
            else:
                self.fall_detected_count = max(self.fall_detected_count - 1, 0)
            
            # Control LEDs and UART based on detections
            self.control_leds_and_uart(current_fall_detected, hand_rec_res)

    def control_leds_and_uart(self, fall_detected, hand_rec_res=None):
        current_time = utime.ticks_ms()
    
        # 检查手势（仅当没有跌倒时处理）
        hand_sos_triggered = False
        if not fall_detected and hand_rec_res:
            for res in hand_rec_res:
                if res[1] == 3 and res[2] >= 0.6:  # "five"手势且置信度≥0.6
                    hand_sos_triggered = True
                    break
    
        # 优先级处理：跌倒 > 手势
        if self.fall_detected_count >= self.fall_threshold:
            # 跌倒检测处理
            if utime.ticks_diff(current_time, self.last_led_toggle_time) > 500:
                self.led_state = not self.led_state
                self.LED1.value(self.led_state)
                self.LED2.value(self.led_state)
                self.last_led_toggle_time = current_time
            
                if utime.ticks_diff(current_time, self.last_sos_time) > 30000:
                    self.uart.write("SOS1: Fall\n")
                    self.last_sos_time = current_time
                    self.hand_sos_sent = True  # 阻断手势触发
                
                    text = self.uart.read(128)
                    if text is not None:
                        print("Received:", text.decode('utf-8'))
    
        elif hand_sos_triggered and not self.hand_sos_sent:
            # 手势处理（仅在无跌倒、置信度≥0.6且超过10秒间隔时执行）
            if utime.ticks_diff(current_time, self.last_hand_sos_time) > self.hand_sos_interval:
                self.uart.write("SOS2: Hand \n")
                self.hand_sos_sent = True
                self.last_sos_time = current_time
                self.last_hand_sos_time = current_time  # 记录本次手势报警时间
            
                self.LED1.on()
                self.LED2.on()
                utime.sleep_ms(200)
                self.LED1.off()
                self.LED2.off()
                utime.sleep_ms(200)
                self.LED1.on()
                self.LED2.on()
                utime.sleep_ms(200)
                self.LED1.off()
                self.LED2.off()
            
                text = self.uart.read(128)
                if text is not None:
                    print("Received:", text.decode('utf-8'))
    
        # 状态重置（当没有检测到任何警报时）
        if not fall_detected and not hand_sos_triggered:
            self.hand_sos_sent = False
            self.LED1.off()
            self.LED2.off()
            self.led_state = False

    def get_padding_param(self):
        dst_w = self.model_input_size[0]
        dst_h = self.model_input_size[1]
        input_width = self.rgb888p_size[0]
        input_high = self.rgb888p_size[1]
        ratio_w = dst_w / input_width
        ratio_h = dst_h / input_high
        if ratio_w < ratio_h:
            ratio = ratio_w
        else:
            ratio = ratio_h
        new_w = int(ratio * input_width)
        new_h = int(ratio * input_high)
        dw = (dst_w - new_w) / 2
        dh = (dst_h - new_h) / 2
        top = int(round(dh - 0.1))
        bottom = int(round(dh + 0.1))
        left = int(round(dw - 0.1))
        right = int(round(dw - 0.1))
        return top, bottom, left, right

# Main Application Class
class CombinedDetectionApp:
    def __init__(self, display_mode="lcd"):
        # Configuration parameters
        rgb888p_size = [1920, 1080]
        
        if display_mode == "hdmi":
            display_size = [1920, 1080]
        else:
            display_size = [540, 940]
        
        # Hand detection parameters
        hand_det_kmodel_path = "/sdcard/hand_detect.kmodel"
        hand_rec_kmodel_path = "/sdcard/gesture.kmodel"
        hand_det_input_size = [512, 512]
        hand_rec_input_size = [224, 224]
        hand_labels = ["gun", "other", "yeah", "wave"]
        hand_anchors = [26,27, 53,52, 75,71, 80,99, 106,82, 99,134, 140,113, 161,172, 245,276]
        
        # Fall detection parameters
        fall_kmodel_path = "/sdcard/fall_detect.kmodel"
        fall_labels = ["Fall", "NoFall"]
        fall_anchors = [10, 13, 16, 30, 33, 23, 30, 61, 62, 45, 59, 119, 116, 90, 156, 198, 373, 326]
        
        # Initialize pipeline
        self.pl = PipeLine(rgb888p_size=rgb888p_size, display_size=display_size, display_mode=display_mode)
        self.pl.create()
        
        # Initialize detection models
        self.hand_det = HandDetApp(hand_det_kmodel_path, model_input_size=hand_det_input_size, 
                                  anchors=hand_anchors, confidence_threshold=0.6, nms_threshold=0.5, 
                                  rgb888p_size=rgb888p_size, display_size=display_size)
        self.hand_rec = HandRecognitionApp(hand_rec_kmodel_path, model_input_size=hand_rec_input_size, 
                                         labels=hand_labels, rgb888p_size=rgb888p_size, display_size=display_size)
        self.fall_det = FallDetectionApp(fall_kmodel_path, model_input_size=[640, 640], 
                                        labels=fall_labels, anchors=fall_anchors, 
                                        confidence_threshold=0.4, nms_threshold=0.45,
                                        rgb888p_size=rgb888p_size, display_size=display_size)
        
        # Configure preprocessing
        self.hand_det.config_preprocess()
        self.fall_det.config_preprocess()

    def run(self):
        try:
            while True:
                with ScopedTiming("total", 1):
                    img = self.pl.get_frame()
                    
                    # Run hand detection and recognition
                    det_boxes = self.hand_det.run(img)
                    hand_det_res = []
                    hand_rec_res = []
                    
                    for det_box in det_boxes:
                        x1, y1, x2, y2 = det_box[2], det_box[3], det_box[4], det_box[5]
                        w, h = int(x2 - x1), int(y2 - y1)
                        
                        # Filter out small or edge detections
                        if (h < (0.1*self.hand_det.rgb888p_size[1])):
                            continue
                        if (w < (0.25*self.hand_det.rgb888p_size[0]) and 
                            ((x1 < (0.03*self.hand_det.rgb888p_size[0])) or 
                            (x2 > (0.97*self.hand_det.rgb888p_size[0])))):
                            continue
                        if (w < (0.15*self.hand_det.rgb888p_size[0]) and 
                            ((x1 < (0.01*self.hand_det.rgb888p_size[0])) or 
                            (x2 > (0.99*self.hand_det.rgb888p_size[0])))):
                            continue
                            
                        self.hand_rec.config_preprocess(det_box)
                        text, idx, confidence = self.hand_rec.run(img)  # 现在返回三个值
                        hand_det_res.append(det_box)
                        hand_rec_res.append((text, idx, confidence))  # 存储文本、索引和置信度
                    
                    # Run fall detection
                    fall_res = self.fall_det.run(img)
                    
                    # Draw results and handle SOS signals
                    self.fall_det.draw_result(self.pl, fall_res, hand_det_res, hand_rec_res)
                    
                    self.pl.show_image()
                    gc.collect()
                    
        except KeyboardInterrupt:
            pass
        
        finally:
            self.hand_det.deinit()
            self.hand_rec.deinit()
            self.fall_det.deinit()
            self.pl.destroy()

if __name__ == "__main__":
    app = CombinedDetectionApp(display_mode="lcd")
    app.run()