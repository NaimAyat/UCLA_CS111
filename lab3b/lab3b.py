#NAME: Naim Ayat
#EMAIL: naimayat@ucla.edu
#ID: 000000000
from __future__ import print_function
import sys

# typedefs to make sys functions less verbose
w = sys.stdout.write
argv = sys.argv

# print to stderr
def eprint(*a, **b):
    print(*a, file=sys.stderr, **b)

# enable switch statements
class switch(object):
    i = 0
    def __new__(class_, i):
        class_.i = i
        return 1
def case(*choices):
    return any((choice == switch.i for choice in choices))


def redundancyHandler(nBlock, nInode, block):
    offset = 0
    while switch(block):
        if case('TRIPLE INDIRECT BLOCK'):
            offset = 65804
            break
        if case('DOUBLE INDIRECT BLOCK'):
            offset = 268
            break
        if case('INDIRECT BLOCK'):
            offset = 12
            break
        break
    w('DUPLICATE ' + block + ' ' + str(nBlock) + ' IN INODE ' + str(nInode) + ' AT OFFSET ' + str(offset) + '\n')


def blockPtrHandler(nBlock, nInode, block, offset, begin, end):
    if nBlock < 0 and nBlock < end - 1:
        w('INVALID ' + block + ' ' + str(nBlock) + ' IN INODE ' + str(nInode) + ' AT OFFSET ' + str(offset) + '\n')
        return False
    if nBlock > 0 and nBlock > end - 1:
        w('INVALID ' + block + ' ' + str(nBlock) + ' IN INODE ' + str(nInode) + ' AT OFFSET ' + str(offset) + '\n')
        return False
    if nBlock < 0 and nBlock > end - 1:
        w('INVALID ' + block + ' ' + str(nBlock) + ' IN INODE ' + str(nInode) + ' AT OFFSET ' + str(offset) + '\n')
        return False
    elif nBlock > 0 and nBlock < begin:
        w('RESERVED ' + block + ' ' + str(nBlock) + ' IN INODE ' + str(nInode) + ' AT OFFSET ' + str(offset) + '\n')
        return False
    return True


# Block Consistency Audits
def bAudit(file):
    bMap = {}

    for line in file:
        dataIn = line.split(',')

        while switch(dataIn[0]):
            if case ('SUPERBLOCK'):
                lenInode = int(dataIn[4])
                lenBlock = int(dataIn[3])
                break

            if case ('GROUP'):
                blockCount = int(dataIn[2])
                inodeCount = int(dataIn[3])
                inodeOff = int(dataIn[8].split('\n')[0])
                availBlockBeg = inodeOff + (lenInode * inodeCount / lenBlock)
                break

            if case ('BFREE'):
                nBlock = int(dataIn[1].split('\n')[0])
                bMap[nBlock] = 'FREE'
                break

            if case ('INODE'):
                nInode = dataIn[1]
                x = 12
                while x < 27:
                    nBlock = int(dataIn[x].split('\n')[0])
                    while switch(x):
                        if case(12,13,14,15,16,17,18,19,20,21,22,23) and nBlock != 0 and nBlock not in bMap:
                            if blockPtrHandler(nBlock, nInode, 'BLOCK', 0, availBlockBeg, blockCount):
                                bMap[nBlock] = (nInode, 'BLOCK')
                            break

                        if case(24) and nBlock != 0 and nBlock not in bMap:
                            if blockPtrHandler(nBlock, nInode, 'INDIRECT BLOCK', 12, availBlockBeg, blockCount):
                                bMap[nBlock] = (nInode, 'INDIRECT BLOCK')
                            break

                        if case(25) and nBlock != 0 and nBlock not in bMap:
                            if blockPtrHandler(nBlock, nInode, 'DOUBLE INDIRECT BLOCK', 268, availBlockBeg, blockCount):
                                bMap[nBlock] = (nInode, 'DOUBLE INDIRECT BLOCK')
                            break

                        if case(26) and nBlock != 0 and nBlock not in bMap:
                            if blockPtrHandler(nBlock, nInode, 'TRIPLE INDIRECT BLOCK', 65804, availBlockBeg, blockCount):
                                bMap[nBlock] = (nInode, 'TRIPLE INDIRECT BLOCK')
                            break

                        if case(12,13,14,15,16,17,18,19,20,21,22,23,24,25,26) and nBlock != 0 and bMap[nBlock] == 'FREE':
                            w('ALLOCATED BLOCK ' + str(nBlock) + ' ON FREELIST\n')
                            break

                        if case(12,13,14,15,16,17,18,19,20,21,22,23,24,25,26) and nBlock != 0 and bMap[nBlock] != 'FREE':
                            block = 'BLOCK'
                            while switch (x):
                                if case(24): 
                                    block = 'INDIRECT BLOCK'
                                    break                      
                                if case(25): 
                                    block = 'DOUBLE INDIRECT BLOCK'
                                    break
                                if case(26): 
                                    block = 'TRIPLE INDIRECT BLOCK'
                                    break 
                                break 
                    
                            redundancyHandler(nBlock, nInode, block)
                            redundancyHandler(nBlock, bMap[nBlock][0], bMap[nBlock][1])
                            break
                        break
                    x += 1
                break

            if case ('INDIRECT'):
                nInode = int(dataIn[1])
                lev = int(dataIn[2])
                off = int(dataIn[3])
                nBlock = int(dataIn[5].split('\n')[0])

                block = 'BLOCK'
                while switch(lev):
                    if case(1):
                        block = 'INDIRECT BLOCK';
			break
                    if case(2):
                        block = 'DOUBLE INDIRECT BLOCK';
			break
                    if case(3):
                        block = 'TRIPLE INDIRECT BLOCK';
			break
	            break

                if nBlock in bMap and bMap[nBlock] == 'FREE':
                        w('ALLOCATED BLOCK ' + str(nBlock) + ' ON FREELIST\n')
                        continue

                elif nBlock in bMap and bMap[nBlock] != 'FREE':
                        redundancyHandler(nBlock, nInode, block)
                        redundancyHandler(nBlock, bMap[nBlock][0], bMap[nBlock][1])
                        continue

                if (blockPtrHandler(nBlock, nInode, block, off, availBlockBeg, blockCount)):
                    bMap[nBlock] = (nInode, block)
                break
            break

    x = availBlockBeg
    while x < blockCount:
        if x not in bMap:
            w('UNREFERENCED BLOCK ' + str(x) + '\n')
        x += 1
            
    file.seek(0)


# I-node Allocation Audits
def iAudit(file):
    iMap = {}

    for line in file:
        dataIn = line.split(',')
        nInode = int(dataIn[1])

        while switch(dataIn[0]):

            if case('SUPERBLOCK'):
                inodeCount = int(dataIn[2])
                availInodeBeg = int(dataIn[7].split('\n')[0])
                break

            if case ('IFREE'):
                iMap[nInode] = 'FREE'
                break

            if case ('INODE'):
                if nInode not in iMap:
                    iMap[nInode] = 'ALLOCATED'
                    break

            if case ('INODE'):
                w('ALLOCATED INODE ' + str(nInode) + ' ON FREELIST\n')
                iMap[nInode] = 'ALLOCATED'
                break
            break

    i = availInodeBeg
    while i <= inodeCount:
        if i not in iMap:
            w('UNALLOCATED INODE ' + str(i) + ' NOT ON FREELIST\n')
        i += 1

    file.seek(0)
    return iMap


# Directory Consistency Audits
def dAudit(file, iMap):
    path, nLinks, lMap = [{},{},{}]
    avail = iMap    
    notAvail = []

    for key in avail:
        if avail[key] != 'FREE':
            notAvail.append(key)

    for key in notAvail:
        del avail[key]

    for line in file:
        dataIn = line.split(',')
        if dataIn[0] == 'INODE':
            nInode = int(dataIn[1])
            prelimLinks = int(dataIn[6])
            nLinks[nInode] = 0
            lMap[nInode] = prelimLinks

    file.seek(0)

    for line in file:
        dataIn = line.split(',')

        while switch(dataIn[0]):
            if case('DIRENT'):
                nInode1 = int(dataIn[1])
                nInodeFile = int(dataIn[3])
                directory = dataIn[6].split('\n')[0]

                if nInodeFile in avail:
                    w('DIRECTORY INODE ' + str(nInode1) + ' NAME ' + directory + ' UNALLOCATED INODE ' + str(nInodeFile) + '\n')

                elif nInodeFile not in avail and nInodeFile not in lMap:
                    w('DIRECTORY INODE ' + str(nInode1) + ' NAME ' + directory + ' INVALID INODE ' + str(nInodeFile) + '\n')

                else:
                    nLinks[nInodeFile] += 1
                    if nInodeFile not in path:
                        path[nInodeFile] = nInode1
                # No break

            if case('DIRENT'):
                nInode1 = int(dataIn[1])
                nInodeFile = int(dataIn[3])
                directory = dataIn[6].split('\n')[0]
                
                while switch(directory):
                    if case("'.'") and (nInodeFile != nInode1):
                        w('DIRECTORY INODE ' + str(nInode1) + ' NAME ' + directory + ' LINK TO INODE ' + str(nInodeFile) + ' SHOULD BE ' + str(nInode1) + '\n')
                        break

                    if case ("'..'"):
                        nInodeBer = path[nInode1]
                        if(nInodeFile != nInodeBer):
                            w('DIRECTORY INODE ' + str(nInode1) + ' NAME ' + directory + ' LINK TO INODE ' + str(nInodeFile) + ' SHOULD BE ' + str(nInodeBer) + '\n')
                        break
                    break
                break         
            break

    for nInode in lMap:
        if nLinks[nInode] != lMap[nInode]:
            w('INODE ' + str(nInode) + ' HAS ' + str(nLinks[nInode]) + ' LINKS BUT LINKCOUNT IS ' + str(lMap[nInode]) + '\n')

    file.seek(0)


def main():
    if len(argv) == True:
        eprint('ERROR: Incorrect arguments. Usage: ./lab3b.\n')
        exit(1)

    for arg in argv:
        if arg == 'lab3b.py': 
            continue

        try:
            file = open(arg)
            bAudit(file)
            iMap = iAudit(file)
            dAudit(file, iMap)
            file.close()

        except IOError as e:
            eprint('ERROR: Failed to open summary file.\n')
            exit(1)

main()


