# 3_ShowDemo.avs

import("..\LIBS\DB_FindTrim.avs")

OUT_FN   = "DEMO.AVI"
G_DB     = "DEMO.DB"

Global  G_PREV_FRAME        = -2                # Force Init
Global  G_CURRENT_TRIM      = -1                # Init for play
Global  G_MOVIETRIM_START   = -1                # Init for play
Global  G_MOVIETRIM_END     = -1                # Init for play
Global  G_FN                = ""                # Init for play
#
Global  G_CDB               = ""                # Client DB filename
#
Global  G_FAV               = ""                # Contents of client DB
Global  G_TIM               = ""
Global  G_FNORG             = ""
Global  G_W                 = 0
Global  G_H                 = 0
#
Avisource(OUT_FN)

ScriptClip("""
    DoInit = (G_PREV_FRAME + 1 != current_frame || current_frame > G_MOVIETRIM_END)
    Global G_CURRENT_TRIM   = (DoInit) ? DB_FindTrim(G_DB,1,2,current_frame) : G_CURRENT_TRIM
    Global G_FN             = (DoInit) ? RT_DBaseGetField(G_DB,G_CURRENT_TRIM,0) : G_FN
    Global G_MOVIETRIM_START= (DoInit) ? RT_DBaseGetField(G_DB,G_CURRENT_TRIM,1) : G_MOVIETRIM_START
    Global G_MOVIETRIM_END  = (DoInit) ? RT_DBaseGetField(G_DB,G_CURRENT_TRIM,2) : G_MOVIETRIM_END
    # Client DB
    Global G_CDB            = (DoInit) ? RT_DBaseGetField(G_DB,G_CURRENT_TRIM,3) : G_CDB                # Name of Client DB
    # Read From Client DB
    Global G_FAV            = (DoInit) ? RT_DBaseGetField(G_CDB,0,0) : G_FAV                            # Fav color
    Global G_TIM            = (DoInit) ? RT_DBaseGetField(G_CDB,0,1) : G_TIM                            # Wazza tyme
    Global G_FNORG          = (DoInit) ? RT_DBaseGetField(G_CDB,0,2) : G_FNORG                          # Original Filename (avi)
    Global G_W              = (DoInit) ? RT_DBaseGetField(G_CDB,0,3) : G_W                              # Original width
    Global G_H              = (DoInit) ? RT_DBaseGetField(G_CDB,0,4) : G_H                              # Original Height
    #
    RT_SubTitle("%04d] {%d:%04d} %s\n\nClient DB=%s\n\nMy Favorite Color Is \a%s\a-\nOpen Time Was %s\nOriginal FileName = %s\n" +
        \ "Original Size=%dx%d\nOutput   Size=%dx%d",
        \ current_frame,G_CURRENT_TRIM,current_frame-G_MOVIETRIM_START,G_FN,
        \ G_CDB,
        \ G_FAV,G_TIM,G_FNORG,
        \ G_W, G_H,
        \ Width,Height
        \ )
    Global  G_PREV_FRAME = current_frame                # Remember previous frame visited for next iteration
    return Last
""")

Return Last


