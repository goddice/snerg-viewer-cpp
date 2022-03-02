[![Windows](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Windows.yml/badge.svg?branch=main)](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Windows.yml)
[![MacOS](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/MacOS.yml/badge.svg?branch=main)](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/MacOS.yml)
[![Linux](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Linux.yml/badge.svg?branch=main)](https://github.com/goddice/snerg-viewer-cpp/actions/workflows/Linux.yml)

C++ implementation for the viewer of SNeRG ([Baking Neural Radiance Fields for Real-Time View-Synthesis](http://nerf.live)).

The third-party libraries are organized as git submodules. 
Therefore, be sure to use the `--recursive` flag when
cloning the repository:
```bash
$ git clone --recursive https://github.com/goddice/snerg-viewer-cpp.git
```

If you accidentally clone the repo without using ``--recursive``, run the
following command to also fetch the dependencies:
```bash
$ git submodule update --init --recursive
```

Steps of running the code:
```bash
$ python -m pip install --user -r requirements.txt
$ python scripts/download_data.py
$ cmake -S . -B build
$ cmake --build build --config Release
```

Run it (e.g.: ./build/app models/drums)

![screen shot](https://i.ibb.co/fkyv8Qr/snerg.png)

For the model training and the original webgl viewer, please refer to the official SNeRG repo: [https://github.com/google-research/google-research/tree/master/snerg](https://github.com/google-research/google-research/tree/master/snerg)

### TODO list
- [ ] Fix rotation issue
- [ ] Support resize
- [ ] Support Android
- [ ] Support iOS
- [ ] Support WebAssembly
