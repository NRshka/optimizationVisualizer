# Very simple demonstration of how GD optimizer works with function approximation
## Installation
The project uses SFML for 2D visualization. The code is written with SFML 3, so you need to install it manually. I did it on Ubuntu with following code:

```bash
# Dependencies
sudo apt update
sudo apt install cmake libfreetype6-dev libxrandr-dev libxcursor-dev \
  libudev-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev \
  libglu1-mesa-dev libx11-dev

# Clone and build
git clone https://github.com/SFML/SFML.git
cd SFML
git checkout 3.0.2  # should work with any 3.x version but I tested with 3.0.2
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON  # cmake is configured to link dynamic libraries so we set the flag to build it
cmake --build build
sudo cmake --install build
sudo ldconfig  # update paths
```

You can ommit it if you already have SFML 3 configured. On some systems you may have installed SFML v2. Any attempt to build the project with SFML 2 will fall.
Build the project:

```bash
cmake -B build
```

Now you make tweak any code and have the app built by:

```bash
cmake --build build
```

All artifacts will be generated at the *build/* directory. Have fun!