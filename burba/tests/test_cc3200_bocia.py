"""bocia layer tests
"""
import os
import pynais.iottest as iot
import pynais as ns
import tc

EVENTS = (
    ('IP acquired', 'IP_ACQUIRED'),
    ("([\d]+) Tests ([\d]+) Failures ([\d]+) Ignored", 'TEST_RESULT'),
    ('expected connection failed', 'CONN_FAILED_OK'),
    ('unexpected connection failed', 'CONN_FAILED_ERR'),
    ('board ready', 'BOARD_READY'),
    ('connection success', 'TEST_OK')
)


def handle_event(event, uart):
    """react to board events
    """

    if(event.name == 'CONN_FAILED_OK'):
        # configure the board with the bocia settings
        host_ip = ns.my_ip_address()
        cfg = ns.msg.Config(network=tc.network, board=tc.board, host=host_ip)
        uart.write(ns.marshall(cfg))

    if event.name == 'CONN_FAILED_ERR':
        return 0
    
    if event.name == 'TEST_OK':
        return 1

    if event.name == 'IP_ACQUIRED':
        return 1

    if (event.name == 'BOARD_READY'):
        # WLAN uid/password setup
        profile = ns.msg.Profile(
            os.environ['TEST_UID'], pwd=os.environ['TEST_PWD'])
        uart.write(ns.marshall(profile))

    if (event.name == 'TEST_RESULT'):
        print("event received: {} - arg1: {}".format(event.name, event.arg1))
        # arg2 contains the number of test failures
        #assert(event.arg2 == 1)

    if (event.name == 'PROTOBUF'):
        print("handle_event: {}".format(event.obj))

        # time to end successfully the test
        # return 1

    if (event.name == 'SERIAL_TIMEOUT'):
        assert(0)


def test_init_board():
    """compile, flash register events map and start event loop 
    """
    tc.check_test_requirements()

    iot.run('bocia', tc.fw_deploy_ctx, events=EVENTS, handler=handle_event, timeout=20)

