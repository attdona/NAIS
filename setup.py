from setuptools import setup, find_packages

setup(
    name='pynais',
    version='0.1',
    packages=find_packages(),
    include_package_data=True,
    install_requires=[
        'Click',
        'protobuf',
        'websockets',
        'pyserial',
        'hbmqtt'
    ],
    extras_require={
        'dev': [
            'sphinx',
            'restructuredtext-lint',
            'pytest',
            'pytest-asyncio',
            'pytest-cov'
        ]
    },
    entry_points='''
        [distutils.commands]
            build_proto=pynais.scripts.build_proto:ProtoBuild
    ''',
)
