import asyncio

def cancel_tasks():
    """Cancel background tasks to reset environment between test cases
    """
    pendings = asyncio.Task.all_tasks()

    ct = asyncio.Task.current_task()

    for task in pendings:
        
        if (task != ct):
            task.cancel()

