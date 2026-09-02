import base64
from pathlib import Path

root = Path(r"D:\_GitHub\30_MyProjects\HarmonyOS-smart-car\src\harmony\12.1_Bluetooth_Control")
for name in ("bluetooth_control.c", "bluetooth_protocol.c"):
    print(name + ":" + base64.b64encode((root / name).read_bytes()).decode("ascii"))
