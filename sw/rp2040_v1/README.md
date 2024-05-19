
# M12 interposer RP2040 firmware.

This first version parsers all the commands that are going from the module
to the S200 and adjust the date by adding 1024 weeks.

It works somewhat, but the date calculation seems wrong (TBD) and
when used with a Furuno GT-8031, it still doesn't make the S200 lock to the
GPS signal.

This version requires commands to arrive fully before they can be patched and
retransmitted. A new version can patch messages as needed on-the-fly.

To build:

* Set the `PICO_SDK_PATH` to your local clone of the [Raspberry pico SDK](https://github.com/raspberrypi/pico-sdk).

    E.g. `export PICO_SDK_PATH=/home/tom/projects/pico-sdk`

* Create `build` directory under the `rp2040_v1` directory
* Build:

```sh
cd build
cmake ..
make
```
    
