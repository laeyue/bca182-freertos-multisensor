# Wokwi live verification run — 2026-09-30

This run was performed in the Wokwi VS Code simulator. It records the live
observations made while driving the circuit through the simulator controls.
No screenshot was retained from this session.

## Test setup

- DHT22: 33.1 C and 66.0% humidity for the alarm check.
- PIR `delayTime`: temporarily set to 86,400 seconds because the Wokwi
  simulator fast-forwarded simulated time between manual UI actions.
- After the run, `diagram.json` was restored to DHT22 24 C / 40% and PIR
  `delayTime` 5 seconds.

## Observed results

- Serial output repeatedly showed `Sensor: sampling DHT22`,
  `Sensor: DHT start delay finished`, `Sensor: DHT22 read complete`,
  `Sensor: 33.1 C, 66.0 %`, `Sensor: light 76 % ADC 1001`, and
  `Motion: heartbeat PIR high`.
- While ACTIVE, clockwise encoder steps showed Temperature -> Humidity ->
  Light -> Motion -> Temperature. Counterclockwise steps showed Temperature
  -> Motion -> Light -> Humidity -> Temperature. The serial monitor showed
  `Input: display page changed` during navigation.
- At 33.1 C, the OLED Temperature page showed `ALARM`; Wokwi displayed the
  buzzer activity icon. The retained [PB8 alarm waveform](wokwi-buzzer-alarm-on.vcd)
  separately shows approximately 500 Hz PWM at the high-temperature setting,
  and the [recovery waveform](wokwi-buzzer-recovery.vcd) shows PB8 returning
  low after a normal-temperature sample.

These observations verify the simulated alarm path and both encoder page
sequences. They do not measure physical buzzer loudness or validate the circuit
on a physical board.
