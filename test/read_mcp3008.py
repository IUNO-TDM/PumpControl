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

# Einleseschleife
while True:
  # Lese ADC-Kanal 0
  adcval = readadc(0)
  # Wert ausgeben 
  print '{0:4d}, {1:4d}, {2:4d}, {3:4d}, {4:4d}, {5:4d}, {6:4d}, {7:4d}'.format(adcval, readadc(1), readadc(2), readadc(3), readadc(4), readadc(5), readadc(6), readadc(7))

#  print "ADC1 = ", readadc(1)
  # etwas warten
  time.sleep(1)

