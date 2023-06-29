#Modified fso_Open 3.7.2<br />
<br />
<br />
This version has backported some fixes and improvements, like fs2 demo support, wav playback fix, lua crashes, model alignment. This is the only version that will run on Raspberry PI 3/4 and RK3588 devices like Orange PI5 via Panfork due to the minimum OpenGL version support needed of the newer versions.
<br />
**How to compile on Ubuntu 22.04 AArch64**<br />
<br />
sudo apt-get install libopenal-dev libjansson-dev libjpeg-dev liblua5.1-dev libogg-dev libpng-dev libsdl1.2-dev libtheora-dev libvorbis-dev automake libswscale-dev libavcodec-dev libavformat-dev<br />
git clone https://github.com/Shivansps/fs2open.github.com<br />
cd fs2open.github.com<br />
./autogen.sh<br />
make -j4<br />