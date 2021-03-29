/*
 * shell.C -- CEG433 File Sys Project shell
 * pmateti@wright.edu
 */

#include "fs33types.hpp"

FILE *my_stdout;
FILE *my_stdin;

extern MountEntry *mtab;
extern VNIN cwdVNIN;
FileVolume *fv;
Directory *wd;

#define nArgsMax 10
#define nPipesMax 6
#define MAXDEPTH 10
char dirs[MAXDEPTH][1024];
int cur_depth = 0;

char types[nPipesMax][1 + nArgsMax];  // +1 for \0

/* An Arg-ument for one of our commands is either a "word" (a null
 * terminated string), or an unsigned integer.  We store both
 * representations of the argument. */

class Arg {
   public:
    char *s;
    uint u;
} arg[nPipesMax][nArgsMax];

int invokeTable[nPipesMax];
const char *cmds[nPipesMax];
uint nArgs[nPipesMax];
uint nCmds;
uint bg;

uint TODO() {
    fprintf(my_stdout, "to be done!\n");
    return 0;
}

uint TODO(char *p) {
    fprintf(my_stdout, "%s to be done!\n", p);
    return 0;
}

uint isDigit(char c) { return '0' <= c && c <= '9'; }

uint isAlphaNumDot(char c) {
    return c == '.' || 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' ||
           '0' <= c && c <= '9';
}

int toNum(const char *p) {
    return (p != 0 && '0' <= *p && *p <= '9' ? atoi(p) : 0);
}

void sigint(int sig) {
    signal(sig, SIG_IGN);
    exit(0);
}

SimDisk *mkSimDisk(byte *name) {
    SimDisk *simDisk = new SimDisk(name, 0);
    if (simDisk->nSectorsPerDisk == 0) {
        fprintf(my_stdout, "Failed to find/create simDisk named %s\n", name);
        delete simDisk;
        simDisk = 0;
    }
    return simDisk;
}

void doMakeDisk(Arg *a) {
    SimDisk *simDisk = mkSimDisk((byte *)a[0].s);
    if (simDisk == 0) return;
    fprintf(my_stdout, 
        "new SimDisk(%s) = %p, nSectorsPerDisk=%d,"
        "nBytesPerSector=%d, simDiskNum=%d)\n",
        simDisk->name, (void *)simDisk, simDisk->nSectorsPerDisk,
        simDisk->nBytesPerSector, simDisk->simDiskNum);
    delete simDisk;
}

void doWriteDisk(Arg *a) {
    SimDisk *simDisk = mkSimDisk((byte *)a[0].s);
    if (simDisk == 0) return;
    char *st = a[2].s;  // arbitrary word
    if (st == 0)        // if it is NULL, we use ...
        st = "CEG-4350";
    char buf[1024];  // assuming nBytesPerSectorMAX < 1024
    for (uint m = strlen(st), n = 0; n < 1024 - m; n += m)
        memcpy(buf + n, st, m);  // fill with several copies of st
    uint r = simDisk->writeSector(a[1].u, (byte *)buf);
    fprintf(my_stdout, "write433disk(%d, %s...) == %d to Disk %s\n", a[1].u, st, r, a[0].s);
    delete simDisk;
}

void doReadDisk(Arg *a) {
    SimDisk *simDisk = mkSimDisk((byte *)a[0].s);
    if (simDisk == 0) return;
    char buf[1024];  // assuming nBytesPerSectorMAX < 1024
    uint r = simDisk->readSector(a[1].u, (byte *)buf);
    buf[10] = 0;  // sentinel
    fprintf(my_stdout, "read433disk(%d, %s...) = %d from Disk %s\n", a[1].u, buf, r,
           a[0].s);
    delete simDisk;
}

void doQuit(Arg *a) { exit(0); }

void doEcho(Arg *a) {
    fprintf(my_stdout, "%s#%d, %s#%d, %s#%d, %s#%d\n", a[0].s, a[0].u, a[1].s, a[1].u,
           a[2].s, a[2].u, a[3].s, a[3].u);
}

void doMakeFV(Arg *a) {
    SimDisk *simDisk = mkSimDisk((byte *)a[0].s);
    if (simDisk == 0) return;
    fv = simDisk->make33fv();
    fprintf(my_stdout, "make33fv() = %p, Name == %s, Disk# == %d\n", (void *)fv, a[0].s,
           simDisk->simDiskNum);

    if (fv) {
        wd = new Directory(fv, 1, 0);
        strcpy(dirs[cur_depth], "");
        cwdVNIN = mkVNIN(simDisk->simDiskNum, 1);
    }
}

void doCat() {
    char c;
    while ((c = fgetc(my_stdin)) != EOF) {
        fprintf(my_stdout, "%c", c);
    }
}

void doCopyTo(byte *from, byte *to) {
    uint r = fv->write33file(wd, to, from);
    fprintf(my_stdout, "write33file(%s, %s) == %d\n", to, from, r);
}

void doCopyFrom(byte *from, byte *to) {
    uint r = fv->read33file(wd, to, from);
    fprintf(my_stdout, "read33file(%s, %s) == %d\n", to, from, r);
}

void doCopy33(byte *from, byte *to) {
    uint r = fv->copy33file(wd, to, from);
    fprintf(my_stdout, "copy33file(%s, %s) == %d\n", to, from, r);
}

void doCopy(Arg *a) {
    byte *to = (byte *)a[0].s;
    byte *from = (byte *)a[1].s;

    if (a[0].s[0] == '@' && a[1].s[0] != '@') {
        doCopyTo(from, (to + 1));
    } else if (a[0].s[0] != '@' && a[1].s[0] == '@') {
        doCopyFrom((from + 1), to);
    } else if (a[0].s[0] != '@' && a[1].s[0] != '@') {
        doCopy33(from, to);
    } else {
        puts("Wrong arguments to cp.");
    }
}

void doLsLong(Arg *a) {
    fprintf(my_stdout, "\nDirectory listing for disk %s, cwdVNIN == 0x%0lx begins:\n",
           wd->fv->simDisk->name, (ulong)cwdVNIN);
    wd->ls(my_stdout);
    fprintf(my_stdout, "Directory listing ends.\n");
}

void doLsDir(Arg *a) {

    byte *pnm = (byte *)a[0].s;
    uint in = wd->iNumberOf(pnm);
    if (in > 0) {
        uint a = wd->nInode;
        wd = new Directory(fv, in, 0);
        fprintf(my_stdout, "\nDirectory listing for disk %s, cwdVNIN == 0x%0lx begins:\n",
               wd->fv->simDisk->name, (ulong)cwdVNIN);
        wd->ls(my_stdout);
        fprintf(my_stdout, "Directory listing ends.\n");
        wd = new Directory(fv, a, 0);
    }
}

void doRm(Arg *a) {
    uint in = wd->deleteFile((byte *)a[0].s, 1);
    fprintf(my_stdout, "rm %s returns %d.\n", a[0].s, in);
}

void doInode(Arg *a) {
    uint ni = a[0].u;

    wd->fv->inodes.show(ni, my_stdout);
}

void doFile(Arg *a) {
    byte *pnm = (byte *)a[0].s;
    uint in = wd->iNumberOf(pnm);

    if (in > 0)
        wd->fv->inodes.show(in, my_stdout);
}

void doMkDir(Arg *a) {
    byte *pnm = (byte *)a[0].s;

    uint in = wd->iNumberOf(pnm);
    if (in == 0) {
        in = wd->createFile(pnm, 1);
        fprintf(my_stdout, "created directory %s with inode %d\n", a[0].s, in);
    }
}

void doRmDir(Arg *a) { 
    byte *pnm = (byte *)a[0].s;
    uint in = wd->iNumberOf(pnm);
    uint nFiles = 0;

    Directory *d = new Directory(fv, in, 0);
    for (byte *bp = 0; (bp = d->nextName()); nFiles++);
    if (nFiles == 2) {
        wd->deleteFile(pnm, 1);
    }
    delete d;

    fprintf(my_stdout, "%d directory entries in %s.\n", nFiles, a[0].s);
}

void doPwd(Arg *a) {
    for (int i = 0; i < cur_depth; i++)
        fprintf(my_stdout, "%s/", dirs[i]);
    if (strcmp(dirs[cur_depth], "") == 0)
        fprintf(my_stdout, "%s\n", "/");
    else
        fprintf(my_stdout, "%s\n", dirs[cur_depth]);
}

void doChDir(Arg *a) {
    if (strcmp(a[0].s, "/") == 0) {
        wd = fv->root;
        cur_depth = 0;
        return;
    }

    char * base = strtok(a[0].s, "/");
    while (base != NULL) {
        uint in = wd->iNumberOf((byte *)base);
        if (strcmp(base, "..") == 0) {
            cur_depth--;
        } else {
            strcpy(dirs[++cur_depth], base);
        }
        if (in > 0)
            wd = new Directory(fv, in, 0);
        base = strtok(NULL, "/");
    }

    doPwd(a);
}

void doMv(Arg *a) {
    byte *from = (byte *)a[0].s;
    byte *to = (byte *)a[1].s;

    uint in_from = wd->iNumberOf(from);
    if (in_from == 0)
        return;
    uint in_to = wd->iNumberOf(to);
    if (fv->inodes.getType(in_from) == iTypeOrdinary) {
        if ((fv->inodes.getType(in_to) == iTypeOrdinary) || in_to == 0) {
            fv->copy33file(wd, from, to);
            wd->deleteFile(from, 1);
        } else if (fv->inodes.getType(in_to) == iTypeDirectory) {
            char from_temp[512];
            strcpy(from_temp, "temp-");
            strcat(from_temp, (char *)from);
            fv->read33file(wd, from, (byte *)from_temp);
            wd->deleteFile(from, 1);
            uint a = wd->nInode;
            wd = new Directory(fv, in_to, 0);
            fv->write33file(wd, (byte *)from_temp, from);
            unlink(from_temp);
            wd = new Directory(fv, a, 0);
        }
    } else if(fv->inodes.getType(in_from) == iTypeDirectory) {
        if (fv->inodes.getType(in_to) == iTypeDirectory) {
            // TODO mv da db (both are path names)
        }
    }
}

void doMountDF(Arg *a)  // arg a ignored
{
    TODO("doMountDF");
}

void doMountUS(Arg *a) { TODO("doMountUS"); }

void doUmount(Arg *a) { TODO("doUmount"); }

/* The following describes one entry in our table of commands.  For
 * each cmmdName (a null terminated string), we specify the arguments
 * it requires by a sequence of letters.  The letter s stands for
 * "that argument should be a string", the letter u stands for "that
 * argument should be an unsigned int."  The data member (func) is a
 * pointer to the function in our code that implements that command.
 * globalsNeeded identifies whether we need a volume ("v"), a simdisk
 * ("d"), or a mount table ("m").  See invokeCmd() below for exact
 * details of how all these flags are interpreted.
 */

class CmdTable {
   public:
    char *cmdName;
    char *argsRequired;
    char *globalsNeeded;  // need d==simDisk, v==cfv, m=mtab
    void (*func)(Arg *a);
} cmdTable[] = {{"cd", "s", "v", doChDir},
                {"cp", "ss", "v", doCopy},
                {"echo", "ssss", "", doEcho},
                {"file", "s", "v", doFile},
                {"inode", "u", "v", doInode},
                {"ls", "", "v", doLsLong},
                {"lsdir", "s", "v", doLsDir},
                {"lslong", "", "v", doLsLong},
                {"mkdir", "s", "v", doMkDir},
                {"mkdisk", "s", "", doMakeDisk},
                {"mkfs", "s", "", doMakeFV},
                {"mount", "us", "", doMountUS},
                {"mount", "", "", doMountDF},
                {"mv", "ss", "v", doMv},
                {"rddisk", "su", "", doReadDisk},
                {"rmdir", "s", "v", doRmDir},
                {"rm", "s", "v", doRm},
                {"pwd", "", "v", doPwd},
                {"q", "", "", doQuit},
                {"quit", "", "", doQuit},
                {"umount", "u", "m", doUmount},
                {"wrdisk", "sus", "", doWriteDisk}};

uint ncmds = sizeof(cmdTable) / sizeof(CmdTable);

void usage() {
    fprintf(my_stdout, "The shell has only the following cmds:\n");
    for (uint i = 0; i < ncmds; i++)
        fprintf(my_stdout, "\t%s\t%s\n", cmdTable[i].cmdName, cmdTable[i].argsRequired);
    fprintf(my_stdout, "Start with ! to invoke a Unix shell cmd\n");
}

/* pre:: k >= 0, arg[] are set already;; post:: Check that args are
 * ok, and the needed simDisk or cfv exists before invoking the
 * appropriate action. */

void invokeCmds() {
    pid_t pid = -1;
    uint ok[nPipesMax];
    for (uint i = 0; i < nPipesMax; i++) {
        ok[i] = 1;
    }
    for (uint j = 0; j < nCmds; j++) {
        int k = invokeTable[j];
        if (k == -1) {
            ok[j] = 0;
        } else if (k != -2) {
            if (cmdTable[k].globalsNeeded[0] == 'v' && cwdVNIN == 0) {
                ok[j] = 0;
                fprintf(my_stdout, "Cmd %s needs the cfv to be != 0.\n", cmdTable[k].cmdName);
            } else if (cmdTable[k].globalsNeeded[0] == 'm' && mtab == 0) {
                ok[j] = 0;
                fprintf(my_stdout, "Cmd %s needs the mtab to be != 0.\n", cmdTable[k].cmdName);
            }

            char *req = cmdTable[k].argsRequired;
            uint na = strlen(req);
            for (uint i = 0; i < na; i++) {
                if (req[i] == 's' && (arg[j][i].s == 0 || arg[j][i].s[0] == 0)) {
                    ok[j] = 0;
                    fprintf(my_stdout, "arg #%d must be a non-empty string.\n", i);
                }
                if ((req[i] == 'u') && (arg[j][i].s == 0 || !isDigit(arg[j][i].s[0]))) {
                    ok[j] = 0;
                    fprintf(my_stdout, "arg #%d (%s) must be a number.\n", i, arg[j][i].s);
                }
            }
        }
    }

    if (bg) {
        pid = fork();
        if (pid != 0) return;
    }

    if (nCmds == 1) {
        if (invokeTable[0] == -2) {
            char command[1024];
            strcpy(command, cmds[0] + 1);
            for (uint i = 0; i < nArgs[0]; i++) {
                strcat(command, " ");
                strcat(command, arg[0][i].s);
            }
            system(command);
        } else {
            if (ok[0]) (*cmdTable[invokeTable[0]].func)(arg[0]);
        }
    } else if (nCmds == 2) {
        if (invokeTable[1] == -2) {
            char command[1024];
            strcpy(command, cmds[1] + 1);
            for (uint i = 0; i < nArgs[1]; i++) {
                strcat(command, " ");
                strcat(command, arg[1][i].s);
            }
            my_stdout = popen(command, "w");
            if (my_stdout && ok[0]) {
                (*cmdTable[invokeTable[0]].func)(arg[0]);
            }
            pclose(my_stdout);
        }
    } else if (nCmds > 2) {
        TODO();
    }

    if (pid == 0) _exit(0);
}

/* pre:: buf[] is the command line as typed by the user, nArgsMax + 1 ==
 * sizeof(types);; post:: Parse the line, and set types[], arg[].s and
 * arg[].u fields.
 */

void setArgsGiven(char *buf) {
    nCmds++;
    if (nCmds == 1) {
        strtok(buf, " \t\n");  // terminates the cmd name with a \0
        cmds[nCmds - 1] = buf;
    } else {
        cmds[nCmds - 1] = strtok(0, " \t");
    }

    for (uint i = 0; i < nArgsMax;) {
        char *q = strtok(0, " \t");
        if (q == 0 || *q == 0) break;
        if (*q == '>') {
            char *filename = strtok(0, " \t");
            my_stdout = fopen(filename, "w");
            break;
        } else if (*q == '|') {
            setArgsGiven(buf);
            break;
        } else if (*q == '&') {
            bg = 1;
            break;
        }
        arg[nCmds - 1][i].s = q;
        arg[nCmds - 1][i].u = toNum(q);
        types[nCmds - 1][i] = isDigit(*q) ? 'u' : 's';
        nArgs[nCmds - 1] = ++i;
    }
}

/* pre:: name pts to the command token, argtypes[] is a string of
 * 's'/'u' indicating the types of arguments the user gave;; post::
 * Find the row number of the (possibly overloaded) cmd given in
 * name[].  Return this number if found; return -1 otherwise. */

void findCmds() {
    for (uint j = 0; j < nCmds; j++) {
        if (cmds[j][0] == '!') {
            invokeTable[j] = -2;
        } else {
            for (uint i = 0; i < ncmds; i++) {
                if (strcmp(cmds[j], cmdTable[i].cmdName) == 0 &&
                    strcmp(types[j], cmdTable[i].argsRequired) == 0) {
                    invokeTable[j] = i;
                }
            }
        }
    }
}

void ourgets(char *buf) {
    char *line = fgets(buf, 1024, stdin);
    if (line == NULL)
        exit(0);
    char *p = index(buf, '\n');
    if (p) *p = 0;
}

void resetArgs() {
    for (uint j = 0; j < nPipesMax; j++) {
        for (uint i = 0; i < nArgsMax; i++) {
            arg[j][i].s = 0;
            types[j][i] = 0;
        }
        nArgs[j] = 0;
        invokeTable[j] = -1;
        types[j][nArgsMax] = 0;
    }
    nCmds = 0;
}

int main() {
    char buf[1024];  // better not type longer than 1023 chars
    my_stdout = stdout;
    signal(SIGINT, sigint);

    usage();
    for (;;) {
        resetArgs();
        *buf = 0;                // clear old input
        my_stdout = stdout; // clear redirection
        my_stdin = stdin;
        bg = 0;
        fprintf(my_stdout, "%s", "sh33% ");  // prompt
        ourgets(buf);
        fprintf(my_stdout, "cmd [%s]\n", buf);  // just print out what we got as-is
        if (buf[0] == 0) continue;
        if (buf[0] == '#') continue;  // this is a comment line, do nothing
        else {
            setArgsGiven(buf);
            findCmds();
            invokeCmds();
        }
    }
}

// -eof-

