import subprocess
import configuration
import os

def runSimulation():
    if configuration.CFG_TYPE == "BARE":
        execution_time_call = ["drrun", "-root", os.getenv("DynamoRIO_DIR"), "-c", "./tracer/build/libtracer.so"]
        vp_param = configuration.CFG_EXECUTION_RESULT + ":" + configuration.CFG_HARDWARE + ":" + ":".join(configuration.CFG_HARDWARE_PARAMETERS) + ":" + configuration.CFG_SOFTWARE
        vp_param = vp_param.split(":")
        execution_time_call = execution_time_call + [vp_param[0], "--"]
        execution_time_call = execution_time_call + vp_param[1:]
        print(" ".join(execution_time_call))
        subprocess.run(execution_time_call)