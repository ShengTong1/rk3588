"""
YOLOv8后处理模块
"""

import numpy as np
import cv2


class YOLOv8PostProcessor:
    def __init__(self, obj_thresh=0.25, nms_thresh=0.45, img_size=(640, 640)):
        self.obj_thresh = obj_thresh
        self.nms_thresh = nms_thresh
        self.img_size = img_size
    
    def filter_boxes(self, boxes, box_confidences, box_class_probs):
        box_confidences = box_confidences.reshape(-1)
        class_max_score = np.max(box_class_probs, axis=-1)
        classes = np.argmax(box_class_probs, axis=-1)
        _class_pos = np.where(class_max_score * box_confidences >= self.obj_thresh)
        scores = (class_max_score * box_confidences)[_class_pos]
        return boxes[_class_pos], classes[_class_pos], scores
    
    def nms_boxes(self, boxes, scores):
        x, y = boxes[:, 0], boxes[:, 1]
        w, h = boxes[:, 2] - boxes[:, 0], boxes[:, 3] - boxes[:, 1]
        areas = w * h
        order = scores.argsort()[::-1]

        keep = []
        while order.size > 0:
            i = order[0]
            keep.append(i)
            xx1 = np.maximum(x[i], x[order[1:]])
            yy1 = np.maximum(y[i], y[order[1:]])
            xx2 = np.minimum(x[i] + w[i], x[order[1:]] + w[order[1:]])
            yy2 = np.minimum(y[i] + h[i], y[order[1:]] + h[order[1:]])
            w1 = np.maximum(0.0, xx2 - xx1 + 0.00001)
            h1 = np.maximum(0.0, yy2 - yy1 + 0.00001)
            inter = w1 * h1
            ovr = inter / (areas[i] + areas[order[1:]] - inter)
            inds = np.where(ovr <= self.nms_thresh)[0]
            order = order[inds + 1]
        return np.array(keep)
    
    def dfl(self, position):
        n, c, h, w = position.shape
        p_num = 4
        mc = c // p_num
        y = position.reshape(n, p_num, mc, h, w)
        exp_y = np.exp(y - np.max(y, axis=2, keepdims=True))
        y = exp_y / np.sum(exp_y, axis=2, keepdims=True)
        acc_metrix = np.arange(mc).reshape(1, 1, mc, 1, 1)
        return np.sum(y * acc_metrix, axis=2)
    
    def box_process(self, position):
        grid_h, grid_w = position.shape[2:4]
        col, row = np.meshgrid(np.arange(0, grid_w), np.arange(0, grid_h))
        grid = np.concatenate((col.reshape(1, 1, grid_h, grid_w),
                              row.reshape(1, 1, grid_h, grid_w)), axis=1)
        stride = np.array([self.img_size[1]//grid_h, self.img_size[0]//grid_w]).reshape(1, 2, 1, 1)
        position = self.dfl(position)
        box_xy = grid + 0.5 - position[:, 0:2, :, :]
        box_xy2 = grid + 0.5 + position[:, 2:4, :, :]
        return np.concatenate((box_xy*stride, box_xy2*stride), axis=1)
    
    def post_process(self, input_data):
        boxes, scores, classes_conf = [], [], []
        default_branch = 3
        pair_per_branch = len(input_data) // default_branch

        for i in range(default_branch):
            boxes.append(self.box_process(input_data[pair_per_branch*i]))
            classes_conf.append(input_data[pair_per_branch*i+1])
            scores.append(np.ones_like(input_data[pair_per_branch*i+1][:, :1, :, :], dtype=np.float32))

        def sp_flatten(_in):
            ch = _in.shape[1]
            return _in.transpose(0, 2, 3, 1).reshape(-1, ch)

        boxes = np.concatenate([sp_flatten(_v) for _v in boxes])
        classes_conf = np.concatenate([sp_flatten(_v) for _v in classes_conf])
        scores = np.concatenate([sp_flatten(_v) for _v in scores])

        boxes, classes, scores = self.filter_boxes(boxes, scores, classes_conf)

        nboxes, nclasses, nscores = [], [], []
        for c in set(classes):
            inds = np.where(classes == c)
            b, c_cls, s = boxes[inds], classes[inds], scores[inds]
            keep = self.nms_boxes(b, s)
            if len(keep) != 0:
                nboxes.append(b[keep])
                nclasses.append(c_cls[keep])
                nscores.append(s[keep])

        if not nclasses:
            return None, None, None

        return np.concatenate(nboxes), np.concatenate(nclasses), np.concatenate(nscores)
    
    def draw_detections(self, image, boxes, scores, classes, class_names):
        if boxes is None:
            return image

        for box, score, cls in zip(boxes, scores, classes):
            top, left, right, bottom = [int(_b) for _b in box]
            top = max(0, min(top, image.shape[1]))
            left = max(0, min(left, image.shape[0]))
            right = max(0, min(right, image.shape[1]))
            bottom = max(0, min(bottom, image.shape[0]))

            cv2.rectangle(image, (top, left), (right, bottom), (255, 0, 0), 2)

            if cls < len(class_names):
                label = f'{class_names[cls]} {score:.2f}'
                cv2.putText(image, label, (top, left - 6),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        return image
