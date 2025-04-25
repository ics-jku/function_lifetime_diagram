import fileStructure
import configuration

class Content:
    def __init__(self, Type, Name, Content, Branches):
        self.Type = Type
        self.Name = Name
        self.Content = Content

class File:
    def __init__(self, Name, Content):
        self.Name = Name
        self.Content = Content

class Library:
    def __init__(self, Path, Name, Files):
        self.Path = Path
        self.Name = Name
        self.Files = Files

class Code:
    def __init__(self):
        self.HWLibraries = []
        self.SWLibraries = []
        #self.MODLibraries = []

class ConditionNode:
    def __init__(self, condition):
        self.condTrue = None
        self.condFalse = None
        self.condition = condition

CODE_STRUCTURE = Code()
PC_LINE = ""

def findMethod(tokenized):
    
    openingPos = -1
    foundLamda = False
    for i, token in enumerate(reversed(tokenized)):
        token = token.replace("\t", " ").replace("\n", " ")
        if token.find(":") != -1 and token.find(":") != token.find("::") or token.find("=") != -1:
            openingPos = -1
        if token.rfind(")") < token.rfind(";"):
            break
        if token.rfind(")>") != -1:
            foundLamda = True
        
        if (token.find(" for(") != -1 or token.startswith("for(") or token.startswith("for (") or
        token.find(" if(")  != -1 or token.startswith("if(") or token.startswith("if (") or
        token.find(" while(") != -1 or token.startswith("while(") or token.startswith("while (") or
        token.find(" try") != -1 or token.startswith("try")):
            openingPos = -1
            break
        if token.rfind("(") != -1 and not foundLamda:
            openingPos = len(tokenized) - i - 1
        if foundLamda and token.rfind("<") != -1:
            foundLamda = False
    if openingPos != -1:
        methodName = tokenized[openingPos].replace("\t", " ").split("(")[0].split(" ")[-1]
        return methodName     
    return ""

def recursiveParse(source):
    i = 0
    k = 0
    content = []
    while(i < len(source)):
        if source[i] == "{":
            l, subcontent = recursiveParse(source[i+1:])
            i = l + i + 1
            tokenized = source[:i].replace("\n", " ").split("{")[0].split(" ")
            
            for j, token in enumerate(reversed(tokenized)):
                token = token.rstrip().lstrip()
                if(token.find(")") != -1):
                    pos = len(tokenized) - j
                    if source.find("read_touch_addr_raw(") != -1:
                        print("Stop")
                    methodName = findMethod(tokenized[:pos])
                    # print(methodName)
                    # if methodName == "" and source.find("read_touch_addr_raw(") != -1:
                    #     print("Stop")
                    # if methodName == "GD32Options":
                    #     print("Stop")
                    content.append(Content("Method", methodName, [], []))
                    break
                if token.endswith("class") or token.endswith("namespace") or token.endswith("struct"):
                    content.append(Content(token.replace("\n", " ").replace("\t", " ").split(" ")[-1], tokenized[len(tokenized) - j], subcontent, []))
                    break
                if token.find(";") != -1 or token.find(">") != -1:
                    break
            source = source[i:]
            k += i
            i = 1
        elif source[i] == "}":
            
            return k + i, content
        i += 1
    return i, content

def readFile(file):
    content = []
    #print(file)
    with open(file, "r") as content:
        # if file.find("iss.cpp") != -1:
        #     print("stop")
        source = content.read()
        commentstart = source.find("//")
        while(commentstart != -1):
            
            commentend = source[commentstart::].find("\n")
            if commentend == -1:
                commentend = len(source)
            else:
                commentend = commentstart + commentend
            source = source.replace(source[commentstart:commentend], "")
            commentstart = source.find("//")
        while(commentstart != -1):
            commentend = source[commentstart::].find("*/")
            if commentend == -1:
                commentend = len(source)
            else:
                commentend = commentstart + commentend
            numberOfLineBreaks = source[commentstart:commentend].count("\n")
            linebreaks = ""
            for i in range(numberOfLineBreaks):
                linebreaks = linebreaks + "\n"
            source = source.replace(source[commentstart:commentend], linebreaks)
            commentstart = source.find("/*")
    end, content = recursiveParse(source)
    return content

def appendFile(libraries, file):
    found = False
    pos = 0
    content = readFile(file[2])
    sourceFile = File(file[2], content)
    for i, library in enumerate(libraries):
        if library.Name == file[1][1]:
            found = True
            pos = i
    if not found:
        libraries.append(Library(file[1][0], file[1][1], [sourceFile]))
    else:
        libraries[pos].Files.append(sourceFile)


def findBlockEnd(code):
    lvl = 1
    for i, character in enumerate(code):
        if character == "{":
            lvl += 1
        if character == "}":
            lvl -= 1
        if lvl == 0:
            return i
    return -1


def generateCodeFileInformationRecursive(content, code, lines):
    end = 0
    if content.Type == "Method":
        if content.Name == "mems_gyro_irq_handler":
            print("Stop")
        codePos = -1
        if code.replace("\t", " ").find(" " + content.Name + "(") != -1:
            codePos = code.replace("\t", " ").find(" " + content.Name + "(")
        elif code.replace("\t", " ").find("\n" + content.Name + "(") != -1:
            codePos = code.replace("\t", " ").find("\n" + content.Name + "(")
        content.StartLine = lines + code[:codePos].count("\n") + 1   
        interfaceStart = code.replace("\t", " ").find(" " + content.Name + "(") + 1
        if interfaceStart == -1:
            content.LineNumber = lines + code[:code.replace("\t", " ").find(content.Name + "(")].count("\n") + 1   
            interfaceStart = code.replace("\t", " ").find(content.Name + "(")
        start = interfaceStart + code[interfaceStart:].find("{") + 1
        end = start + findBlockEnd(code[start:])
        content.StartLine += code[code.replace("\t", " ").find(" " + content.Name + "("):start].count("\n")
        lines = lines + code[:end].count("\n")
        if code[start:end].replace("\t", " ").find(" return ") != -1:
            next_start = start
            content.EndLines = []
            while code[next_start:end].replace("\t", " ").find(" return ") != -1:
                next_start = next_start + code[next_start:end].replace("\t", " ").find(" return ") + 1
                content.EndLines.append(content.StartLine + code[start:next_start].count("\n"))
        else:
            content.EndLines = [lines + 1]
        #for i, line in enumerate(lines):
        #    content.Content.append(Content("Line", i+content.LineNumber, [], []))
    else:
        for subcontent in content.Content:
            tmp_end,lines = generateCodeFileInformationRecursive(subcontent, code[end:], lines)
            end = end + tmp_end
    return end, lines

def parseCodeContentHierarchy():
    global CODE_STRUCTURE
    for file in fileStructure.FILES:
        if file[0] == "HW":
            appendFile(CODE_STRUCTURE.HWLibraries, file)
        elif file[0] == "SW":
            appendFile(CODE_STRUCTURE.SWLibraries, file)
        else:
            print("<X> error when parsing the code content hierarchy")

def parseCodeContent():
    global PC_LINE
    
    for hw in CODE_STRUCTURE.HWLibraries:
        for file in hw.Files:
            with open(file.Name, "r") as source:
                code = source.read()
                start = code.find(configuration.CFG_PC + " = ")
                end = len(code)
                while start != -1:
                    possibleAssignment = code[start:code.find(configuration.CFG_PC + "  ")]
                    if possibleAssignment.strip().startswith(configuration.CFG_PC + " = ") and PC_LINE == "" and not possibleAssignment.strip().startswith(configuration.CFG_PC + " = 0"):
                        PC_LINE = file.Name + ":" + str(code[:start].count("\n") + 1)
                    if code[start:].find(configuration.CFG_PC + " = ") != -1:
                        start = start + code[start:].find(configuration.CFG_PC + " = ") + 1
                    else:
                        start = -1
                last_pos = 0
                lines = 0
                for content in file.Content:

                    tmp_pos, lines = generateCodeFileInformationRecursive(content, code[last_pos:], lines)
                    last_pos = last_pos + tmp_pos
                    #lines = lines + tmp_lines

    for sw in CODE_STRUCTURE.SWLibraries:
        for file in sw.Files:
            with open(file.Name, "r") as source:
                code = source.read().replace("\t", " ")
                last_pos = 0
                lines = 0
                for content in file.Content:
                    tmp_pos, lines = generateCodeFileInformationRecursive(content, code[last_pos:], lines)
                    last_pos = last_pos + tmp_pos
                    #lines = lines + tmp_lines

def parseCodeStructure():
    parseCodeContentHierarchy()
    parseCodeContent()
