Zephyr Project
##############

This is a simple utility meter project inspired by GitHub user wulffern's `nrf52-utilitymeter`_
project, based on the `Zephyr Project RTOS`_, version 1.9.1. It is a modification of the Zephyr
project's peripheral_hr example, without all the heartrate stuff.

The project will log your power consumption over time, and advertise the current and recent power
consumption.  Features are constantly changing :)

To compile for nRF51 on PCA10028, just run::

  make -C powermeter

You will have to have set up the Zephyr project's SDK, see Resources below.

Disclaimer
**********
This is a quick and dirty hobby project. The code quality may reflect that :)

Hardware setup
**************
Phototransistor e.g. TEPT 5600, between ground and pin P0.01 on a Nordic Semiconductor PCA10028 dev
kit. Aim the phototransistor at the blinking diode on your utility meter.

Resources
*********

Here's a quick summary of resources to find your way around the Zephyr Project
support systems:

* **Zephyr Project Website**: The https://zephyrproject.org website is the
  central source of information about the Zephyr Project. On this site, you'll
  find background and current information about the project as well as all the
  relevant links to project material.  For a quick start, refer to the
  `Zephyr Introduction`_ and `Getting Started Guide`_.

.. _Zephyr Project RTOS: https://github.com/zephyrproject-rtos/zephyr
.. _nrf52-utilitymeter: https://github.com/wulffern/nrf52-utilitymeter
.. _Zephyr Introduction: https://www.zephyrproject.org/doc/introduction/introducing_zephyr.html
.. _Getting Started Guide: https://www.zephyrproject.org/doc/getting_started/getting_started.html
