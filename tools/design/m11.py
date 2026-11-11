"""Original M11.0 SVG concepts; these are designs, never runtime test captures."""
from pathlib import Path
from html import escape

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/architecture/figures/m11'
INK='#173042';MUTED='#607685';ACCENT='#67e4bc';PAPER='#f9fbfc';LINE='#dce5eb'

def rect(x,y,w,h,fill,r=0,stroke=None):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}"'+(f' stroke="{stroke}"' if stroke else '')+'/>'
def text(x,y,value,size=16,color=INK,weight=400):
    return f'<text x="{x}" y="{y}" font-family="Inter, Segoe UI, Arial, sans-serif" font-size="{size}" font-weight="{weight}" fill="{color}">{escape(value)}</text>'
def button(x,y,w,label,primary=False):
    return rect(x,y,w,40,ACCENT if primary else '#fff',7,LINE)+text(x+16,y+26,label,15,INK,600)
def icon(x,y,label,tint='#edc877'):
    return rect(x,y,32,27,tint,5)+rect(x+2,y-4,15,8,tint,3)+text(x+45,y+20,label,16)
def base(title):
    return '<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="800" viewBox="0 0 1280 800" role="img"><title>'+escape(title)+'</title>'+rect(0,0,1280,800,'#15313e')+'<path d="M0 610 Q420 260 810 560 T1280 370 V800 H0Z" fill="#204c55"/>'+text(30,40,'E   Engacer OS',22,'#e5f8f2',600)+text(30,66,'Your space. Your system.',14,'#a7c7c9')
def taskbar():
    return rect(0,744,1280,56,'#f4f8fa')+button(18,752,106,'E   Start')+button(135,752,98,'Files')+button(244,752,115,'Settings')+text(1162,768,'10:30',14)+text(1130,789,'11 Nov 2026',12)
def finish(body,title,desktop=True):
    OUT.mkdir(parents=True,exist_ok=True)
    return base(title)+body+(taskbar() if desktop else '')+rect(825,12,435,34,'#eff7f5',7)+text(839,34,'M11.0 DESIGN CONCEPT - NOT A VM CAPTURE',12,MUTED,600)+'</svg>'
def files(context=False):
    s=rect(168,101,950,592,PAPER,12,LINE)+rect(168,101,950,45,'#edf3f7',12)
    s+=text(192,130,'Files',17,INK,600)+text(998,130,'_      []      x',16)
    s+=button(190,160,46,'<')+button(246,160,46,'>')+button(302,160,46,'^')+rect(362,160,600,40,'#fff',7,LINE)+text(380,186,'Home  /  Documents',16)
    s+=button(190,214,122,'New folder')+button(324,214,86,'Copy')+button(422,214,86,'Paste')+button(982,160,112,'Search')
    s+=rect(184,270,181,369,'#eef4f4',8)
    for i,label in enumerate(['Home','Desktop','Documents','Downloads','Pictures','System drive','USB drives','Recycle Bin']):
        if i==2:s+=rect(193,282+i*41,162,36,'#d5f4e9',6)
        s+=text(208,305+i*41,label,15,INK,600 if i==2 else 400)
    s+=text(390,290,'Name',13,MUTED,600)+text(766,290,'Type',13,MUTED,600)+text(1000,290,'Size',13,MUTED,600)
    for i,(name,kind,size) in enumerate([('Projects','Folder',''),('Holiday plans.txt','Text document','2 KB'),('Photos','Folder',''),('Welcome.txt','Text document','1 KB')]):
        yy=306+i*55
        if i==1:s+=rect(380,yy,716,49,'#d3f4e8',6,'#299775')
        s+=icon(393,yy+14,name,'#edc877' if kind=='Folder' else '#d4e5f0')+text(766,yy+32,kind,14,MUTED)+text(1000,yy+32,size,14,MUTED)
    s+=text(391,621,'One click selects. Double-click opens.',14,MUTED)+text(190,674,'4 items   |   1 selected',14,MUTED)
    if context:
        x,y=658,373;s+=rect(x+5,y+7,259,321,'#c9d6dc',9)+rect(x,y,259,321,'#fff',9,LINE)
        for i,label in enumerate(['Open','Open with...','Cut','Copy','Rename','Move to Recycle Bin','Properties']):
            if i==0:s+=rect(x+6,y+7,247,36,'#e0f6ee',5)
            s+=text(x+19,y+32+i*42,label,15,INK,600 if i==0 else 400)
    return finish(s,'Engacer OS Files context-menu concept' if context else 'Engacer OS Files selection concept')
def installer():
    s=rect(155,111,970,590,PAPER,14)+rect(155,111,222,590,'#e6f1ef',14)
    s+=text(183,159,'ENGACER OS SETUP',16,INK,600)+text(183,188,'Make yourself at home.',14,MUTED)
    for i,label in enumerate(['Welcome','Keyboard','Your disk','Components','Your style','Review','Installing','Restart']):
        yy=228+i*48
        if i==2:s+=rect(172,yy-24,190,38,'#bdeedc',7)
        s+=text(187,yy,f'{i+1}   {label}',15,INK,600 if i==2 else 400)
    s+=text(411,160,'STEP 3 OF 8',12,MUTED,600)+text(410,211,'Where should we install Engacer OS?',28,INK,600)
    s+=text(411,244,'Choose a disk from the list. Disk tools lets you review its partitions.',15,MUTED)
    s+=rect(411,276,677,222,'#fff',8,LINE)+text(430,305,'DISKS AND PARTITIONS',12,MUTED,600)+text(999,305,'CAPACITY',12,MUTED,600)
    s+=rect(419,324,661,71,'#d3f4e8',5,'#299775')+text(438,349,'(o)   Disk 1',17,INK,600)+text(481,376,'SATA  |  Empty disk  |  Ready to install',14,MUTED)+text(1004,363,'20 GiB',16)
    s+=text(438,425,'( )   USB drive',16)+text(481,452,'File transfer only - not an installation target',14,MUTED)+text(1013,437,'8 GiB',16)
    s+=button(411,518,142,'Disk tools...')+text(411,589,'Your selected disk will be reviewed before any changes are made.',14,MUTED)
    s+=button(411,637,104,'Back')+button(961,637,127,'Next',True)
    return finish(s,'Engacer OS installation disk-selection concept',False)
def start():
    s=rect(320,104,640,624,'#f7fbfc',14,LINE)+rect(348,128,584,45,'#fff',7,LINE)+text(367,157,'Search apps and settings',16,MUTED)
    s+=button(348,190,124,'Pinned')+button(482,190,124,'All apps')+button(616,190,150,'Categories',True)
    s+=text(350,270,'Your categories',22,INK,600)+text(350,299,'Organize your apps in a way that makes sense to you.',14,MUTED)
    names=[('Accessories','Editor, Clock, Calculator'),('System','Files, Settings, Disks'),('Internet','Browser, Downloads'),('Media','Images, Media Player'),('Development','Terminal, Developer tools')]
    for i,(name,detail) in enumerate(names):
        yy=320+i*62;s+=rect(348,yy,584,53,'#fff',7,LINE)+icon(364,yy+15,name,'#a9dfd7')+text(564,yy+33,detail,13,MUTED)
    s+=rect(320,659,640,69,'#eaf2f3',12)+text(350,700,'Owner',16,INK,600)+button(754,675,178,'Power options')
    return finish(s,'Engacer OS Start categories concept')
if __name__=='__main__':
    for name,content in [('files-selection',files()),('context-menu',files(True)),('installer-disk',installer()),('start-menu',start())]:
        (OUT/(name+'.svg')).write_bytes(content.encode('utf-8'))
    print('Prepared four original SVG design concepts; no native runtime claim')
