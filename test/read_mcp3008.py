#!/usr/bin/python
import spidev
import time

# SPI-Instance erzeugen und den Bus oeffen
spi = spidev.SpiDev()
spi.open(1,0)
spi.max_speed_hz = 1600000

def readadc(channel = 0):
  adc = spi.xfer2([1, (8 + channel) << 4, 0])
  data = ((adc[1] & 3) << 8) + adc[2]
  return data

adcval = [0]*8
state  = [0]*8

# Einleseschleife
while True:
    for i in range(8):
        adcval[i] = readadc(i)
        state[i] = '#' if adcval[i] > 3 else '-'

    # Wert ausgeben
    print '{0:4d}, {1:4d}, {2:4d}, {3:4d}, {4:4d}, {5:4d}, {6:4d}, {7:4d}'.format(adcval[0], adcval[1], adcval[2], adcval[3], adcval[4], adcval[5], adcval[6], adcval[7]), ' | ',
    print '{0:3s} {1:3s} {2:3s} {3:3s} {4:3s} {5:3s} {6:3s} {7:3s}'.format(state[0], state[1], state[2], state[3], state[4], state[5], state[6], state[7])

    # etwas warten
    time.sleep(1)

