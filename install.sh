#!/bin/bash

cd thirdparty
git clone https://github.com/opencv/opencv.git
cd opencv
git checkout tags/4.5.5
mkdir -p build

cmake .. -DBUILD_JAVA=OFF \
-DBUILD_PER_TESTS=OFF \
-DBUILD_PROTOBUF=OFF \
-DBUILD_TESTS=OFF \
-DBUILD_opencv_apps=OFF \
-DBUILD_opencv_calib3d=OFF \
-DBUILD_opencv_core=ON \
-DBUILD_opencv_dnn=OFF \
-DBUILD_opencv_features2d=OFF \
-DBUILD_opencv_flann=OFF \
-DBUILD_opencv_gapi=OFF \
-DBUILD_opencv_imgcodecs=OFF \
-DBUILD_opencv_imgproc=OFF \
-DBUILD_opencv_java_bindings_generator=OFF \
-DBUILD_opencv_js_bindings_generator=OFF \
-DBUILD_opencv_ml=OFF \
-DBUILD_opencv_objc_bindings_generator=OFF \
-DBUILD_opencv_objdetect=OFF \
-DBUILD_opencv_photo=OFF \
-DBUILD_opencv_python3=OFF \
-DBUILD_opencv_python_bindings_generator=OFF \
-DBUILD_opencv_python_tests=OFF \
-DBUILD_opencv_stitching=OFF \
-DBUILD_opencv_ts=OFF