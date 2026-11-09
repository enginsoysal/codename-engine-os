"""RFC 6143 rectangle assembly retains unchanged pixels between updates.
Only authenticated loopback guests created by this test may use this adapter.
"""
import struct,select
from PIL import Image

def retained(base):
 class RetainedFramebuffer(base):
  def shot(self,path):
   if not hasattr(self,'canvas75'):
    self.canvas75=Image.new('RGB',(self.width,self.height));self.coverage75=Image.new('1',(self.width,self.height),0)
   if not getattr(self,'pending9',False):
    self.socket.sendall(struct.pack('!BBHHHH',3,0,0,0,self.width,self.height));self.pending9=True
   while True:
    if not select.select([self.socket],[],[],.5)[0]:
     assert self.coverage75.getextrema()==(1,1),'No complete guest framebuffer received'
     self.canvas75.save(path);return
    kind=self.read(1)[0]
    if kind==0:
     _,count=struct.unpack('!BH',self.read(3));assert count<=4096
     for _ in range(count):
      x,y,w,h,encoding=struct.unpack('!HHHHi',self.read(12));assert encoding==0 and w<=self.width-x and h<=self.height-y
      raw=self.read(w*h*4);self.canvas75.paste(Image.frombytes('RGB',(w,h),raw,'raw','BGRX'),(x,y));self.coverage75.paste(1,(x,y,x+w,y+h))
     self.pending9=False
     assert self.coverage75.getextrema()==(1,1),'Screenshot contains pixels never supplied by the guest'
     self.canvas75.save(path);return
    if kind==2:continue
    if kind==3:
     header=self.read(7);length=struct.unpack_from('!I',header,3)[0];assert length<=1024**2;self.read(length);continue
    raise AssertionError('Unsupported owned guest update '+str(kind))
 return RetainedFramebuffer
