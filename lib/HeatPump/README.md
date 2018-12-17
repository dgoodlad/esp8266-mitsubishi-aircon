# HeatPump

This library is vendored from
[SwiCago/HeatPump#6b64b6f](https://github.com/SwiCago/HeatPump/tree/6b64b6f).

## Changes

My only significant change is to allow the caller of `HeatPump::connect` to
cause the init code to call `Serial.swap()` to swap GPIO pins. I may open a pull
request for this change, but am unsure how broadly useful it is.
