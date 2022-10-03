FN=".\OUT.AVI"                            # Written by TWriteAVI
Avisource(FN)
#DirectShowSource(FN,Pixel_Type="YV24")

Global OPT_UseWaveExtensible = (AudioChannels>2||AudioBits>16)      # If more than 2 channels or > 16 bit, set true.
Global OPT_AllowFloatAudio   = (IsAudioFloat)                       # Must be set true to play in eg Media Player Classic - Home Cinema
                                                                    # Otherwise Avisynth output converted to 16 bit.

Return Last #.Info.ConvertToRGB32 #.ConvertAudioTo16bit()   # MPC-HC (recent versions) play YV24 and Float Audio OK.

# See properties in MPC-HC to verify what it is playing


 
