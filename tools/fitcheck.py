# Fit check: pocketwatch STLs against a mesh of the Waveshare board STEP.
#
# Board mesh: extract ESP32-S3-Touch-LCD-1_46.stp from the "With Cover" .rar
# (URL in docs/board-specs.md), then in a venv with cadquery-ocp and numpy:
#   python tools/step2stl.py ESP32-S3-Touch-LCD-1_46.stp board.stl
#   python -c "import numpy as np,sys; exec(open('tools/fitcheck.py').read().split('BEZEL=')[0]); np.save('board.stl.npy', load('board.stl'))"
#   python tools/fitcheck.py pocketwatch/front.stl pocketwatch/back.stl board.stl.npy
#
# Reports board points that fall inside case material, grouped by location.
# BEZEL and BACKZ must match bezel_t and back_z in pocketwatch.scad.
import numpy as np, sys
def load(f):
    d=open(f,'rb').read()
    if d[:5]==b'solid' and b'facet' in d[:400]:
        v=[list(map(float,l.split()[1:4])) for l in d.decode().splitlines() if l.strip().startswith('vertex')]
        return np.array(v,dtype=np.float64).reshape(-1,3,3)
    n=int.from_bytes(d[80:84],'little')
    return np.frombuffer(d,dtype=np.dtype([('n','<3f4'),('v','<9f4'),('x','<u2')]),count=n,offset=84)['v'].reshape(n,3,3).astype(np.float64)
BEZEL=1.2; BACKZ=-21.15
front=load(sys.argv[1]); back=load(sys.argv[2]); board=np.load(sys.argv[3]).astype(np.float64)
front=front.copy(); front[:,:,0]*=-1; front[:,:,2]=BEZEL-front[:,:,2]
back=back.copy(); back[:,:,2]+=BACKZ
for n,m in (('front',front),('back',back)):
    v=m.reshape(-1,3); print(n,'model-frame bbox',v.min(0).round(2),v.max(0).round(2))
pts=np.concatenate([board.reshape(-1,3),board.mean(1)])
def segs(tri,z):
    zs=tri[:,:,2]; out=[]
    for i,j in ((0,1),(1,2),(2,0)):
        za,zb=zs[:,i],zs[:,j]; m=(za-z)*(zb-z)<0
        t=(z-za[m])/(zb[m]-za[m]); out.append(tri[m,i,:2]+(tri[m,j,:2]-tri[m,i,:2])*t[:,None])
    # pair up: each crossing triangle contributes 2 points; recompute per-triangle
    S=[]
    for k in range(len(tri)):
        pass
    return None
def slice_pairs(tri,z):
    zs=tri[:,:,2]
    below=zs<z; cnt=below.sum(1); m=(cnt==1)|(cnt==2); T=tri[m]; B=below[m]
    P=[]
    for i,j in ((0,1),(1,2),(2,0)):
        za,zb=T[:,i,2],T[:,j,2]; c=B[:,i]!=B[:,j]
        t=np.where(c,(z-za)/np.where(c,zb-za,1),0)
        P.append((T[:,i,:2]+(T[:,j,:2]-T[:,i,:2])*t[:,None],c))
    a=[];b=[]
    for k in range(len(T)):
        q=[P[e][0][k] for e in range(3) if P[e][1][k]]
        if len(q)==2: a.append(q[0]); b.append(q[1])
    return np.array(a),np.array(b)
def inside(p,a,b):
    if len(a)==0: return np.zeros(len(p),bool)
    x,y=p[:,0][:,None],p[:,1][:,None]
    ay,by=a[:,1][None],b[:,1][None]; ax,bx=a[:,0][None],b[:,0][None]
    c=((ay>y)!=(by>y)) & (x < ax+(y-ay)*(bx-ax)/np.where(by==ay,1e-12,by-ay))
    return c.sum(1)%2==1
for name,m in (('front',front),('back',back)):
    zmin,zmax=m[:,:,2].min(),m[:,:,2].max(); hits=[]
    for z in np.arange(zmin+0.05,zmax,0.1):
        sel=pts[np.abs(pts[:,2]-z)<0.05]
        if not len(sel): continue
        a,b=slice_pairs(m,z)
        ins=np.zeros(len(sel),bool)
        for s in range(0,len(sel),4000): ins[s:s+4000]=inside(sel[s:s+4000],a,b)
        if ins.any(): hits.append(sel[ins])
    if hits:
        H=np.concatenate(hits); print(name,'COLLISIONS',len(H))
        # cluster by rounding
        keys={}
        for p in H: keys.setdefault((round(p[0]/3)*3,round(p[1]/3)*3),[]).append(p)
        for k,v in sorted(keys.items(),key=lambda kv:-len(kv[1]))[:15]:
            v=np.array(v); print('   near',k,'n',len(v),'z %.2f..%.2f'%(v[:,2].min(),v[:,2].max()),'r %.2f..%.2f'%(np.hypot(v[:,0],v[:,1]).min(),np.hypot(v[:,0],v[:,1]).max()))
    else: print(name,'no collisions')
