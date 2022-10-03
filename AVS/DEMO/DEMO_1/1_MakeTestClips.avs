# 1_MakeTestClips.avs

# Requires TWriteAVI v2.0, RT_Stats and GScript


# Requires 5 empty directories in current Directory. DIR0, DIR1, DIR2, DIR3, DIR4
# View Progress via DebugView

#Avisource("E:\V\StarWars.avi")      # YV12,YUY2,RGB - (with audio)
Avisource("D:\BBT.avi")                # YV12,YUY2,RGB - (with audio)
###
ConvertToYV12()                     # we're gonna use UT_Video YV12 rec601 codec
###
#ConvertAudioTo8Bit
ConvertAudioTo16Bit
#ConvertAudioTo24Bit
#ConvertAudioTo32Bit
#ConvertAudioToFloat


# Below 1000*20, (20,000) frames produced, source clip must be at least that length.
NCLIPS      =   1000            # Number of clips produced, total
CLIPLEN     =   20              # Frames Per clip

NDIRS       =   5               # Number of directories

MAXPERDIR   =   Int(Ceil(Float(NCLIPS) / NDIRS))    # max number of clips per directory

myName="MakeTestClips: "

Assert(Framecount>= NCLIPS * CLIPLEN,myName+"Not Enough Frames, need " + String(NCLIPS * CLIPLEN))

IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
Assert(IsAvsPlus || HasGScript,RT_String("%sNeed either GScript or AVS+",myName))


GS="""
    For(i=0,NCLIPS-1) {
        FN=RT_String("DIR%d\\File_%06d.AVI",i/MAXPERDIR,i)
        RT_DebugF("%d ] Name=%s",i,FN,name=myName)              # Output to DebugView (Google)
        c=Trim(i*CLIPLEN,-CLIPLEN)
        o=TWriteAVI(c,FN,True,True,FourCC="ULY0")
        o.ForceProcessAVI()
        o = 0                                       # Close clip and call clip Destructor, Not Necessary
    }
"""
HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)
MessageClip(myName+"All Done")
