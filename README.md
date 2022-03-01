[![Windows](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Windows.yml/badge.svg?branch=main)](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Windows.yml)
[![MacOS](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/MacOS.yml/badge.svg?branch=main)](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/MacOS.yml)
[![Linux](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Linux.yml/badge.svg?branch=main)](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Linux.yml)

C++ implementation for the viewer of SNeRG ([Baking Neural Radiance Fields for Real-Time View-Synthesis](http://nerf.live)).

Steps of running the code:
1. python download download_data.py
2. cmake -S . -B build
3. cmake --build build --config Release

Run it (e.g.: ./build/app models/drums)

![screen shot](https://i.ibb.co/fkyv8Qr/snerg.png)


For the model training and the original webgl viewer, please refer to the official SNeRG repo: [https://github.com/google-research/google-research/tree/master/snerg](https://github.com/google-research/google-research/tree/master/snerg)
