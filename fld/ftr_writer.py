# Based on https://github.com/Minres/LWTR4SC/blob/main/src/ftr/ftr_writer.h

import datetime
from datetime import datetime, timezone
from cbor2 import CBORTag, dumps
import struct

#region FTRWriter
class FTRWriter:

    #region Constructors

    def __init__(self):
        self.FTRFileContent = self._GenerateTag(55799)
        self.FTRFileContent += self._GenerateArraySize()
        self.DumpedDictItems = 0
        self.Dictionary = []
        self.Streams = []
        self.Generators = []
        self.TXEntries = []
        self.TransactionId = 0
        self.Relations = []

    #endregion Constructors
    
    #region FileHandlers

    def WriteFTRFile(self, fileName):
        self.FTRFileContent += self._GenerateBreak()
        with open(fileName, "wb") as ftrFile:
            ftrFile.write(self.FTRFileContent)

    #endregion FileHandlers

    #region Helper Functions

    def _GenerateArraySize(self, size = None):
        if size is not None:
            return self._GenerateTypedValue(4, size)
        else:
            return (struct.pack('B', ((4<<5) | 31) & 0xFF))
        
    def _GenerateTag(self, value):
        return self._GenerateTypedValue(6, value)
    
    def _GenerateTime(self, time):
        return self._GenerateTypedValue(0, time)

    def _GenerateSigned(self, value):
        if value < 0:
            return self._GenerateTypedValue(1, abs(value) -1)
        else:
            return self._GenerateTypedValue(0, value)

    def _GenerateBreak(self):
        return (struct.pack('B', 0xFF))

    def _GenerateDictionaryItem(self, idx, text):
        item = self._GenerateTypedValue(0, idx)
        item += self._GenerateTypedValue(3, len(text.encode('utf-8')))
        item += text.encode('utf-8') 
        return item
    
    def _GenerateTypedValue(self, majorType, value):
        majorType = majorType << 5
        typedValue = None
        if value < 24:
            typedValue = (struct.pack('B', majorType | value))
        elif value < 255:
            typedValue = (struct.pack('B', majorType | 24))
            typedValue += (struct.pack('B', (value) & 0xFF))
        elif value < 65536:
            typedValue = (struct.pack('B', majorType | 25))
            typedValue += (struct.pack('B', (value >> 8) & 0xFF))
            typedValue += (struct.pack('B', (value) & 0xFF))
        elif value < 2**32-1:
            typedValue = (struct.pack('B', majorType | 26))
            typedValue += (struct.pack('B', (value >> 24) & 0xFF))
            typedValue += (struct.pack('B', (value >> 16) & 0xFF))
            typedValue += (struct.pack('B', (value >> 8) & 0xFF))
            typedValue += (struct.pack('B', (value) & 0xFF))
        else:
            typedValue = (struct.pack('B', majorType | 27))
            typedValue += (struct.pack('B', (value >> 56) & 0xFF))
            typedValue += (struct.pack('B', (value >> 48) & 0xFF))
            typedValue += (struct.pack('B', (value >> 40) & 0xFF))
            typedValue += (struct.pack('B', (value >> 32) & 0xFF))
            typedValue += (struct.pack('B', (value >> 24) & 0xFF))
            typedValue += (struct.pack('B', (value >> 16) & 0xFF))
            typedValue += (struct.pack('B', (value >> 8) & 0xFF))
            typedValue += (struct.pack('B', (value) & 0xFF))    
        return typedValue
    
    #endregion Helper Functions

    #region Chunks

    #region Info Chunk

    def GenerateInfoChunk(self, unit):
        infoChunk = self._GenerateArraySize(2)
        infoChunk += self._GenerateSigned(unit)
        infoChunk += self._GenerateTag(1)
        infoChunk += self._GenerateTime(int(datetime.now(timezone.utc).replace(microsecond=0).timestamp()))

        self.FTRFileContent += dumps(CBORTag(6, infoChunk))
    
    #endregion Info Chunk

    #region Dictionary Chunk

    def AddToDictionary(self, item):
        self.Dictionary.append(item)
        return self.Dictionary.index(item)

    def GenerateDictionaryChunk(self):
        dictionary = self._GenerateTypedValue(5, len(self.Dictionary[self.DumpedDictItems:]))
        for idx, item in enumerate(self.Dictionary[self.DumpedDictItems:]):
            dictionary += self._GenerateDictionaryItem(idx+self.DumpedDictItems, item)
        self.DumpedDictItems = len(self.Dictionary)
        self.FTRFileContent += dumps(CBORTag(8, dictionary))

    #endregion Dictionary Chunk

    #region Directory Chunk

    def AddToStreams(self, name, kind):
        nameIndex = self.Dictionary.index(name)
        kindIndex = self.Dictionary.index(kind)
        self.Streams.append([nameIndex, kindIndex])
        return self.Streams.index([nameIndex, kindIndex]) + 1 

    def AddToGenerators(self, name, stream):
        nameIndex = self.Dictionary.index(name)
        streamIndex = stream
        self.Generators.append([nameIndex, streamIndex])
        return self.Generators.index([nameIndex, streamIndex]) + 1 + len(self.Streams) 

    def _GenerateStream(self, streamIdx, nameIdx, kindIdx):
        stream = self._GenerateTag(16)
        stream += self._GenerateArraySize(3)
        stream += self._GenerateSigned(streamIdx)
        stream += self._GenerateSigned(nameIdx)
        stream += self._GenerateSigned(kindIdx)
        return stream
    
    def _GenerateGenerator(self, generatorIdx, nameIdx, streamIdx):
        generator = self._GenerateTag(17)
        generator += self._GenerateArraySize(3)
        generator += self._GenerateSigned(generatorIdx)
        generator += self._GenerateSigned(nameIdx)
        generator += self._GenerateSigned(streamIdx)
        return generator

    def GenerateDirectoryChunk(self):
        directory = self._GenerateArraySize()
        lastIdx = 0
        for lastIdx, item in enumerate(self.Streams):
            directory += self._GenerateStream(lastIdx+1, item[0], item[1])
        
        for idx, item in enumerate(self.Generators):
            directory += self._GenerateGenerator(lastIdx + idx + 2, item[0], item[1])

        directory += self._GenerateBreak()
        self.FTRFileContent += dumps(CBORTag(10, directory))

    #endregion Directory Chunk

    #region TX Block Chunk

    def AddTXStream(self, streamIdx, startTime, endTime):
        self.TXEntries.append([streamIdx, startTime, endTime, []])
        return self.TXEntries.index([streamIdx, startTime, endTime, []])

    def AddTransaction(self, txEntryIdx, generatorIdx, startTime, endTime):
        self.TransactionId += 1
        self.TXEntries[txEntryIdx][3].append([self.TransactionId, generatorIdx, startTime, endTime, []])
        return self.TransactionId
    
    def AddAttribute(self, transactionId, attrType, name, type, value):
        nameIdx = self.Dictionary.index(name)
        for txEntryIdx, txStream in enumerate(self.TXEntries):
            for i, transaction in enumerate(txStream[3]):
                if transaction[0] == transactionId:
                    self.TXEntries[txEntryIdx][3][i][4].append([attrType, nameIdx, type, value])

    def GenerateTXBlockChunk(self):
        for stream in self.TXEntries:
            #txblock = self._GenerateArraySize(len(stream[3]))
            txblock = self._GenerateArraySize()
            
            for idx, transaction in enumerate(stream[3]):
                txblock += self._GenerateArraySize(len(transaction[4]) + 1)
                txblock += self._GenerateTag(6)
                txblock += self._GenerateArraySize(4)
                txblock += self._GenerateSigned(transaction[0])
                txblock += self._GenerateSigned(transaction[1])
                txblock += self._GenerateSigned(transaction[2])
                txblock += self._GenerateSigned(transaction[3])

                for attribute in transaction[4]:
                    txblock += self._GenerateTag(attribute[0])
                    txblock += self._GenerateArraySize(3)
                    txblock += self._GenerateSigned(attribute[1])
                    txblock += self._GenerateSigned(attribute[2])
                    txblock += self._GenerateSigned(attribute[3])
                
            txblock += self._GenerateBreak()
            self.FTRFileContent += dumps(CBORTag(12, [stream[0], stream[1], stream[2], txblock]))

    #endregion TX Block Chunk

    #region Relations Chunk

    def AddRelation(self, name, txSource, txSink, streamSource, streamSink):
        nameIdx = self.Dictionary.index(name)
        self.Relations.append([nameIdx, txSource, txSink, streamSource, streamSink])

    def GenerateRelationChunk(self):
        relations = self._GenerateArraySize()
        for relation in self.Relations:
            relations += self._GenerateArraySize(5)
            relations += self._GenerateSigned(relation[0])
            relations += self._GenerateSigned(relation[1])
            relations += self._GenerateSigned(relation[2])
            relations += self._GenerateSigned(relation[3])
            relations += self._GenerateSigned(relation[4])
        relations += self._GenerateBreak()
        self.FTRFileContent += dumps(CBORTag(14, relations))

    #endregion Relations Chunk

    #endregion Chunks

#endregion