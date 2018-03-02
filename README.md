[![Build Status](https://travis-ci.org/IUNO-TDM/PumpControl.svg?branch=master)](https://travis-ci.org/IUNO-TDM/PumpControl)
# PumpControl
An executable for decrypting and running recipe programs on the cocktail mixer

See https://github.com/IUNO-TDM/MixerControl/blob/master/Setup.md for further information.

**Table of contents:**

<!-- TOC depthFrom:2 depthTo:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [Development](#development)
	- [Prerequisites](#prerequisites)
	- [Compile Flags](#compile-flags)
	- [Recipe Format](#recipe-format)
- [Deploy with Docker](#deploy-with-docker)
	- [Build the image](#build-the-image)
	- [Usage](#usage)

<!-- /TOC -->

---

## Development
### Prerequisites
```
$ sudo apt-get install libboost-all-dev libssl-dev
```

### Compile Flags
Use provided compile flags to control the way the PumpControl program is compiled. Flags can be set like:
```
$ make FLAG=value ...
```

Flag        | Description
------------|----------------------------------------
ENCRYPTION  | ? (**on**, off)
REALDRIVERS | Include real pump drivers (**on**, off)

### Recipe Format
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

## Deploy with Docker
### Build the image
Using *Docker Compose* the image can be simply build by running the following command inside the root of this repository:
```
docker-compose build
```

### Usage
After the Docker Image has been built a Container can be deployed. Beforehand a configuration file need to be created and placed in `/etc/pumpcontrol/pumpcontrol.conf` of the host filesystem (see the `docker-compose.yml`). The configuration is then mounted as an Volume inside the container. To actually start the container run:
```
docker-compose up -d
```
