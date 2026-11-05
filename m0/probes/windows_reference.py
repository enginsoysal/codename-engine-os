"""Additional Windows-host behavior references; no claims about guest implementation."""
import ctypes
from ctypes import wintypes as w
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    k = ctypes.WinDLL('kernel32', use_last_error=True)
    k.CreateFileW.argtypes = [w.LPCWSTR,w.DWORD,w.DWORD,ctypes.c_void_p,w.DWORD,w.DWORD,w.HANDLE]
    k.CreateFileW.restype = w.HANDLE
    k.CloseHandle.argtypes = [w.HANDLE]
    k.CloseHandle.restype = w.BOOL
    k.MoveFileExW.argtypes = [w.LPCWSTR,w.LPCWSTR,w.DWORD]
    k.MoveFileExW.restype = w.BOOL
    k.DeleteFileW.argtypes = [w.LPCWSTR]
    k.DeleteFileW.restype = w.BOOL
    k.ReadFile.argtypes = [w.HANDLE,ctypes.c_void_p,w.DWORD,ctypes.POINTER(w.DWORD),ctypes.c_void_p]
    k.ReadFile.restype = w.BOOL
    checks = []
    def check(name, value, detail=None):
        checks.append({'name':name,'passed':bool(value),'detail':detail})
    invalid = ctypes.c_void_p(-1).value
    def openfile(path,sharing):
        handle = k.CreateFileW(str(path),0x80000000,sharing,None,3,0x80,None)
        if handle == invalid:
            raise ctypes.WinError(ctypes.get_last_error())
        return handle
    with tempfile.TemporaryDirectory(prefix='engine-m0-reference-') as temp:
        d=Path(temp)
        original, renamed = d/'naïef-文件.txt', d/'hernoemd-文件.txt'
        original.write_bytes(b'Engine OS reference')
        handle = openfile(original,1)
        try:
            ctypes.set_last_error(0)
            moved = k.MoveFileExW(str(original),str(renamed),0)
            error = ctypes.get_last_error()
            check('rename blocked without delete sharing',not moved and error==32,{'error':error})
        finally:
            k.CloseHandle(handle)
        check('Unicode rename succeeds after close', k.MoveFileExW(str(original),str(renamed),0)
              and renamed.read_bytes()==b'Engine OS reference')
        handle = openfile(renamed,1|2|4)
        try:
            deleted = bool(k.DeleteFileW(str(renamed)))
            buffer=ctypes.create_string_buffer(64)
            count=w.DWORD()
            read=bool(k.ReadFile(handle,buffer,64,ctypes.byref(count),None))
            check('delete-shared open file remains readable',deleted and read and buffer.raw[:count.value]==b'Engine OS reference')
        finally:
            k.CloseHandle(handle)
        check('deleted name gone after final close',not renamed.exists())
        lower=d/'caseprobe.txt'; lower.write_bytes(b'case')
        uppercase=d/'CASEPROBE.TXT'
        # Documents this directory's behavior; a case-sensitive reference dir must not be misreported.
        check('reference directory case-insensitive lookup',uppercase.exists() and uppercase.read_bytes()==b'case')
        composed, decomposed=d/'é.txt',d/'e\u0301.txt'
        composed.write_bytes(b'composed'); decomposed.write_bytes(b'decomposed')
        check('Unicode normalization names remain distinct',composed.read_bytes()==b'composed' and decomposed.read_bytes()==b'decomposed')
    CALLBACK=ctypes.WINFUNCTYPE(ctypes.c_long,ctypes.c_void_p)
    seen=[]
    code=0xe0420001
    @CALLBACK
    def handler(pointer):
        record=ctypes.cast(pointer,ctypes.POINTER(ctypes.c_void_p))[0]
        observed=ctypes.cast(record,ctypes.POINTER(w.DWORD))[0]
        if observed==code:
            seen.append(observed)
            return -1  # EXCEPTION_CONTINUE_EXECUTION for our continuable custom exception
        return 0
    k.AddVectoredExceptionHandler.argtypes=[w.ULONG,CALLBACK]
    k.AddVectoredExceptionHandler.restype=ctypes.c_void_p
    k.RemoveVectoredExceptionHandler.argtypes=[ctypes.c_void_p]
    k.RemoveVectoredExceptionHandler.restype=w.ULONG
    k.RaiseException.argtypes=[w.DWORD,w.DWORD,w.DWORD,ctypes.c_void_p]
    k.RaiseException.restype=None
    registration=k.AddVectoredExceptionHandler(1,handler)
    if not registration:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        k.RaiseException(code,0,0,None)
        check('continuable exception delivered to registered handler',seen==[code])
    finally:
        removed=k.RemoveVectoredExceptionHandler(registration)
    check('exception handler cleanup',removed!=0)
    result={'scope':'Windows host reference only; no guest implementation proof',
            'passed':all(c['passed'] for c in checks),'checks':checks}
    (ROOT/'evidence/M0/windows-reference-results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
    if not result['passed']:
        raise SystemExit(1)

if __name__=='__main__':
    main()
