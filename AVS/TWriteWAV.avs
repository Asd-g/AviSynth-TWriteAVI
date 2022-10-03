#Avisource("E:\V\StarWars.avi").Trim(35000,-(60*25))
Avisource("D:\G.avi")
ConvertToYV12
# TWriteWAV(clip c,String filename,bool "overwrite"=false,int "dwChannelMask"=-1)
FN="OUT.WAV"
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
Video=Last
TwriteWAV(Last,FN,Overwrite=true,dwChannelMask=dwChannelMask)
ForceProcessWAV(Last)
AUD=Wavsource(FN)       # Open force written audio
AudioDub(Video,AUD)     # Use written Audio

#Global OPT_UseWaveExtensible = (AudioChannels>2||AudioBits>16)      # If more than 2 channels or > 16 bit, set true.
#Global OPT_AllowFloatAudio   = (IsAudioFloat)                       # Must be set true to play in eg Media Player Classic - Home Cinema

Return Last.Info.ConvertAudioTo16bit()                              # Avoid problems in player 
Return MessageClip("DONE")                                          # Without view clip, only ForceProcess usage.
