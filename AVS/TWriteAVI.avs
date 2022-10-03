#Avisource("E:\V\StarWars.avi").Trim(35000,-(60*25))
Avisource("D:\G.avi")
################### YV411 NOT SUPPORTED IN TWriteAVI
#ConvertToRGB24
#ConvertToRGB32
#ConvertToYUY2
ConvertToYV12
#ConvertToY8        # v2.6 Avisynth and dll Only
#ConvertToYV16      # Ditto
#ConvertToYV24      # Ditto
###################
# TWriteAvi(clip c,String filename,bool "overwrite"=false,bool "showall"=false,string "fourcc"="",int "dwChannelMask"=-1)
FN="OUT.AVI"
#FourCC = "ULY0"    # UT_Video YV12 rec601
FourCC = ""
dwChannelMask=-1    # DEFAULT = -1 (No Override, Use standard mapping as for v2.6 OPT_dwChannelMask)
                    # 0 = Non Located ie multiple MONAURAL channels.
                    # Otherwise, need 1 bit set for each valid channel, see dwChannelMask flags at below link.
                    #    https://msdn.microsoft.com/en-us/library/windows/desktop/dd390971%28v=vs.85%29.aspx
                    #    Or in brief defaults here on Wiki:- http://avisynth.nl/index.php/Internal_functions#OPT_dwChannelMask
                    #    Or in full, https://msdn.microsoft.com/en-us/library/windows/hardware/dn653308%28v=vs.85%29.aspx

#ConvertAudioTo8Bit
ConvertAudioTo16Bit
#ConvertAudioTo24Bit
#ConvertAudioTo32Bit
#ConvertAudioToFloat
#return last

 # Will auto close output file on last frame, will write only strictly sequential frames.
TwriteAvi(Last,FN,Overwrite=true,FourCC=FourCC,dwChannelMask=dwChannelMask)
ForceProcessAVI(Last) # Force writing AVI file.
Avisource(FN)         # Open force written clip.
#DirectShowSource(FN,Pixel_Type="YV24")

Global OPT_UseWaveExtensible = (AudioChannels>2||AudioBits>16)      # If more than 2 channels or > 16 bit, set true.
Global OPT_AllowFloatAudio   = (IsAudioFloat)                       # Must be set true to play in eg Media Player Classic - Home Cinema
                                                                    # Otherwise Avisynth output converted to 16 bit.

# See properties in MPC-HC to verify what it is playing
Return Last #.Info.ConvertToRGB32 #.ConvertAudioTo16bit()           # MPC-HC (recent versions) play YV24 and Float Audio OK. 
Return MessageClip("DONE")                                          # Without view clip, only ForceProcessAVI usage.


