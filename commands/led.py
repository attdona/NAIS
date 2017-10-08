"""Custom command for managing on/off objects like leds
"""
RED_BIT = 0
YELLOW_BIT = 1
GREEN_BIT = 2


class Leds:
    """An array of leds: red, green and yellow
    """

    RED = 1 << RED_BIT
    YELLOW = 1 << YELLOW_BIT
    GREEN = 1 << GREEN_BIT

    def __init__(self, red=None, green=None, yellow=None):
        self.red = red
        self.green = green
        self.yellow = yellow

    def set_status(self, values):
        """Set led states from bit indexed ``values``
        """
        self.red = 'on' if (values & Leds.RED) else 'off'
        self.yellow = 'on' if (values & Leds.YELLOW) else 'off'
        self.green = 'on' if (values & Leds.GREEN) else 'off'

    def set_protobuf(self, obj):
        """Encode the object into a protobuf binary array
        """
        obj.id = 1

        mask = 0
        values = 0

        for (led, attr) in (
                (Leds.RED, self.red),
                (Leds.YELLOW, self.yellow),
                (Leds.GREEN, self.green)):
            if attr:
                mask |= led
                values |= led if attr == 'on' else 0

        # set the mash/values only if ther is at least one led switch request
        # if ther are no switch requests interprets the command as a get led
        # states request
        if mask:
            obj.ivals.append(mask)
            obj.ivals.append(values)

    def build_from_protobuf(self, obj):
        """used by software simulation of physical board
        """
        self.id = obj.id
        if len(obj.ivals) == 2:
            mask = obj.ivals[0]
            values = obj.ivals[1]

            if mask & Leds.RED:
                self.red = 'on' if (values & Leds.RED) else 'off'
            if mask & Leds.YELLOW:
                self.yellow = 'on' if (values & Leds.YELLOW) else 'off'
            if mask & Leds.GREEN:
                self.green = 'on' if (values & Leds.GREEN) else 'off'

        return self
