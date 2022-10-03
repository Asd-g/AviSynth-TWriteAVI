# 1_CreateDemoSource.avs

# Requires TWriteAVI v2.0, RT_Stats and GScript

# YOU NEED to create an 'AVS' directory in current directory for this DEMO.

INFILE = "D:\BBT.AVI"                                # at least 2,500 frames (only 1500 used)
V=AviSource(INFILE).Trim(1000,0).ConvertToYV12()

# We create source DEMO files from INFILE.

FOURCC = "ULY0"                                     # We're gonna use UT_Video YV12 rec601 codec
# Make AVI TestClips in AVS directory, different sizes.
F1 = V.Trim(0000,-500).BilinearResize(800,600).TWriteAVI(".\AVS\File_000001.AVI",Overwrite=true,FourCC=FOURCC)
F2 = V.Trim(0500,-500).BilinearResize(600,500).TWriteAVI(".\AVS\File_000002.AVI",Overwrite=true,FourCC=FOURCC)
F3 = V.Trim(1000,-500).BilinearResize(720,600).TWriteAVI(".\AVS\File_000003.AVI",Overwrite=true,FourCC=FOURCC)
ForceProcessAVI(F1)     # Forced write demo AVI clips
ForceProcessAVI(F2)
ForceProcessAVI(F3)

# Now, Make AVS TEST scripts in AVS directory
IMPORT_TMPT = """
    FN = "File_%06d.AVI"
    FAV_COLOR="%s"
    o=AVISource(FN)
    #
    Global GLB_SPLICE_WIDTH  = (GLB_SPLICE_WIDTH ==0) ? Width  : GLB_SPLICE_WIDTH       # Usage of Global Header
    Global GLB_SPLICE_HEIGHT = (GLB_SPLICE_HEIGHT==0) ? Height : GLB_SPLICE_HEIGHT
    Spline36Resize(o,GLB_SPLICE_WIDTH,GLB_SPLICE_HEIGHT)
    #                                                                                   # Passing Private Dbase to Main DB
    # Field 0 string[256], Field 1 string[256], Field 2 String[1024], Field 3 Int, Field 4 int.
    DB_TYPESTRING   = "sss1024ii"                                                       
    Global GLB_CLIPDBASE=ScriptName+".DB"                                               # Global to make available via main DBase
    RT_DBaseAlloc(GLB_CLIPDBASE,1,DB_TYPESTRING)                                        # 1 Record (record 0), 5 Fields
    TIM = RT_LocalTimeString(File=False)                                                # What time is it ?
    RT_DBaseSet(GLB_CLIPDBASE,0,FAV_COLOR,TIM,FN,o.Width,o.Height)                      # Record 0, Fill all 5 fields
    Return Last
    #
"""

RT_WriteFile("AVS\1.avs",IMPORT_TMPT,1,"!Red")
RT_WriteFile("AVS\2.avs",IMPORT_TMPT,2,"4Green")
RT_WriteFile("AVS\3.avs",IMPORT_TMPT,3,"9Blue")

MessageClip("CreateDemoSource: All Done")
