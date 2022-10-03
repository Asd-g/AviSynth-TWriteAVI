# ############
# DB_FindTrim.avs
# DB_FindTrim Returns clip (or record number) containing movie frame Frame.
#   Search DBase for the record (clip) that contains the arg Frame, using Binary Search.
#   S_Field is the DB field that contains the first frame of the record clip.
#   E_Field is the DB field that contains the Last frame of the record clip.
#   Return of -1 = NOT FOUND.
# ############

Function DB_FindTrim(String DB,Int S_Field,Int E_Field,Int Frame) {
    IsAvsPlus=(FindStr(UCase(versionString),"AVISYNTH+")!=0) HasGScript=RT_FunctionExist("GScript")
    Assert(IsAvsPlus || HasGScript,RT_String("DB_FindTrim: Need either GScript or AVS+"))
    result = -1                                                     # Init NOT FOUND
    low     = 0
    high    = RT_DBaseRecords(DB) - 1
    GS="""
        while(low <= high) {
            mid = (low + high) / 2
            if(RT_DBaseGetField(DB,mid,E_Field) < Frame) {
                low = mid + 1
            } Else If (RT_DBaseGetField(DB,mid,S_Field) > Frame) {
                high = mid - 1
            } Else {
                low = high + 1                                      # Force exit
                Result = mid
            }
        }
    """
    HasGScript ? GScript(GS) : Eval(GS)   # Use GSCript if installed (loaded plugs override builtin)
    return result
}
