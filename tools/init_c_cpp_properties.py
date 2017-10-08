import click
import re
import json
import os
import platform
import subprocess

WORKSPACE_ROOT = "${workspaceRoot}"

# the vscode workspace root dir
NAIS_ROOT = os.environ['NAIS']

gcc_version = subprocess.check_output(['arm-none-eabi-gcc', '-dumpversion'])
gcc_version = gcc_version.decode('utf-8')[:-1]

# Linux paths for arm compiler
# To be tested for Windows and MAC
system_paths = ['/usr/arm-none-eabi/include/',
                '/usr/lib/gcc/arm-none-eabi/{}/include/'.format(gcc_version)]

CPP_CONFIG_FILENAME = os.path.join(
    NAIS_ROOT, '.vscode', 'c_cpp_properties.json')

PLATFORM_NAME = platform.system()


@click.command()
@click.option("--compiler_output", default="make.log")
def main(compiler_output):
    """Configure c_cpp_properties.json for RIOT
    """

    default_config = {}

    with open(compiler_output) as f:
        lines = f.read()

    riotbuilds=set(re.findall("-include[\s]+'([\S]+)'", lines))
    formatted_defs = []
    for include in riotbuilds:
        with open(include) as f:
            defines = re.findall("#define[\s]+([\S]+)[\s]+([\S]+)", f.read())
            formatted_defs += ["{}={}".format(el[0],el[1]) for el in defines]

    default_config['configurations'] = [{
        'name': PLATFORM_NAME,
        'defines': formatted_defs
    }]


    isystem = set(re.findall("-isystem[\s]+([\S]+)", lines))

    includes = set(re.findall("-I([\S]+)", lines))

    paths = []
    for path in includes:
        paths.append(re.sub(os.environ['NAIS'], WORKSPACE_ROOT, path))

    for config in default_config['configurations']:
        if config['name'] == PLATFORM_NAME:
            config['includePath'] = system_paths + list(isystem) + paths

    # write back the configuration
    with open(CPP_CONFIG_FILENAME, 'w') as f:
        f.write(json.dumps(default_config))


if __name__ == "__main__":
    main()
