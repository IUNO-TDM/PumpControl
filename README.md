[![Build Status](https://travis-ci.org/IUNO-TDM/PumpControl.svg?branch=master)](https://travis-ci.org/IUNO-TDM/PumpControl)
# PumpControl
An executable for decrypting and running recipe programs on the cocktail mixer

See https://github.com/IUNO-TDM/MixerControl/blob/master/Setup.md for further information.

## prerequisites

```$ sudo apt-get install libboost-all-dev libssl-dev```

## compile without encryption

```$ make ENCRYPTION=off```


## recipe format

The format of the recipe:
```json
Program {
  "id":	"string",
  "lines": [
    {
      "components": [
        {
          "ingredient":	"string",
          "amount":	"number: <the amount in ml"
        }
      ],
      "timing":	"number: 0: machine can decide, 1: all ingredients as fast as possible, 2: all beginning as early as possible and end with the slowest together, 3: one after the other",
      "sleep":	"number: <the sleep time after the line in ms>"
     }
  ]
}
```
