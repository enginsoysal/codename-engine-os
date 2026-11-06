"""Owned local test workspaces and verified sparse Windows files, never block devices."""
import ctypes as c
from ctypes import wintypes as w
import msvcrt,os,re,subprocess,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
BASE=Path(os.environ.get('LOCALAPPDATA',str(Path.home()/'AppData/Local')))/'EngineOS/Tests/M6'
api=c.WinDLL('kernel32',use_last_error=True)
api.SetFilePointerEx.argtypes=[w.HANDLE,c.c_longlong,c.c_void_p,w.DWORD];api.SetFilePointerEx.restype=w.BOOL
api.SetEndOfFile.argtypes=[w.HANDLE];api.SetEndOfFile.restype=w.BOOL
api.GetCompressedFileSizeW.argtypes=[w.LPCWSTR,c.POINTER(w.DWORD)];api.GetCompressedFileSizeW.restype=w.DWORD

def new_work(suite):
    if not re.fullmatch('[a-z][a-z0-9-]{0,31}',suite):raise ValueError('Invalid test suite')
    path=BASE/suite/uuid.uuid4().hex;path.mkdir(parents=True)
    assert path.resolve().is_relative_to(BASE.resolve());return path

def allocated(path):
    high=w.DWORD();c.set_last_error(0);low=api.GetCompressedFileSizeW(str(path),c.byref(high))
    if low==0xffffffff and c.get_last_error():raise c.WinError(c.get_last_error())
    return (high.value<<32)|low

def sparse_disk(path,size):
    path=Path(path);assert (path.resolve().is_relative_to(BASE.resolve()) or path.resolve().is_relative_to((ROOT/'build/m6/vmware').resolve())) and size>0
    with path.open('xb'):pass
    subprocess.run(['fsutil','sparse','setflag',str(path)],check=True,capture_output=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    with path.open('r+b',buffering=0) as file:
        handle=w.HANDLE(msvcrt.get_osfhandle(file.fileno()))
        if not api.SetFilePointerEx(handle,size,None,0) or not api.SetEndOfFile(handle):raise c.WinError(c.get_last_error())
    if path.stat().st_size!=size or allocated(path)>1024*1024:raise RuntimeError('New owned test image is not genuinely sparse')
    with path.open('rb') as file:
        assert file.read(min(4096,size))==bytes(min(4096,size));file.seek(max(0,size-4096));assert file.read()==bytes(min(4096,size))
    return path

def redact(text):
    for path,marker in [(ROOT,'<project-root>'),(BASE,'<test-root>')]:
        for value in [str(path),path.as_posix(),str(path).replace('\\','\\\\')]:text=text.replace(value,marker)
    return text

if __name__=='__main__':
    work=new_work('sparse-probe');path=sparse_disk(work/'owned.img',32*1024**2)
    print('Owned sparse test file PASS: logical='+str(path.stat().st_size)+' allocated='+str(allocated(path)))
