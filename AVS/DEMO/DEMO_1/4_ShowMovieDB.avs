# 4_ShowMovieDB.avs
# Write contents of Moveie.DB to DebugView window (google) 

TYPENAMES=RT_String("Bool\nInt\nFloat\nString\nBin\n")

DB="MOVIE.DB"

IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
Assert(IsAvsPlus || HasGScript,RT_String("4_ShowMovieDB.avs: Need either GScript or AVS+"))

GS="""
    myName="ShowMovieDB: "
    S=DB
    RT_DebugF("\n%s\n",S)
    Records=RT_DBaseRecords(DB)
    Fields=RT_DBaseFields(DB)
    S=RT_String("Records      = %d",RT_DBaseRecords(DB))
    RT_DebugF(S)
    S=RT_String("RecordSize   = %d",RT_DBaseRecordSize(DB))
    RT_DebugF(S)
    S=RT_String("RecordsMax   = %d ($%X)",RT_DBaseRecordsMax(DB),RT_DBaseRecordsMax(DB))
    RT_DebugF(S)
    FIELDS=RT_DBaseFields(DB)
    S=RT_String("Fields       = %d",FIELDS)
    RT_DebugF(S)
    for(i=0,Fields - 1) {
        TYPE_S=RT_TxtGetLine(TYPENAMES,RT_DBaseFieldType(DB,i))
        S =RT_String("  %2d ) Type = %d(%6s) Size = %d",i,RT_DBaseFieldType(DB,i),TYPE_S,RT_DBaseFieldSize(DB,i))
        RT_DebugF("%s",S)
    }

    QUOT=Chr(34)
    for(i=0,Records-1) {
        RT_DebugF("%d] ----------------",i)
        for(j=0,fields-1) {
            Typ=RT_DBaseFieldType(DB,j)
            RT_DebugF("        %d:%d] %s%s%s",i,j,Typ==3?QUOT:"",String(RT_DBaseGetField(DB,i,j)),Typ==3?QUOT:"")
        }
    }
"""
HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)

MessageClip(myName+"All Done")
