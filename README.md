
* Install PICO SDK

```
export PICO_SDK_PATH="/home/tom/projects/pico-sdk"
```

For DSView:
```
sudo apt install -y qtcreator qtbase5-dev qt5-qmake cmake
```

```
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
cd ~/projects
git clone git@github.com:raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init --recursive
```
