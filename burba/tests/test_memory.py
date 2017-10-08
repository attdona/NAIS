"""simple test demostrating the test machinery: test dynamic memory allocation
"""
import functools

# the firmware test machinery
import pynais.iottest as iot

# import firmware configuration
from tc import fw_deploy_ctx

EVENTS = (
    ('pointer_addr: ([x0-9a-f]+)', 'POINTER_ADDR'),
)


def handle_event(obj, event, _):
    """Check that memory address has a constant value after each malloc/free cycle
    """
    if (event.name == 'POINTER_ADDR'):
        print("value:", event.arg1)
        if 'addr_value' in obj:
            assert obj['addr_value'] == event.arg1
        else:
            obj['addr_value'] = event.arg1


def test_malloc_and_free():
    """start memory test
    """

    obj = {}

    handler = functools.partial(handle_event, obj)
    iot.run('memory', fw_deploy_ctx, events=EVENTS, handler=handler)
