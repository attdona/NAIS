import pynais as ns
from .led import Leds

LEDS = 10

ns.setCommand(id=LEDS, model=Leds)
