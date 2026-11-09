"""Observe every real installer step and restart the owned guest with ISO retained."""
import argparse,json,struct,time,traceback
import ui
ROOT=ui.ROOT;q=ui.q;h=ui.h

def state(g,name,installed=False):return ui.integer(g,name,installed)
def control(g,action):
 count=state(g,'controls');raw=ui.memory(g,ui.symbol('buttons',480)[0],count*20)
 return [struct.unpack_from('<5I',raw,i*20) for i in range(count) if struct.unpack_from('<I',raw,i*20+16)[0]==action]
def click(g,p,action,edge=False):
 rows=control(g,action);assert len(rows)==1,(action,rows);x,y,w,height,_=rows[0];start=len(g.log());p.logical(x+w-(12 if edge else w//2),y+height//2)
 if action not in [30,84,3]:g.wait('M73_VIEW step=',start)
 return start

