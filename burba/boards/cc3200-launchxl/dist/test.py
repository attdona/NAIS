#!/usr/bin/env python

import subprocess
import os
import serial
import glob
import click

ENERGIA_ROOT = '/opt/energia-0101E0017'
PORT = '/dev/ttyUSB0'

currdir = os.path.dirname(os.path.abspath(__file__))
try:
    test_dir = os.path.join(os.environ['NAIS'], 'burba', 'tests')
except KeyError:
    test_dir = os.path.join(os.getcwd(), 'burba', 'tests')

def find_images(from_path):
    for filename in glob.iglob(os.path.join(from_path, '**/*.bin'), recursive=True):
        print(filename)


def check_test_output():

    read_serial = True
    with serial.Serial(PORT, 115200) as ser:
        while read_serial:
            line = ser.readline()   # read a '\n' terminated line
            try:
                print(">>", line.decode('utf-8'), end='')
            except UnicodeDecodeError:
                print(">>", line)
            if (line == b'OK\n'):
                break

@click.command()
@click.option('--tdir', default=test_dir, help="test directory with a Makefile")
def main(tdir):

    flash_and_run(tdir)
    # check_test_output()


def flash_and_run(from_path):
    scriptdir = os.path.dirname(os.path.abspath(__file__))
    #print(scriptdir)

    makefiles = glob.glob(os.path.join(from_path, '**/Makefile'), recursive=True)
    for makefile in makefiles:
        # build
        completed = subprocess.run('make', cwd=os.path.dirname(makefile))
        if (completed.returncode == 0):
            # build successfull
            for fn in glob.iglob(os.path.join(from_path, '**/*.bin'), recursive=True):

                completed = subprocess.run(
                    [os.path.join(scriptdir, 'flash.sh'), ENERGIA_ROOT, PORT, os.path.abspath(fn)])

                if (completed.returncode == 0):
                    # capture the test output
                    check_test_output()


if __name__ == "__main__":
    main()
