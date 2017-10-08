import os
import sys
import subprocess
import glob
import serial
import re
import logging
from pynais import SYNC_START_B, SYNC_END_B
import pynais as ns

logging.basicConfig(
    level=logging.DEBUG,
    format='%(name)s: %(message)s',
    stream=sys.stderr
)

LOG = logging.getLogger(__name__)


class SerialReadTimeout(Exception):
    pass


class IOTEvent:

    def __init__(self, name, obj=None):
        self.name = name
        self.obj = obj

    def match(self):
        return self.name != 'LOG'


cfg = {}

A_STREAM = 0  # ascii stream
B_STREAM = 1  # binary stream


def event_from_board(line):
    try:
        if (ns.is_protobuf(line)):
            ev = IOTEvent('PROTOBUF')
            ev.obj = ns.unmarshall(line)
            return ev

        sline = line.decode('utf-8')
        for (pattern, ev_name) in cfg['EVENTS']:
            match = re.match(pattern, sline)
            if (match):
                ev = IOTEvent(ev_name)
                ev.obj = line
                for (index, arg) in enumerate(match.groups(), start=1):
                    setattr(ev, 'arg{}'.format(index), arg)

                return ev
    except UnicodeDecodeError:
        LOG.debug("skipping line with non ascii chars: ", line)

    return IOTEvent('LOG', line)


def init(config):
    cfg.update(config)


def msg_receive(reader):
    """Return a message from a UART channel

        Detect the type of message, currently protobuf encoded or ascii
        strings \n terminated.

    """
    sts = A_STREAM
    msg = bytearray()
    ch = reader.read(1)
    if (ch == b''):
        raise SerialReadTimeout

    while (ch):
        # LOG.debug("read |%r|", ch)
        msg += ch

        if ch == SYNC_START_B:
            sts = B_STREAM
        elif ch == SYNC_END_B and sts == B_STREAM:
            sts = A_STREAM
            break
        elif sts == A_STREAM and ch == b'\n':
            break

        ch = reader.read(1)
        if (ch == b''):
            raise SerialReadTimeout

    return msg


def print_event(ev):
    if ev.name == 'PROTOBUF':
        print("PROTO >> {}".format(ev.obj))

    else:
        try:
            print("{} >> {}".format(ev.name, ev.obj.decode('utf-8')), end='')
        except UnicodeDecodeError:
            print("RAW >> {}".format(ev.obj))
        except AttributeError:
            print("{}".format(ev.name))


def check_test_output(test_success_line=b'OK\n', test_fail_line=b'FAIL\n'):

    read_serial = True
    with serial.Serial(cfg['SERIAL_PORT'], 115200, timeout=cfg['SERIAL_PORT_TIMEOUT']) as ser:
        try:
            while read_serial:
                line = msg_receive(ser)

                ev = event_from_board(line)
                print_event(ev)

                if(ev.match()):
                    rc = cfg['HANDLER'](ev, ser)
                    if (not rc is None):
                        return rc

                if (line == test_success_line):
                    return True
                elif (line == test_fail_line):
                    return False
        except SerialReadTimeout:
            LOG.debug("serial read timeout, exiting test")
            cfg['HANDLER'](IOTEvent('SERIAL_TIMEOUT'), ser)


def run(test_suite, context, events=None, handler=None, timeout=None):
    """Compile, flash and run the image

        If events==None compile and flash without checking the output
    """
    init(context)

    cfg['EVENTS'] = events
    cfg['HANDLER'] = handler


    if timeout:
        cfg['SERIAL_PORT_TIMEOUT'] = timeout

    scriptdir = os.path.dirname(os.path.abspath(__file__))

    tdir = os.path.join(cfg['ROOT_TEST_DIR'], test_suite)

    completed = subprocess.run(['make', 'flash'], cwd=tdir)
    if (completed.returncode == 0):
        # capture the test output
        rc = check_test_output()

        assert rc


        # # build successfull
        # for fn in glob.iglob(os.path.join(tdir, '**/*.bin'), recursive=True):
        #     # PATH must resolve flash.sh script
        #     completed = subprocess.run(
        #         ['flash.sh', cfg['ENERGIA_ROOT'], cfg['SERIAL_PORT'], os.path.abspath(fn)])
        #     print(completed.returncode)
        #     if (completed.returncode == 0):
        #         rc = 1
        #         if events:
        #             # capture the test output
        #             rc = check_test_output()

        #         assert rc
    else:
        # build failed
        assert 0, "build/flash failed"
