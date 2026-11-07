"""Independently decode QEMU's WAV capture, including unfinalized length fields."""
import struct,wave

def verify(capture,out,source=None):
    raw=capture.read_bytes()
    assert raw[:4]==b'RIFF' and raw[8:16]==b'WAVEfmt ' and raw[36:40]==b'data'
    assert struct.unpack_from('<IHHIIHH',raw,16)==(16,1,2,48000,192000,4,16)
    assert (len(raw)-44)%4==0
    riff,data=struct.unpack_from('<I',raw,4)[0],struct.unpack_from('<I',raw,40)[0]
    assert riff in (0,len(raw)-8) and data in (0,len(raw)-44)
    # This Windows QEMU build leaves its two length fields zero on shutdown.
    # Preserve original bytes; use actual capture length, never a source fixture,
    # to decode PCM and write an explicitly derived playable WAV.
    (out/'audio-device-capture.bin').write_bytes(raw)
    pcm=raw[44:];samples=struct.unpack('<'+'h'*(len(pcm)//2),pcm)
    active=[samples[i] for i in range(0,len(samples),2) if abs(samples[i])>50]
    assert len(active)>15000 and max(active)>5000 and min(active)<-5000
    assert all(samples[i]==samples[i+1] for i in range(0,len(samples),2))
    if source:
        with wave.open(str(source),'rb') as expected:
            assert expected.getnchannels()==1 and expected.getsampwidth()==2 and expected.getframerate()==48000
            mono=expected.readframes(expected.getnframes())
        stereo=b''.join(mono[i:i+2]*2 for i in range(0,len(mono),2))
        assert stereo in pcm,'Uninterrupted playback did not emit the complete original PCM waveform'
    with wave.open(str(out/'audio-output.wav'),'wb') as decoded:
        decoded.setnchannels(2);decoded.setsampwidth(2);decoded.setframerate(48000);decoded.writeframes(pcm)
    return {'sample_rate':48000,'channels':2,'active_frames':len(active),'frames':len(samples)//2,
            'raw_riff_length':riff,'raw_data_length':data,'decoded_from_actual_capture_bytes':True}
