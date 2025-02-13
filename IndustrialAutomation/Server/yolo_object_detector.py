"""
YOLO Object Detection Model

This script defines the YOLO_v3 deep learning model for object detection in images and videos.
It provides functionalities to:

1. **Load a YOLO model** with specified weights, anchors, and class definitions.
2. **Perform object detection** on images and videos using bounding boxes.
3. **Process detected objects** by computing their locations and distances.
4. **Calibrate detected coordinates** for robot positioning.
5. **Save detection results** for further use.

This script is essential for applications requiring real-time object detection and coordinate extraction.
"""

import colorsys
import os
from timeit import default_timer as timer
import numpy as np
from keras import backend as K
from keras.models import load_model
from keras.layers import Input
from PIL import Image, ImageFont, ImageDraw
from yolo3.model import yolo_eval, yolo_body, tiny_yolo_body
from yolo3.utils import letterbox_image
from keras.utils import multi_gpu_model
from math import pow, sqrt

class YOLO:
    """
    Implements the YOLO object detection model with functionalities for real-time detection.
    """
    _defaults = {
        "model_path": 'model_data/new_triangulo_fine3.h5',
        "anchors_path": 'model_data/yolo_anchors.txt',
        "classes_path": 'model_data/my_classes.txt',
        "score": 0.3,
        "iou": 0.45,
        "model_image_size": (640, 480),
        "gpu_num": 1,
    }

    @classmethod
    def get_defaults(cls, n):
        return cls._defaults.get(n, f"Unrecognized attribute name '{n}'")

    def __init__(self, **kwargs):
        self.__dict__.update(self._defaults)  # Set up default values
        self.__dict__.update(kwargs)  # Update with user overrides
        self.class_names = self._get_class()
        self.anchors = self._get_anchors()
        self.sess = K.get_session()
        self.boxes, self.scores, self.classes = self.generate()

    def _get_class(self):
        """Load class names from the provided file."""
        with open(os.path.expanduser(self.classes_path)) as f:
            class_names = [c.strip() for c in f.readlines()]
        return class_names

    def _get_anchors(self):
        """Load anchor points from the provided file."""
        with open(os.path.expanduser(self.anchors_path)) as f:
            anchors = [float(x) for x in f.readline().split(',')]
        return np.array(anchors).reshape(-1, 2)

    def generate(self):
        """Load and initialize the YOLO model."""
        model_path = os.path.expanduser(self.model_path)
        assert model_path.endswith('.h5'), 'Model must be a .h5 file.'
        num_anchors = len(self.anchors)
        num_classes = len(self.class_names)
        is_tiny_version = num_anchors == 6

        try:
            self.yolo_model = load_model(model_path, compile=False)
        except:
            self.yolo_model = tiny_yolo_body(Input(shape=(None, None, 3)), num_anchors // 2, num_classes) if is_tiny_version else yolo_body(Input(shape=(None, None, 3)), num_anchors // 3, num_classes)
            self.yolo_model.load_weights(self.model_path)

        print(f'Model {model_path} loaded with anchors and classes.')
        self.input_image_shape = K.placeholder(shape=(2,))
        if self.gpu_num >= 2:
            self.yolo_model = multi_gpu_model(self.yolo_model, gpus=self.gpu_num)

        return yolo_eval(self.yolo_model.output, self.anchors, len(self.class_names), self.input_image_shape, score_threshold=self.score, iou_threshold=self.iou)

    def detect_image(self, image):
        """Detect objects in an image using YOLO."""
        start = timer()
        boxed_image = letterbox_image(image, tuple(reversed(self.model_image_size))) if self.model_image_size != (None, None) else image
        image_data = np.expand_dims(np.array(boxed_image, dtype='float32') / 255., 0)
        out_boxes, out_scores, out_classes = self.sess.run([self.boxes, self.scores, self.classes], feed_dict={
            self.yolo_model.input: image_data,
            self.input_image_shape: [image.size[1], image.size[0]],
            K.learning_phase(): 0
        })

        print(f'Found {len(out_boxes)} objects')
        draw = ImageDraw.Draw(image)
        for i, c in enumerate(out_classes):
            predicted_class = self.class_names[c]
            box = out_boxes[i]
            score = out_scores[i]
            label = f'{predicted_class} {score:.2f}'
            draw.rectangle([box[1], box[0], box[3], box[2]], outline=(255, 0, 0))
            draw.text((box[1], box[0]), label, fill=(255, 255, 255))
        print(f'Processing time: {timer() - start:.2f}s')
        return image

    def close_session(self):
        """Close the current YOLO session."""
        self.sess.close()


def detect_video(yolo, video_path, output_path=""):
    """Perform object detection on a video file."""
    import cv2
    vid = cv2.VideoCapture(video_path)
    if not vid.isOpened():
        raise IOError("Could not open video file")
    video_fps = vid.get(cv2.CAP_PROP_FPS)
    while True:
        ret, frame = vid.read()
        if not ret:
            break
        image = Image.fromarray(frame)
        result = np.asarray(yolo.detect_image(image))
        cv2.imshow("YOLO Detection", result)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    yolo.close_session()
    vid.release()
    cv2.destroyAllWindows()
