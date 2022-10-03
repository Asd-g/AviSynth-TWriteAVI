# ############
# DB_AppendFileName.Avs, by StainlessS. RT_Stats DataBase Utility Functions.
# Requires:
#    GScript,  (c) Gavino      http://forum.doom9.org/showthread.php?t=147846
#    RT_Stats, (c) StainlessS  http://forum.doom9.org/showthread.php?t=165479
# ############
Function DB_AppendFileName(String DB,String FileName,Int "FileNameField",String "Log",Bool "Append",Bool "Debug",Bool "Verbose",Bool "MustExist") {
/*
    DB_AppendFileName(), Given a DB Dbase, appends (ie adds records) and fills one field of each new record with a filename.
        The remainder of the fields receive null data, ie Int=0, Float=0.0, Bool=False, String="", Bin=0.
        The filenames added will be converted to Full Path Filenames if they are not already.
    DB, Name of DBase file.
    FileName, can be a Chr(10) separated list of filenames where each separate filename will be added to the Dbase.
    FileNameField, Default 0. The field number in YOUR DBase that will hold the filenames you will add to the DBase.
    Log, Default "" (not used). If filename given then outputs info to that file.
    Debug, Default False. When True, output info to DebugView Window (Google).
    Verbose, Default False. When True, adds extra info to logs and debug output.
    MustExist, Default True. If True then the filesnames you add must already exist, else will throw an error.
    Returns the number of filenames added to the DBase.

    Example:
        RT_DBaseAlloc(DB,0,"s1024if")  # Allocate DBase file named DB, with 0 records pre-allocated, 3 fields of type String[1024], Int, Float.
        Files = "Text_1.Txt" + Chr(10) + "Text_2.Txt" + Chr(10)         # Two file names we will add to 1st field 0 (string, max len=1024).
        DB_AppendFileName(DB,Files,FileNameField=0,MustExist=False)     # Append two records each with file names that do not have to exist.
*/
    myName = "DB_AppendFileName: "
    IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
    Assert(IsAvsPlus || HasGScript,RT_String("%sNeed either GScript or AVS+",myName))
    Assert(Exist(DB),myName+"'"+DB+"' Does not Exist")
    records=RT_DBaseRecords(DB)     Fields=RT_DBaseFields(DB)
    Log=Default(Log,"")
    Append=Default(Append,True)
    (!Append && Log!="" && Exist(Log)) ? RT_FileDelete(Log) : NOP
    Debug=Default(Debug,False)
    Verbose=Default(Verbose,False)
    MustExist=Default(MustExist,True)
    GS="""
        FileNameField=Default(FileNameField,0)
        Assert(FileNameField>=0 && FileNameField<Fields,RT_String("%s FileNameField %d Does NOT Exist",myName,FileNameField))
        Assert(RT_DBaseFieldType(DB,FileNameField)==3,RT_String("%s FileNameField %d is NOT String",myName,FileNameField))
        NFILES=RT_TxtQueryLines(FileName)                                           # Query Number of lines in String ie number of FileNames.
        if(Debug || Log!="") {
            S=RT_String("Writing %d files",NFiles)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
        }
        Count = 0
        for(i=0,NFILES-1) {
            FN=RT_TxtGetLine(FileName,i)                                            # file name
            if(!Exist(FN)) {
                if(Verbose && (Debug || Log!="")) {
                    S=RT_String("%d/%d ] *** File Does NOT Exist *** '%s'",i+1,NFILES,FN)
                    (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
                    (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
                }
                Assert(!MustExist,myName+String(i)+" ] '"+FN+"' Does not Exist")
            }
            FN=RT_FilenameSplit(FN)                                             # convert to full path name (also convert / to \ )
            if(Verbose && (Debug || Log!="")) {
                S=RT_String("%d/%d ] DBaseAppend(%d) = '%s'",i+1,NFILES,records+i,FN)
                (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            }
            RT_DBaseExtend(DB,1)                                                # Add a record with null fields
            RT_DBaseSetField(DB,records+i,FileNameField,FN)                     # Set only the selected FileName string field
            Count = Count + 1
        }
    """
    HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)
    Return Count
}


Function DB_AppendFilenamesFromDir(String DB,String "Dir",String "WildCard",Int "FileNameField",
            \ String "Log",Bool "Append",Bool "Debug",Bool "Verbose") {
/*
    DB_AppendFilenamesFromDir(), Given a DB Dbase, appends (ie adds records) and fills one field of each new record with a filename.
        The remainder of the fields receive null data, ie Int=0, Float=0.0, Bool=False, String="", Bin=0.
        The filenames added will be converted to Full Path Filenames if they are not already.
    DB, Name of DBase file.
    Dir, Default "" = Current directory. Dir can be a Chr(10) separated list of directories where filenames from each directory can be
        added to the Dbase. All directories must exist else error.
    WildCard, Default "*.*", matches everything. Used to filter and select files in a directory.
      Eg, "F*d.jpg" or "Fr?d.jp*g". where the '?' character matches any single character and the '*' char matches zero or more characters.
      Can take multiple pipe ('|') separated Wilcards in the extension ONLY, as in "*.BMP|JP*G|PNG"
    FileNameField, Default 0. The field number in YOUR DBase that will hold the filenames you will add to the DBase.
    Log, Default "" (not used). If given filename then outputs info to that file.
    Debug, Default False. When True, output info to DebugView Window (Google).
    Verbose, Default False. When True, adds extra info to logs and debug output.
    Returns the number of filenames added to the DBase.

    Example:
        RT_DBaseAlloc(DB,0,"s1024if")  # Allocate DBase file named DB, with 0 records pre-allocated, 3 fields of type String[1024], Int, Float.
        Dirs = "Dir_1" + Chr(10) + "Dir_2" + Chr(10)                    # Two Directories containing files we will add to the Dbase.
        DB_AppendFileName(DB,Dirs,"*.AVI|AVS",FileNameField=0)          # Append 1 record for each file in the two directories that match wildcard.
*/
    myName = "DB_AppendFileNamesFromDir: "
    IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
    Assert(IsAvsPlus || HasGScript,RT_String("%sNeed either GScript or AVS+",myName))
    STIM = RT_TimerHP()
    Assert(Exist(DB),myName+"'"+DB+"' Does not Exist")
    records=RT_DBaseRecords(DB)     Fields=RT_DBaseFields(DB)
    Directories=Default(Dir,"")                                             # default "" converted to current dir
    WildCard=Default(WildCard,"*.*")                                        # default match everything
    FileNameField=Default(FileNameField,0)
    Log=Default(Log,"")
    Append=Default(Append,True)
    (!Append && Log!="" && Exist(Log)) ? RT_FileDelete(Log) : NOP
    Debug=Default(Debug,False)
    Verbose=Default(Verbose,False)
    GS="""
        if(Debug || Log!="") {
            S=RT_String("DB Name      = '%s'",DB)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            S=RT_String("Records      = %d",records)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            S=RT_String("Fields       = %d",fields)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            TYPENAMES=RT_String("Bool\nInt\nFloat\nString\nBin\n")
            for(i=0,Fields - 1) {
                TYPE_S=RT_TxtGetLine(TYPENAMES,RT_DBaseFieldType(DB,i))
                S =RT_String("  %2d ) Type = %d(%6s) Size = %d",i,RT_DBaseFieldType(DB,i),TYPE_S,RT_DBaseFieldSize(DB,i))
                (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            }
        }
        Assert(FileNameField>=0 && FileNameField<Fields,RT_String("%s FileNameField %d Does NOT Exist",myName,FileNameField))
        Assert(RT_DBaseFieldType(DB,FileNameField)==3,RT_String("%s FileNameField %d is NOT String",myName,FileNameField))
        Directories = (Directories=="") ? "." : Directories
        DIR=""
        NDIRS=RT_TxtQueryLines(Directories)                                    # Query Number of lines in String ie number of Directories.
        for(i=0,NDIRS-1) {
            DN=RT_TxtGetLine(Directories,i)                                    # Directory name
            DN = (DN=="") ? "." : DN                                           # Current Dir
            DN=RT_FilenameSplit(DN)                                            # convert to full path name (convert / to \ )
            While(RightStr(DN,1)=="\") { LeftStr(DN,Strlen(DN)-1) }            # Strip any trailing slash else Exist fails
            Assert(Exist(DN),myName+String(i)+" ] '"+DN+"' Does not Exist")
            DN = DN + "\"                                                      # Append Backslash
            DIR = RT_TxtAddStr(DIR,DN)
        }
        if(Debug || Log!="") {
            S=RT_String("Searching %d Directories for files matching '%s'",NDIRS,WildCard)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
        }
        Count = 0
        for(D = 0,NDIRS-1) {
            DN=RT_TxtGetLine(DIR,D)                                                 # Directory name
            TMPLIST = "~TEMP_"+RT_LocalTimeString()+".txt"
            if(Verbose && (Debug || Log!="")) {
                S=RT_String("%d/%d Searching for files in '%s'",D+1,NDIRS,DN)
                (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            }
            N=RT_WriteFileList(DN+WILDCARD,TMPLIST)                                 # Create a listing movie files
            if(Debug || Log!="") {
                S=RT_String("%d/%d Found %d Files in %s",D+1,NDIRS,N,DN)
                (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            }
            if(N>0) {
                FLIST=RT_ReadTxtFromFile(TMPLIST)                                   # Get list of files
                Count = Count + DB_AppendFileName(DB,FLIST,FileNameField,Log=Log,Append=True,debug=Debug,Verbose=Verbose,MustExist=True)
            }
            if(Verbose && (Debug || Log!="")) {
                S=RT_String("%d/%d Deleting Temp FileList %s",D+1,NDIRS,TMPLIST)
                (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            }
            RT_FileDelete(TMPLIST)
        }
        if(Debug || Log!="") {
            S=RT_String("Total Files Added to DBase = %d",Count)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
            ETIM = RT_TimerHP()
            S=RT_String("Time = %f secs",ETIM-STIM)
            (Debug)     ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="")   ? RT_TxtWriteFile(myName+S,LOG,Append=True) : NOP
        }
    """
    HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)
	Return Count
}
