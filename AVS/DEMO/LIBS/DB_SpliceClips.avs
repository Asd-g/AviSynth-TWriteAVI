# ############
# DB_SpliceClips.Avs, by StainlessS. RT_Stats DataBase Utility Functions.
# Requires:
#    GScript,  (c) Gavino       http://forum.doom9.org/showthread.php?t=147846
#    RT_Stats, (c) StainlessS   http://forum.doom9.org/showthread.php?t=165479
#    TWriteAVI (c) Kevin Stone, http://forum.doom9.org/showthread.php?t=172761 (other versions may be broken)
# ############


Function DB_SpliceClips(String DB,String OUTAVI,Int "FileNameField",Int "StartFrameField",Int "EndFrameField",Int "ClipDBField",
        \ Int "OpenMAX",Int "Pass1MAX", String "FourCC",
        \ String "Log",Bool "Append",Bool "Debug",Bool "Verbose",String "OpenClip",String "Global_Header") {
/*
    DB_SpliceClips(), Splices a number of video clips together. It is normally difficult/impossible to splice more than about 25
    clips, due to system resource limitations (limit on number of open clips). This function splices them into bigger and bigger
    clips, saving each aggregate clip before re-opening & splicing them together into even bigger ones, until the whole job is done.
    The function needs a DB set up with filenames of the clips to splice, using either DB_AppendFileNamesFromDir() or DB_AppendFileName().
        It is possible to render AVS scripts into lossless AVI, and join together with others, tested up to 1000 input clips.
    You can include a Global header, so as to pass global info to a group of AVS clips, perhaps of use in conversion of clips to all
    be of similar characteristics so that they can be spliced together.
        It is also possible for an AVS file to create its own DBase and pass the filename via a global variable, into the OpenClip() function,
    and have that filename entered into the main DBase so that your private clip DBase can be accessed at any point/frame in the completed
    movie (when playing).

    #################
    DB, Name of the DBase file, already setup containing filenames to splice together, eg *.AVS,*.AVI etc files.
    OUTAVI, Filename of the output AVI file.
    FileNameField, DBase field that holds the filename of an input video clip. Default 0.
    StartFrameField, DBase field that will hold the START frame number of the input clip for current record. (relative to complete spliced movie).
        Default -1, Not used.
    EndFrameField, DBase field that will hold the END frame number of the input clip for current record. (relative to complete spliced movie).
        Default -1, Not used.
    ClipDBField, DBase field that will hold a filename of a personal DBase for the input clip. Default -1, Not used.  See DB_SpliceOpenClip().
    OpenMAX, Maximum number of clips that can be opened at once (due to system resource restrictions), Default 20.
    Pass1MAX, Maximum number of clips that can be opened at once for the original source clips only, default OpenMAX, 1 <= Pass1MAX <= OpenMAX.
        If the source clips are AVS, then they may already have many input clips open, if I/P AVS, suggest Pass1MAX = 1, where the AVS will be
    rendered into lossless AVI on first pass.
    FourCC, Default "ULY0",  Lossless UT Video YV12 rec.601. For Output AVI clip and also temporary aggregate files. (Requires UT Video codec)
    Log, Default "". LogFile Name.
    Append, Default TRUE, Append output to any currently existing log file.
    Debug, Default False. If true then output logging to DebugView window (google).
    Verbose, Default False. If true then output extra info to Log file and Debug window.
    OpenClip, Default "DB_SpliceOpenClip". Name of function to call to open a video clip or import an AVS.
       See args for DB_SpliceOpenClip(), must take identical args, none of which are optional.
    Global_Header, Default "". Filename of AVS file containing Global Variable Initialization, and perhaps LoadPlugins.
       Global args will be available to any AVS files imported in OpenClip() function, See DB_SpliceOpenClip().
    Returns the number of records (clips) in the DBase.
*/
    ### AVS Header file containing only Globals (maybe LoadPlugins too)
    ### Globals initialized and so available during AVS Import.
    myName="DB_SpliceClips: "
    IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
    Assert(IsAvsPlus || HasGScript,RT_String("%sNeed either GScript or AVS+",myName))
    Global_Header=Default(Global_Header,"")
    (Global_Header != "") ? Import(Global_Header) : NOP
    ###
    START=RT_TimerHP()
    Assert(Exist(DB),myName+"'"+DB+"' Does not Exist")
    Assert(OUTAVI!="",myName+"'"+OUTAVI+"' Need OUTAVI Name")
    Ext=RT_GetFileExtension(OUTAVI)
    Assert(Ext==".avi",myName+"'"+OUTAVI+"' Not an AVI")
    records=RT_DBaseRecords(DB)
    Fields=RT_DBaseFields(DB)
    FileNameField=Default(FileNameField,0)
    Assert(FileNameField>=0 && FileNameField<Fields,RT_String("%s FileNameField %d Does NOT Exist",myName,FileNameField))
    Assert(RT_DBaseFieldType(DB,FileNameField)==3,RT_String("%s FileNameField %d is NOT String",myName,FileNameField))
    StartFrameField=Default(StartFrameField,-1)
    Assert(StartFrameField<Fields,RT_String("%s StartFrameField %d Does NOT Exist",myName,StartFrameField))
    Assert(StartFrameField<0 || RT_DBaseFieldType(DB,StartFrameField)==1,RT_String("%s StartFrameField %d is NOT Int",myName,StartFrameField))
    EndFrameField=Default(EndFrameField,-1)
    Assert(EndFrameField<Fields,RT_String("%s EndFrameField %d Does NOT Exist",myName,EndFrameField))
    Assert(EndFrameField <0 || RT_DBaseFieldType(DB,EndFrameField)==1,RT_String("%s EndFrameField %d is NOT Int",myName,EndFrameField))
    ClipDBField=Default(ClipDBField,-1)
    Assert(ClipDBField<Fields,RT_String("%s ClipDBField %d Does NOT Exist",myName,ClipDBField))
    Assert(ClipDBField <0 || RT_DBaseFieldType(DB,ClipDBField)==3,RT_String("%s ClipDBField %d is NOT String",myName,ClipDBField))
    OpenMAX=Default(OpenMAX,20)                                                           # maximum num of clips to join at once
    Assert(OpenMAX>1,RT_String("%s OpenMAX %d MUST be greater than 1",myName,OpenMAX))
    Pass1MAX=Default(Pass1MAX,OpenMAX)                                                    # maximum num of clips to join at once, 1st PASS
    Assert(Pass1MAX>=1 && Pass1MAX <= OpenMAX,RT_String("%s 1 <= Pass1MAX(%d) <= OpenMAX(%d)",myName,Pass1MAX,OpenMAX))
    FourCC=Default(FourCC,"ULY0")                                                         # UT Video YV12 rec.601
    Log=Default(Log,"")
    Log=(Log!="") ? RT_FilenameSplit(Log) : Log                                           # convert to full path name (also convert / to \ )
    Append=Default(Append,True)
    (!Append && Log!="" && Exist(Log)) ? RT_FileDelete(Log) : NOP
    Debug=Default(Debug,False)
    Verbose=Default(Verbose,False)
    dbgS=String(Debug)  VerbS=String(VerBose)
    OpenClip=Default(OpenClip,"DB_SpliceOpenClip")
    Assert(OpenClip != "",myName+"OpenClip cannot be empty string, need a function name")
    FnFMT="""%s("%s",%i,%i,%i,%i,%i,%i,"%s",%s,%s)"""
    CompleteFrames = 0  FrameNo = 0     Pass = 0   NClips = records     TotalFramesR=0  TotalFramesW=0
    PassMAX=Pass1MAX
    GS="""
        While (NClips > 0) {
            if(NClips <= OpenMAX && PassMAX==OpenMAX) {
                ODB=""                                                                      # Last Pass, dont need Output DB
            } Else {
                ODB="~TEMP_"+RT_LocalTimeString()+".DB"
                RT_DBaseAlloc(ODB,0,"s1024")
            }
            ACC = 0    JOINED=0     OCNT=0
            for(in_i=0,NClips-1) {
                if(Pass==0) {
                    FN=RT_DBaseGetField(DB,in_i,FileNameField)
                    EVS=RT_String(FnFMT,OpenClip,DB,in_i,FileNameField,StartFrameField,EndFrameField,ClipDBField,FrameNo,Log,DbgS,VerbS)
                    in_c = Eval(EVS)                                                               # Call OpenClip
                    Len = in_c.FrameCount
                    if(Verbose && (Debug || Log!="")) {
                        S=RT_String("%d:%d ] OPENED Len=%d I/P Clip %s",Pass,in_i,Len,FN)
                        (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                        (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
                    }
                    FrameNo = FrameNo+Len
                } Else {
                    FN=RT_DBaseGetField(DB,in_i,0)                                                  # Field 0 was Filename string in Aggregate DB
                    in_c = AviSource(FN)                                                            # Always AVI
                    Len = in_c.FrameCount
                    if(Verbose && (Debug || Log!="")) {
                        S=RT_String("%d:%d ] Len=%d Aggregate Clip %s",Pass,in_i,Len,FN)
                        (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                        (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
                    }
                }
                ACC = (ACC.IsClip()) ? ACC ++ in_c  : in_c
                JOINED = JOINED + 1
                if(JOINED>=PassMAX || in_i==NClips-1) {                                    # limit or last input clip ?
                    if(ODB=="") {
                        OFN = OUTAVI                                                       # Very last FINAL OUTPUT file
                    } Else {
                        OFN = "~TEMP_"+RT_LocalTimeString()+".AVI"
                        RT_DBaseAppend(ODB,OFN)                                            # rem aggregate o/p filename
                    }
                    OCNT=OCNT+1
                    out_c=TWriteAVI(ACC,OFN,True,True,FourCC=FourCC)
                    Len = out_c.FrameCount
                    CompleteFrames = (ODB=="") ? Len : 0
                    TotalFramesR=TotalFramesR+Len
                    if(Debug || Log!="") {
                        S=RT_String("%d:%d ] Len = %d WRITING Aggregate Clip %s",Pass,OCNT,Len,OFN)
                        (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                        (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
                    }
                    out_c.ForceProcessAVI()
                    ACC = 0 in_c = 0 out_c = 0                                              # MAKE SURE each clip closed and destructor called
                    ACC = AviSource(OFN)                                                    # Always AVI
                    TotalFramesW=TotalFramesW+ACC.FrameCount
                    ACC=0                                                                   # Close it again
                    JOINED = 0
                }
            }
            if(Pass != 0 ) {
                if(Verbose && (Debug || Log!="")) {
                    S=RT_String("%d ] Deleting Aggregate Clips and DB %s",Pass,DB)
                    (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                    (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
                }
                for(deli=0,NClips-1) {RT_FileDelete(RT_DBaseGetField(DB,deli,0))}           # Delete temp aggregate files (DONT DEL ORIGINAL I/P FILES)
                RT_FileDelete(DB)                                                           # Delete aggregate i/p DB
            }
            Pass = Pass + 1
            PassMAX = OpenMAX                                                               # Change to OpenMAX for next PASS != 0
            DB=ODB                                                                          # Next I/P is OLD O/P
            NClips = (DB=="") ? 0 : RT_DBaseRecords(DB)                                     # Last Pass ?
        }
        if(Debug || Log!="") {
            T=RT_TimerHP() - START
            S=RT_String("Output AVI Length = %d Frames",CompleteFrames)
            (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
            if(TotalFramesR > 0) {
                S=RT_String("Read  %d Frames %f FPS (%f SPF)",TotalFramesR,TotalFramesR/T,T/(TotalFramesR))
                (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
            }
            if(TotalFramesW > 0) {
                S=RT_String("Write %d Frames %f FPS (%f SPF)",TotalFramesW,TotalFramesW/T,T/(TotalFramesW))
                (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
            }
            S=RT_String("Time = %f secs (%.2f mins)",T,T/60.0)
            (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
        }
    """
    HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)
    return records
}

Function DB_SpliceOpenClip(String DB,Int Record,Int FileNameField,Int StartFrameField,Int EndFrameField,Int ClipDBField,
           \ Int FrameNo, String Log,Bool Debug, Bool Verbose) {
/*
    DB_SpliceOpenClip(), Called by DB_SpliceClips (by default) to Open a video clip, either an AVS or container format.
*/
    myName="DB_SpliceOpenClip: "
    IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
    Assert(IsAvsPlus || HasGScript,RT_String("%sNeed either GScript or AVS+",myName))
    clp = 0                                                                                 # Not yet got a clip
    GS="""
        FN=RT_DBaseGetField(DB,Record,FileNameField)                                        # Get clip Filename from DBase record
        Ext=RT_GetFileExtension(FN)                                                         # Open File based on extension
        if(Debug || Log!="") {
            S=RT_String("%d ] Opening as Ext='%s' : '%s'",Record,Ext,FN)
            (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
            (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
        }
        if( Ext==".AVS" )      {
            # The Import on AVS, Allows you to join files of differing characteristics, if you eg resize everything to same size in AVS files.
            # Could eg use in DB_SpliceClips(Global_Header) arg script, intialized to Global Glb_Width=0,
            #   Then in your script set Global Glb_Width = Clip.Width when Glb_Width==0
            #   Then resize to width = Glb_Width.
            #   Or, Set Initial Global Glb_Width = RequiredWidth, and so when resized to Glb_Width, all clips would be resized to RequiredWidth.
            #   Exactly the same with other characterists that need to be identical to splice files.
            #
            # In AVS Script, You could create a clip DBase to hold whatever you want, eg subtitles FileName, original width/height,
            # what frames were trimmed off, data extracted using MediaInfo, your favorite color or anything you like.
            # This clip DBase could be eg based on the ScriptName() + ".DB". So, WhenEver you open the script, it creates its own
            # DBase with its own records and fields, fills them with whatever it wants, and then sets Global GLB_CLIPDBASE = myDBName
            # where myDBName is whatever you decided to call it (best based on script name, with full path). Can use name=RT_FilenameSplit(name)
            # to convert to full path name if not already full path.
            # Below, Eg, Set ClipDBField to hold FileName of client Clip Dbase (holding Whatever Client wants it to hold)
            Global GLB_CLIPDBASE = RT_Undefined                                 # Make Sure we dont use variable set by a different script.
            clp = Import(FN)                                                    # Script should setup its own DBase itself, storing whatever.
            if( RT_VarExist("GLB_CLIPDBASE") && IsString(GLB_CLIPDBASE) && ClipDBField>=0) {
                RT_DBaseSetField(DB,Record,ClipDBField, GLB_CLIPDBASE )
                # Now the client DBase filename is available in ClipDBField field of the clip's record in the main DB.
                if(Debug || Log!="") {
                    S=RT_String("%d ] Client clip DBase Stored '%s'",Record,GLB_CLIPDBASE)
                    (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                    (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
                }
            }
        } Else {
            # Here, open clip based on filename extension, needs to be expanded for additional source filters.
            if( Ext==".AVI" )  { clp = AviSource(FN) }                                     # Whatever, Select source filter based on Extension
            Else               { clp = DirectShowSource(FN) }
        }
        if(!clp.IsClip()) {
            S=RT_String("%d ] *** Not A Clip *** on %s",Record,FN)
            # Output to Debug even if not required.
            RT_DebugF("%s",S,name=myName)
            (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
            Assert(False,myName+S)
        }
        Len = clp.FrameCount
        if(StartFrameField>=0)  {                                                       # set Start Frame of clip relative to main movie
            if(Debug || Log!="") {
                S=RT_String("%d ] StartFrameField Set to %d",Record,FrameNo)
                (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
            }
            RT_DBaseSetField(DB,Record,StartFrameField,FrameNo)
        }
        if(EndFrameField>=0)    {                                                       # set End Frame of clip relative to main movie
            if(Debug || Log!="") {
                S=RT_String("%d ] EndFrameField   Set to %d",Record,FrameNo+Len-1)
                (Debug)   ? RT_DebugF("%s",S,name=myName) : NOP
                (Log!="") ? RT_TxtWriteFile(S,Log,Append=True) : NOP
            }
            RT_DBaseSetField(DB,Record,EndFrameField,FrameNo+Len-1)
        }
    """
    HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)
    return clp
}

