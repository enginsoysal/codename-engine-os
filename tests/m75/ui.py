"""Reuse normal guest input with M7.5 immutable map identities."""
import os,importlib.util
from pathlib import Path
os.environ["ENGINE74_IMAGE"]="EngineOS-M75"
spec=importlib.util.spec_from_file_location("shared74",Path(__file__).resolve().parents[1]/"m74/ui.py")
shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
globals().update({k:v for k,v in vars(shared).items() if not k.startswith("__")})

_original_guest=guest
def guest(*args,**kwargs):
    value=_original_guest(*args,**kwargs)
    value.args=[arg for arg in value.args if arg!="-no-reboot"]
    return value
