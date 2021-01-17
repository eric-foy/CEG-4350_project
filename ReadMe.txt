CEG 433/633 P0 ReadMe.txt
thomas.wischgoll@wright.edu

The files in this directory constitute a simple implementation of a
file system written in C++, but using minimal features of OOP.
Efficiency or good OOP style was not a goal.  But, illustrating good
systems programming style was a goal.

The code segments that invoke TODO() are for you to develop in later
projects.

The design follows faithfully the details of i-node based file
volumes, described in the lectures.  So, these notes highlight only
the non-obvious.

Robustness: There are a few robustness checks that I often omitted in
order to keep the main logic of the code clear.  E.g., we *should*
check that the new operator does return a non-0 pointer.  Several
buffers are declared to be 1024 or so long; even so, we must check
that these not overflowing.

The methods named create() and reCreate() are our "constructors."  The
name create is used when the simulated disk is "fresh" and a file
volume is yet to be made out of it.  The name reCreate is used when
re-creating the objects from the file volume image that is already
present on the sim disk.  Similarly, the methods named destroy() are
destructors.  As explained in the lectures, this gives us greater
control over when the objects are initialized and finalized.


iNodes.cpp: 

A well designed class has an invariant that holds true for all methods
except the constructors and destructors. Make sure that you understand
the following class invariant: (i) No block number appears more than
once in the i-nodes.  (ii) If a 0 appears as a block number in an
i-node or indirect blocks, all subsequent entries for that file must
be 0 also.  (iii) If fbvb[x] == 1, then block numbered x must not
apear in any i-node direct entries, or indirect blocks, and (iv) if an
x appears any i-node direct entries, or indirect blocks, then fbvb[x]
must be 0.

Note that in our design, (i) a single iNode must never be bigger than
a single disk block, and (ii) a block contains a fixed number
(iNodesPerBlock) of iNodes snugly, i.e., iNodesPerBlock * iHeight *
iWidth == nBytesPerBlock.

There is no class for a single iNode.  Instead, one block of iNodes at
a time is loaded from disk into uintBuffer, and the variable named pin
(pointer to iNode) points into uintBuffer as an array of iNode
entries.  The method getINode(n, 0) calculates this pointer for iNode
numbered n.

pin[0], pin[1], ... , pin[iDirect-1] are direct block numbers;
pin[iDirect]	is the single indirect block number;
pin[iDirect+1] 	is the double indirect block number;
pin[iDirect+2]	is the triple indirect block number;
pin[iHeight - 1] is fileSize;

Note how the various numbers (double-indirect, filesize, e.g.) are
stored at different positions in the i-node.

The iNode width is assumed to be 4 bytes.  I used a trivial heuristic
to decide the number of indirect entries.

When we create a fresh iNode, all its entries must be zero.  But as we free
and re-use iNodes we must be sure that the iNode is zeroed out before
reuse.


volume.cpp: 

Note that that one block equals one sector.  For now.

For you TODO: How will you verify that there is indeed a previously
made file volume on a simDisk?  The code given in this solution is
valid but *weak*.

directory.cpp: 

The dirEntry is allocated in the (re)creates. It is used to return a
directory entry (pair of file-name string followed by the inumber).

Class invariant: (i) Every directory has the dot and dot-dot
entries. (ii) No name appears more than once.  (iii) No name is empty,
or contains illegal (such as blanks, tabs, slash) chars.


shell.cpp: 

Several unrelated routines (e.g., TODO, (un)packNumber) are collected
here just to keep the total number of files low.

Note that doMakeDiskArgs(Arg * a) no longer (cf. P0) writes to
diskparams.dat

Note that mkfs subsumes the mkdisk command.  

-eof-
