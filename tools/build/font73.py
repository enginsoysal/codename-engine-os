"""Generate bounded native coverage masks from the unchanged OFL Inter font.

Rasterize at each actual physical size. The kernel never enlarges a bitmap,
loads a font interpreter or allocates a font cache at runtime.
"""
from pathlib import Path
import hashlib
import json
from PIL import Image,ImageDraw,ImageFont,features
ROOT=Path(__file__).resolve().parents[2]
SIZES=(16,24,32,36,48,64)
def prepare(work):
    folder=ROOT/'third_party/inter';provenance=json.loads((folder/'provenance.json').read_text(encoding='utf-8'))
    for name,digest in provenance['files'].items():
        if hashlib.sha256((folder/name).read_bytes()).hexdigest()!=digest:raise ValueError('Pinned Inter source changed: '+name)
    glyphs=[];data=bytearray();faces=[]
    for weight in (400,600):
        for size in SIZES:
            if (weight==400 and size>32) or (weight==600 and size==16):
                faces.append(faces[2] if weight==400 else faces[0]);continue
            font=ImageFont.truetype(str(folder/'Inter.ttf'),size)
            font.set_variation_by_axes([min(32,max(14,size)),weight])
            ascent,descent=font.getmetrics();faces.append((len(glyphs),size,ascent,ascent+descent))
            for code in range(32,127):
                char=chr(code);x0,y0,x1,y1=font.getbbox(char,anchor='ls');w,h=x1-x0,y1-y0
                image=Image.new('L',(max(1,w),max(1,h)));ImageDraw.Draw(image).text((-x0,-y0),char,font=font,fill=255,anchor='ls')
                raw=image.tobytes() if w and h else b'';encoded=bytearray();at=0
                while at<len(raw):
                    count=1
                    while count<255 and at+count<len(raw) and raw[at+count]==raw[at]:count+=1
                    encoded.extend((count,raw[at]));at+=count
                glyphs.append((len(data),len(encoded),w,h,x0,y0,round(font.getlength(char))));data.extend(encoded)
    secondary=work.name=='m11'
    if secondary:
        # Original Inter, rasterized at actual 14/21/28 px for 100/150/200%.
        for size in (14,21,28):
            font=ImageFont.truetype(str(folder/'Inter.ttf'),size)
            font.set_variation_by_axes([min(32,max(14,size)),400])
            ascent,descent=font.getmetrics();faces.append((len(glyphs),size,ascent,ascent+descent))
            for code in range(32,127):
                char=chr(code);x0,y0,x1,y1=font.getbbox(char,anchor='ls');w,h=x1-x0,y1-y0
                image=Image.new('L',(max(1,w),max(1,h)));ImageDraw.Draw(image).text((-x0,-y0),char,font=font,fill=255,anchor='ls')
                raw=image.tobytes() if w and h else b'';encoded=bytearray();at=0
                while at<len(raw):
                    count=1
                    while count<255 and at+count<len(raw) and raw[at+count]==raw[at]:count+=1
                    encoded.extend((count,raw[at]));at+=count
                glyphs.append((len(data),len(encoded),w,h,x0,y0,round(font.getlength(char))));data.extend(encoded)
    if len(data)>1024*1024:raise ValueError('Coverage atlas exceeds its static bound')
    text='typedef struct {unsigned offset;unsigned short bytes;unsigned char width,height; signed char x,y;unsigned char advance;} font73_glyph;\n'
    text+='typedef struct {unsigned first,size,ascent,height;} font73_face;\n'
    text+='static const font73_face font73_faces[]={'+','.join('{'+','.join(map(str,v))+'}' for v in faces)+'};\n'
    text+='static const font73_glyph font73_glyphs[]={'+','.join('{'+','.join(map(str,v))+'}' for v in glyphs)+'};\n'
    text+='static const unsigned char font73_data[]={'+','.join(map(str,data))+'};\n'
    text+='static const char font73_license[]={'+','.join(map(str,(folder/'OFL.txt').read_bytes()+b'\0'))+'};\n'
    import PIL
    lock=json.loads((folder/('raster-lock11.json' if secondary else 'raster-lock.json')).read_text())
    actual={'pillow':PIL.__version__,'freetype':features.version('freetype2'),'header_sha256':hashlib.sha256(text.encode()).hexdigest()}
    if actual!=lock:raise ValueError('Rasterizer/output differs from reviewed lock: '+str(actual))
    (work/'font73.h').write_bytes(text.encode())
    import PIL
    report={'font':'Inter','license':'OFL-1.1','source_sha256':provenance['files']['Inter.ttf'],'sizes':SIZES,'secondary_sizes':(14,21,28) if secondary else [],'weights':[400,600],'glyphs':len(glyphs),'coverage_bytes':len(data),'pillow':PIL.__version__,'freetype':features.version('freetype2'),'header_sha256':hashlib.sha256(text.encode()).hexdigest()}
    (work/'font73-manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    return report
