import sys

import configuration
import codeStructure

from elftools.dwarf.descriptions import describe_form_class
from elftools.elf.elffile import ELFFile
from elftools.dwarf.locationlists import (LocationEntry, LocationExpr, LocationParser)

from pathlib import Path

import pickle

HW_DWARF_INFO = []
SW_DWARF_INFO = []
PC_LINE_ADDRESS = 0

METHODS = []

def parsePCs(dwarfinfo, type):
    global HW_DWARF_INFO
    global SW_DWARF_INFO
    global PC_LINE_ADDRESS

    binary_path = ""
    if type == "SW":
        binary_path = configuration.CFG_SOFTWARE
    elif type == "HW":
        binary_path = configuration.CFG_HARDWARE

    for CU in dwarfinfo.iter_CUs():
        lineprog = dwarfinfo.line_program_for_CU(CU)
        delta = 1 if lineprog.header.version < 5 else 0
        #delta = 0
        prevstate = None
        for entry in lineprog.get_entries():
            if entry.state is None:
                continue
            if prevstate:
                address = prevstate.address
                path = str(lineprog["include_directory"][lineprog['file_entry'][prevstate.file - delta]["dir_index"]].decode("utf-8"))
                
                if path.startswith("../../"):
                    path = path.replace("../../", str(Path(binary_path).resolve().parent.parent.parent.absolute()))
                elif path.startswith("../"):
                    path = path.replace("../", str(Path(binary_path).resolve().parent.parent.absolute()) + "/")
                filename = str(lineprog['file_entry'][prevstate.file - delta].name.decode('utf-8'))
                line = prevstate.line
                column = prevstate.column
                if type == "SW":
                    path = path.replace(configuration.CFG_SOFTWARE_MAIN_FOLDER, "") # workaround sometime the path ist already stored absolute
                    path = configuration.CFG_SOFTWARE_MAIN_FOLDER + "/" + path
                    append = True
                    if len(configuration.CFG_SOFTWARE_EXECUTION_WHITELIST) > 0:
                        append = False
                        for whitelist_file in configuration.CFG_SOFTWARE_EXECUTION_WHITELIST:
                            if str(Path(path + "/" + filename).absolute().resolve()) == whitelist_file:
                                append = True
                    if append:
                        SW_DWARF_INFO.append([str(Path(path + "/" + filename).absolute().resolve()), line, address])
                elif type == "HW":
                    pc_line_info = codeStructure.PC_LINE.split(":")
                    if str(Path(path + "/" + filename).absolute().resolve()) == pc_line_info[0] and line == int(pc_line_info[1]):
                        if address > PC_LINE_ADDRESS:
                            PC_LINE_ADDRESS = address
                    append = True
                    if len(configuration.CFG_HARDWARE_EXECUTION_WHITELIST) > 0:
                        append = False
                        for whitelist_file in configuration.CFG_HARDWARE_EXECUTION_WHITELIST:
                            if str(Path(path + "/" + filename).absolute().resolve()) == whitelist_file:
                                append = True
                    if append:
                        HW_DWARF_INFO.append([str(Path(path + "/" + filename).absolute().resolve()), line, address])
            if entry.state.end_sequence:
                prevstate = None
            else:
                prevstate = entry.state

def readDWARFInfo(type):
    path = ""
    if type == "SW":
        path = configuration.CFG_SOFTWARE
    elif type == "HW":
        path = configuration.CFG_HARDWARE
    
    with open(path, "rb") as binary:
        elffile = ELFFile(binary)
        dwarinfo = None
        if not elffile.has_dwarf_info:
            print("... Error: Binary " + path + " has no DWARF information")
            sys.exit(0)
        try:
            dwarfinfo = elffile.get_dwarf_info()
        except:
            print("Error in DWARF!")
        if dwarfinfo != None:
            parsePCs(dwarfinfo, type)

def getHWDWARFAddress(fileName, line):
    global HW_DWARF_INFO
    for hw_info in HW_DWARF_INFO:
        if hw_info[0] == fileName and hw_info[1] == line:
            return hw_info[2]

def exportHWAddresses(address_file, fileName, content):
    global METHODS
    for inner in content:
        if inner.Type == "Method":
            inner.StartAddress = getHWDWARFAddress(fileName, inner.StartLine)
            inner.EndAddresses = []
            for endline in inner.EndLines:
                inner.EndAddresses.append(getHWDWARFAddress(fileName, endline))
            if inner.EndAddresses == []:
                inner.EndAddresses = [0]
            if inner.StartAddress is not None:
                address_file.write("1:" + str(inner.StartAddress) + "\n")
                for endaddress in inner.EndAddresses:

                    address_file.write("1:" + str(endaddress) + "\n")
                METHODS.append([0, fileName, inner.Name, inner.StartAddress, inner.EndAddresses])
        exportHWAddresses(address_file, fileName, inner.Content)

def getSWDWARFAddress(fileName, line):
    global SW_DWARF_INFO
    for sw_info in SW_DWARF_INFO:
        if sw_info[0] == fileName and sw_info[1] == line:
            return sw_info[2]

def exportSWAddresses(address_file, fileName, content):
    global METHODS
    for inner in content:
        if inner.Type == "Method":
            inner.StartAddress = getSWDWARFAddress(fileName, inner.StartLine)
            inner.EndAddresses = []
            for endline in inner.EndLines:
                inner.EndAddresses.append(getSWDWARFAddress(fileName, endline))
            if inner.EndAddresses == []:
                inner.EndAddresses = [0]
            if inner.StartAddress is not None:
                address_file.write("2:" + str(inner.StartAddress) + "\n")
                for endaddress in inner.EndAddresses:
                    address_file.write("2:" + str(endaddress) + "\n")
                print(fileName + ":" + inner.Name + ":" + hex(inner.StartAddress) + str(inner.EndAddresses))
                if inner.Name == "mems_gyro_irq_handler":
                    print("stop")
                METHODS.append([1, fileName, inner.Name, inner.StartAddress, inner.EndAddresses])
        exportSWAddresses(address_file, fileName, inner.Content)

def exportAddressTable():
    global PC_LINE_ADDRESS
    with open(configuration.CFG_ADDRESS_TABLE, "w") as address_file:
        address_file.write("0:" + str(PC_LINE_ADDRESS) + "\n")
        for hw in codeStructure.CODE_STRUCTURE.HWLibraries:
            for file in hw.Files:
                exportHWAddresses(address_file, file.Name, file.Content)
        for sw in codeStructure.CODE_STRUCTURE.SWLibraries:
            for file in sw.Files:
                exportSWAddresses(address_file, file.Name, file.Content)

def storeMethods():
    with open("./files/methods.exp", "wb") as methodsFile:
        pickle.dump(METHODS, methodsFile)

def generateAddressTable():
    readDWARFInfo("HW")
    readDWARFInfo("SW")
    exportAddressTable()
    storeMethods()