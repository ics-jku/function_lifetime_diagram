import sys
import time

import configuration
import fileStructure
import codeStructure
import addressTranslation
import simulator
#import generateGraph
import generateFTRFile

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage" + sys.argv[0] + " <configuration_file>")
        sys.exit(1)

    print("Start reading configuration ...")
    start_time=time.time()
    configuration.parseConfig(sys.argv[1])
    print("... Finished reading configuration: " + str(time.time()-start_time))

    print("Start static code analysis ...")
    print("   Start analysing file structure ...")
    start_time = time.time()
    fileStructure.parseFileStructure()
    print("   ... Finished analysing file structure: " + str(time.time()-start_time))
    print("   Start analysing source code strucutre ...")
    sub_start_time = time.time()
    codeStructure.parseCodeStructure()
    print("   ... Finished analysing source code structure: " + str(time.time()-sub_start_time))
    print("... Finished static code analysis: " + str(time.time()-start_time))

    print("Start address translation ...")
    start_time = time.time()
    addressTranslation.generateAddressTable()
    print("... Finished address translation: " + str(time.time()-start_time))

    print("Start simulation ...")
    start_time = time.time()
    simulator.runSimulation()
    print("... Finished simulation: " + str(time.time()-start_time))

    # print("Start reading results and generating graph ...")
    # start_time = time.time()
    # generateGraph.generate()
    # print("... Finished reading results and generating graph: " + str(time.time()-start_time))

    print("Start reading results and generating FTR file ...")
    start_time = time.time()
    generateFTRFile.generate()
    print("... Finished reading results and generating FTR file: " + str(time.time()-start_time))
