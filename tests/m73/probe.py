"""Development-only owned framebuffer probe; does not claim acceptance."""
import argparse
import ui
p=argparse.ArgumentParser();p.add_argument('--ram',type=int,default=128);p.add_argument('--resolution',default='3840x2160');a=p.parse_args()
work=ui.q.new_work('m73-display-probe');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
with ui.guest(work/'probe',disk,tuple(map(int,a.resolution.split('x'))),ram=a.ram) as g:
    print('\n'.join(line for line in g.log().splitlines() if 'GOP' in line or 'DISPLAY' in line or 'EDID' in line))
    (ui.ROOT/'build/m73/display-probe.log').write_bytes(g.log().encode());ui.shot(g,ui.ROOT/'build/m73','display-probe')
