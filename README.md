pcb_etching_machine
===================


![My image](http://abload.de/img/img_20131203_202908_4bak7n.jpg)

This projekt was inspired by [this idea](http://avr-projekte.de/aetz2.htm) from Jürgen Woetzel

Though I wanted to play with servos and had some ideas I wanted to try so I wrote my own firmware and changed the hardware a little bit.
The result has some improvements and some drawbacks

It's powerded by two connectors from a computer power supply(NT in my case)
The second one is used for heating(12V) and servo(5V) because I had some issues with the sudden current changes

This machine is in use from time to time but I wouldn't say it's finished

> **Features:**
> 
> - Timebased etching/exposing a pcb
> - endless etching/exposing
> - Wait for etching until temp is okay
> - Change settings during etching(etching won't stop)
> - Solid connection between heater and container
> - able to store values in EEPROM
> - menu accessible trough rotary encoder
> - servo speed adjustable and increase/decrease on startup/end

> **Todo/Bugs:**
> 
> - If temperature sensor doesn't answer program resets(atm no timeout)
    watchdog resets and shows on display so no damage should occur
> - some code cleanup and add some comments
> - heating takes somewhat long but I think it's a problem with my power supply
> - PCB needs some improvements: connectors position, enable switch board for external exposer
> - Alarm needs to be tweaked and inclued in HW

> **Things I would change if I built it again**
> 
> - use one servo or let both face the same direction
> - make the aluminium heat plate thinner

There are two holes in the heat-plate and a plate sticked to the container for the actual PCB both 3mm thick
which has two threads with short M3 screws in it so it won't move( will append some images soon)

The other screws are mostly M4 but it depends mainly on the hinges and what you have lying arround

I used some of heat foil available at Reichelt [like this](http://www.reichelt.de/THF-65100/3/index.html?&ACTION=3&LA=446&ARTICLE=108461&artnr=THF-65100&SEARCH=heizung)

If interested I can put toghether a complete list of things needed
