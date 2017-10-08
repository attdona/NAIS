"""global test configuration
"""
import os
import asyncio

#broker_url
broker='mqtt://localhost:1883'

# board bocia configuration
network='bolleri'
board='living'

# setting for building (compile and flash)
fw_deploy_ctx = {
    'ROOT_TEST_DIR': os.path.dirname(os.path.abspath(__file__)),
    'SERIAL_PORT': os.getenv('PORT', '/dev/ttyUSB0'),
    'SERIAL_PORT_TIMEOUT': 5,
    'ENERGIA_ROOT': os.getenv('ENERGIA_ROOT', '/opt/energia-0101E0017')
}

def check_test_requirements():
    """required test values
    """
    assert 'TEST_UID' in os.environ, "unset env variable TEST_UID"
    assert 'TEST_PWD' in os.environ, "unset env variable TEST_PWD"

def cancel_tasks():
    """Cancel background tasks to reset environment between test cases
    """
    pendings = asyncio.Task.all_tasks()

    current_task = asyncio.Task.current_task()
    for task in pendings:
        
        if (task != current_task):
            task.cancel()
