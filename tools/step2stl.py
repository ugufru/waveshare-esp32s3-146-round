import sys
from OCP.STEPControl import STEPControl_Reader
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.StlAPI import StlAPI_Writer
r=STEPControl_Reader(); r.ReadFile(sys.argv[1]); r.TransferRoots(); s=r.OneShape()
BRepMesh_IncrementalMesh(s,0.05,False,0.3,True)
w=StlAPI_Writer(); w.ASCIIMode=False; w.Write(s,sys.argv[2]); print("ok")
