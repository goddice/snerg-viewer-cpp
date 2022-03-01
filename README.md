C++ implementation for the viewer of SNeRG ([Baking Neural Radiance Fields for Real-Time View-Synthesis](http://nerf.live)).

Steps of running the code:
1. python download download_data.py
2. mkdir build
3. cd build
4. cmake ../
5. make -j8

Run it (e.g.: ./app ../models/hotdog)

For the model training and the original webgl viewer, please refer to the official SNeRG repo: [https://github.com/google-research/google-research/tree/master/snerg](https://github.com/google-research/google-research/tree/master/snerg)
