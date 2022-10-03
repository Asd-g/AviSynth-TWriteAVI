# 2_SpliceClips.avs

import("..\LIBS\DB_AppendFilename.avs")
import("..\LIBS\DB_SpliceClips.avs")
import("..\LIBS\DB_FindTrim.avs")

# View Progress via DebugView

OUTAVI          = "Movie.AVI"                    # Spliced output file
DB              = "Movie.DB"
DB_TYPESTRING   = "s1024ii"                      # 1 String[1024] Filename, 2 Int fields (Start,End frames in final movie)
DB_FILENAMEFIELD= 0
GLOBAL_HEADER   = ""                             # Not needed for this demo (all same size etc)

#Directory Names in String pseudo array
DLIST="DIR0"+Chr(10)+"DIR1"+Chr(10)+"DIR2"+Chr(10)+"DIR3"+Chr(10)+"DIR4"+Chr(10)
# OR DLIST=RT_String("DIR0\nDIR1\nDIR2\nDIR3\nDIR4\n")

WILDCARD = "*.AVI"                                             # Whatever
LOG_FN   = "MakeDB.LOG"
FOURCC   = "ULY0"                                              # Video Codec FOURCC (Lossless UT_Video rec601 YV12)

##################
    myName="SpliceClips: "
    (Exist(LOG_FN)) ? RT_FileDelete(LOG_FN) : NOP
    RT_DBaseAlloc(DB,0,DB_TYPESTRING)               # Allocate Dbase, 0 Records
    DB_AppendFileNamesFromDir(DB,DLIST,Wildcard,DB_FILENAMEFIELD,Log=LOG_FN,Debug=True,Verbose=True)
    DB_SpliceClips(DB,OUTAVI,StartFrameField=1,EndFrameField=2,ClipDBField=-1,OpenMax=24,Pass1Max=24,
        \ FourCC=FOURCC,Log=LOG_FN,Append=True,Debug=True,Verbose=True,Global_Header=GLOBAL_HEADER)

MessageClip(myName+"All Done")

