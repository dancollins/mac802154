# WSN
A wireless sensor network platform designed for IEEE 802.15.4 radios.

## Supported Hardware
* CC2538DK

## Important Note
This is not a functional implementation, as it is lacking an operating system!
There is an OS abstraction layer in lib/src/os/ws_os.h that needs to be ported
in order for things to work. At some stage, I hope to write a minimal OS that
can demonstrate this platform working.
