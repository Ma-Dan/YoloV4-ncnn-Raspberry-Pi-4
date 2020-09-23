FROM ubuntu:18.04

#Install tools
RUN apt update && \
    apt install -y wget unzip gcc g++ make cmake libjpeg-dev git libprotobuf-dev protobuf-compiler build-essential libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev python3-pip && \
    pip3 install Cython && \
    pip3 install numpy && \
    apt clean

#Install ncnn
RUN cd / && \
    git clone https://github.com/Tencent/ncnn && \
    cd ncnn && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/aarch64-linux-gnu.toolchain.cmake .. && \
    make -j $(nproc) && \
    make install && \
    mkdir /usr/local/lib/ncnn && \
    cp -r install/include/ncnn /usr/local/include/ncnn && \
    cp -r install/lib/libncnn.a /usr/local/lib/ncnn/libncnn.a && \
    rm -r /ncnn

#Install OpenCV
RUN cd / && \
    pip3 install Cython && \
    pip3 install numpy && \
    wget -O opencv.zip https://github.com/opencv/opencv/archive/4.4.0.zip && \
    wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/4.4.0.zip && \
    unzip opencv.zip && \
    unzip opencv_contrib.zip && \
    mv opencv-4.4.0 opencv && \
    mv opencv_contrib-4.4.0 opencv_contrib && \
    cd /opencv/ && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D OPENCV_EXTRA_MODULES_PATH=/opencv_contrib/modules -D ENABLE_NEON=ON -D WITH_FFMPEG=ON -D WITH_GSTREAMER=ON -D WITH_TBB=ON -D BUILD_TBB=ON -D BUILD_TESTS=OFF -D WITH_EIGEN=OFF -D WITH_V4L=ON -D WITH_LIBV4L=ON -D WITH_VTK=OFF -D WITH_QT=OFF -D OPENCV_ENABLE_NONFREE=ON -D INSTALL_C_EXAMPLES=OFF -D INSTALL_PYTHON_EXAMPLES=OFF -D BUILD_NEW_PYTHON_SUPPORT=OFF -D BUILD_opencv_python3=FALSE -D OPENCV_GENERATE_PKGCONFIG=ON -D BUILD_EXAMPLES=OFF .. && \
    make -j $(nproc) && \
    make install && \
    ldconfig && \
    apt-get update && \
    rm -r /opencv /opencv_contrib /opencv.zip /opencv_contrib.zip

#Install Yolov4 video
RUN cd / && \
    git clone https://github.com/Ma-Dan/YoloV4-ncnn-Raspberry-Pi-4 && \
    cd YoloV4-ncnn-Raspberry-Pi-4 && \
    make video -j $(nproc)

EXPOSE 7777
