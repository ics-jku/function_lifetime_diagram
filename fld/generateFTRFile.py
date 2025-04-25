import configuration
#import addressTranslation
import pickle

from ftr_writer import FTRWriter

LATEST = 0
EARLIEST = 0xFFFFFFFF

CONTENT = []

METHODS = []

# CALLER = [0, 0, 0]

def commonPath(files):
    if not files:
        return ""
    
    files.sort()

    first = files[0]
    last = files[-1]
    i = 0
    while i < len(first) and i < len(last) and first[i] == last[i]:
        i += 1

    return first[:i]

def generateDictionary():
    global CONTENT
    paths = []
    for method in METHODS:
        found = False
        for entry in CONTENT:
            #print(entry[1] + ":" + method[1])
            if entry[1] == method[1]:
                found = True
        if found == False:
            CONTENT.append([method[0], method[1], []])
            paths.append(method[1])

    for method in METHODS:
        for fileIdx, entry in enumerate(CONTENT):
            if entry[1] == method[1]:
                found = False
                for fileEntry in entry[2]:
                    
                    if fileEntry == method[0]:
                        found = True
                if found == False:
                    CONTENT[fileIdx][2].append([method[2], method[3], method[4], []])   

    toReplace = commonPath(paths)
    for fileIdx, file in enumerate(CONTENT):
        CONTENT[fileIdx][1] = CONTENT[fileIdx][1].replace(toReplace, "").replace("/", ".")
    

def addToContent(pc_type, pc, pc_timestamp):
    global CONTENT
    #global CALLER
    for fileIdx, file in enumerate(CONTENT):
        if file[0] == pc_type:
            for methodIdx, method in enumerate(file[2]):
                if method[1] == pc:
                    CONTENT[fileIdx][2][methodIdx][3].append([pc_timestamp, None])
                    #CALLER = [fileIdx, methodIdx, len(CONTENT[fileIdx][2][methodIdx][3]) - 1]  
                for methodEnd in method[2]:
                    if methodEnd == pc:
                        CONTENT[fileIdx][2][methodIdx][3][-1][1] = pc_timestamp
                        #CALLER = [fileIdx, methodIdx, len(CONTENT[fileIdx][2][methodIdx][3]) - 1]  
                              

def readTracerResults():
    global LATEST
    global EARLIEST
    #global RELATIONS
    
    #last_pc = 0
    with open(configuration.CFG_EXECUTION_RESULT, "rb") as results:
        while(1):
            pc_timestamp = int.from_bytes(results.read(8), "little")
            pc_type = int.from_bytes(results.read(4), "little")
            pc = int.from_bytes(results.read(4), "little")
            if pc_type == 0 and pc == 0:
                break
            if LATEST < pc_timestamp:
                LATEST = pc_timestamp
            if EARLIEST > pc_timestamp:
                EARLIEST = pc_timestamp            
            addToContent(pc_type, pc, pc_timestamp)
            #RELATIONS.append([last_pc, pc])
            #last_pc = pc
    #print(EARLIEST)
    
def getMinMaxFileTime(file):
    min = 0xFFFFFFFF
    max = 0
    for method in file[2]:
        for executions in method[3]:
            if executions[0] < min:
                min = executions[0]
            if executions[1] != None and executions[1] > max:
                max = executions[1]
    if max == 0:
        max = LATEST
    return min,max
            
        

def generateFTRFile():
    global CONTENT
    #global RELATIONS
    STREAMS = []
    GENERATORS = []
    ftrWriter = FTRWriter()
    ftrWriter.GenerateInfoChunk(-6)
    
    ftrWriter.AddToDictionary("file")
    ftrWriter.AddToDictionary("call")
    ftrWriter.AddToDictionary("return")
    for file in CONTENT:
        ftrWriter.AddToDictionary(file[1])
        STREAMS.append([file[1], ftrWriter.AddToStreams(file[1], "file")])
        for method in file[2]:
            ftrWriter.AddToDictionary(method[0])
    ftrWriter.GenerateDictionaryChunk()
    
    for file in CONTENT:
        for method in file[2]:
            ftrWriter.AddToDictionary(method[0])
            for stream in STREAMS:
                if stream[0] == file[1]:
                    GENERATORS.append([method[0], ftrWriter.AddToGenerators(method[0], stream[1])])
    ftrWriter.GenerateDirectoryChunk()

    #TRANSACTIONS = []
    for fileIdx,file in enumerate(CONTENT):
        #print("Streams: ")
        for stream in STREAMS:
            #print(stream)
            if stream[0] == file[1]:
                min,max = getMinMaxFileTime(file)
                currentTxStream = ftrWriter.AddTXStream(stream[1], min-EARLIEST, max-EARLIEST)
                #print("Methods: ")
                for methodIdx, method in enumerate(file[2]):
                    #print(method)
                    #print("Executions:" )
                    for execIdx, executions in enumerate(method[3]):
                        #print(executions)
                        fixedEnd = executions[1]
                        if fixedEnd == None:
                            fixedEnd = LATEST
                        #print("Generators:")
                        for generator in GENERATORS:
                            #print(generator)
                            if generator[0] == method[0]: # and (executions[0]-EARLIEST < fixedEnd-EARLIEST):
                                currentTransId = ftrWriter.AddTransaction(currentTxStream, generator[1], executions[0]-EARLIEST, fixedEnd-EARLIEST)
                                # for transaction in TRANSACTIONS:
                                #     if (currentTransId == 10292 or transaction[3] == 10292) and (currentTransId == 9247 or transaction[3] == 9247):
                                #         print("Relation Call: " + str(currentTransId) + " -> " + str(transaction[3]))
                                #         print("Relation Return: " + str(transaction[3]) + " -> " + str(currentTransId))
                                #         print("File: " + str(executions[2][0]) + " -> " + str(transaction[0]))
                                #         print("Method: " + str(executions[2][1]) + " -> " + str(transaction[1]))
                                #         print("Execution: " + str(executions[2][2]) + " -> " + str(transaction[2]))
                                #     if executions[2][0] == transaction[0] and executions[2][1] == transaction[1] and executions[2][2] == transaction[2]:
                                #         ftrWriter.AddRelation("call", transaction[3], currentTransId, transaction[4], stream[1])
                                #         ftrWriter.AddRelation("return", currentTransId, transaction[3], stream[1], transaction[4])
                                # TRANSACTIONS.append([fileIdx, methodIdx, execIdx, currentTransId, stream[1]])
                                    
                                    
    ftrWriter.GenerateTXBlockChunk()
    ftrWriter.GenerateRelationChunk()
    ftrWriter.WriteFTRFile(configuration.CFG_FTR_RESULT)
    
def readMethods():
    global METHODS
    with open("./files/methods.exp", "rb") as methodFile:
        METHODS = pickle.load(methodFile)

def generate():
    print("Reading Methods")
    readMethods()
    print("Generating Dictionary")
    generateDictionary()
    print("Reading Tracer Results")
    readTracerResults()
    print("Generating File")
    generateFTRFile()