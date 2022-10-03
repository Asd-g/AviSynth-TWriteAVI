# 2_MakeDEMO.avs
#
import("..\LIBS\DB_AppendFilename.avs")
import("..\LIBS\DB_SpliceClips.avs")
#
#We create the demo using the prefabricated source files (AVI + AVS) in AVS directory (created by 1_CreateDemoSource.avs).

OUTAVI          = "DEMO.AVI"
DB              = "DEMO.DB"
DB_TYPESTRING   = "s1024iis1024"      # 1 String[1024] Filename, 2 Int fields (Start,End frames in final movie), 3 string[1024] client DB Filename
DB_FILENAMEFIELD= 0
GLOBAL_HEADER   = "GlobalHeader.avs"

DIR_STRING = "AVS"
WILDCARD   = "*.AVS"                                         # Whatever
LOG_FN     = "MakeDEMO.LOG"
FOURCC     = "ULY0"                                          # We're gonna use UT_Video YV12 rec601 codec

myName="MakeDEMO: "
(Exist(LOG_FN)) ? RT_FileDelete(LOG_FN) : NOP
DLIST=DIR_STRING
RT_DBaseAlloc(DB,0,DB_TYPESTRING)               # Allocate Dbase, 0 Records
DB_AppendFileNamesFromDir(DB,DLIST,Wildcard,DB_FILENAMEFIELD,Log=LOG_FN,Debug=True,Verbose=True)
DB_SpliceClips(DB,OUTAVI,StartFrameField=1,EndFrameField=2,ClipDBField=3,OpenMax=24,Pass1Max=1,
    \ FourCC=FOURCC,Log=LOG_FN,Append=True,Debug=True,Verbose=True,Global_Header=GLOBAL_HEADER)

MessageClip(myName+"All Done")
