"""Minimal RFC 6143 client restricted to authenticated owned loopback VM fixtures.

This is a test adapter for a newly created headless VM, never a host desktop
connection. It does not become an Engine OS networking dependency.
"""
import socket
import struct
import time


class RFB:
    def __init__(self,port,password):
        from cryptography.hazmat.primitives.ciphers import Cipher,modes
        from cryptography.hazmat.decrepit.ciphers.algorithms import TripleDES
        self.socket=socket.create_connection(('127.0.0.1',port),timeout=5)
        self.socket.settimeout(5)
        try:
            version=self.read(12);assert version.startswith(b'RFB 003.')
            modern=int(version[8:11])>=8;self.socket.sendall(b'RFB 003.008\n' if modern else b'RFB 003.003\n')
            if modern:
                count=self.read(1)[0];assert count and count<=32
                methods=self.read(count);assert 2 in methods,'Owned fixture must require its ephemeral VNC password'
                self.socket.sendall(b'\x02')
            else:assert struct.unpack('!I',self.read(4))[0]==2
            challenge=self.read(16)
            assert len(password)==8
            key=bytes(int(f'{value:08b}'[::-1],2) for value in password.encode('ascii'))
            encryptor=Cipher(TripleDES(key*3),modes.ECB()).encryptor()
            self.socket.sendall(encryptor.update(challenge)+encryptor.finalize())
            assert struct.unpack('!I',self.read(4))[0]==0,'Owned fixture authentication failed'
            self.socket.sendall(b'\x01')
            initial=self.read(24);self.width,self.height=struct.unpack_from('!HH',initial)
            length=struct.unpack_from('!I',initial,20)[0];assert length<=65536;self.read(length)
            assert 320<=self.width<=4096 and 200<=self.height<=2160
            pixel=struct.pack('!BBBBHHHBBBxxx',32,24,0,1,255,255,255,16,8,0)
            self.socket.sendall(b'\x00\x00\x00\x00'+pixel)
            self.socket.sendall(struct.pack('!BBHi',2,0,1,0))
        except Exception:self.close();raise

    def read(self,count):
        data=bytearray()
        while len(data)<count:
            part=self.socket.recv(count-len(data))
            if not part:raise EOFError('Owned RFB connection closed')
            data.extend(part)
        return bytes(data)

    def close(self):self.socket.close()

    def shot(self,path):
        from PIL import Image
        image=Image.new('RGB',(self.width,self.height))
        self.socket.sendall(struct.pack('!BBHHHH',3,0,0,0,self.width,self.height))
        while True:
            kind=self.read(1)[0]
            if kind==0:
                _,count=struct.unpack('!BH',self.read(3));assert count<=4096
                for _ in range(count):
                    x,y,width,height,encoding=struct.unpack('!HHHHi',self.read(12))
                    assert encoding==0 and width<=self.width-x and height<=self.height-y
                    raw=self.read(width*height*4)
                    image.paste(Image.frombytes('RGB',(width,height),raw,'raw','BGRX'),(x,y))
                image.save(path);return
            if kind==2:continue
            if kind==3:
                header=self.read(7);length=struct.unpack_from('!I',header,3)[0];assert length<=1024**2;self.read(length);continue
            raise AssertionError('Unsupported owned RFB server message '+str(kind))

    def key(self,value,down):self.socket.sendall(struct.pack('!BBHI',4,int(down),0,value))

    def press(self,value):
        self.key(value,True);time.sleep(.07);self.key(value,False);time.sleep(.08)

    def control(self,value):
        self.key(0xffe3,True);self.press(ord(value));self.key(0xffe3,False);time.sleep(.1)
