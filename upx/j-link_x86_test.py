#!d:\cccc2020\TOOL\python-3.9.1-embed-win32\python.exe

import sys
sys.path.append('pylink')

import os
import jlink

print(sys.path)

def init_jlink(dllname: str):
    #jlink initial

    dllpath = os.path.join(os.getcwd(), dllname)
    print(dllpath)

    jlk = jlink.JLink(lib=jlink.library.Library(dllpath=dllpath))
    jlk.open()
    print(jlk.product_name)
    print(jlk.connected())
    print(jlk.target_connected())
    jlk.set_tif(jlink.enums.JLinkInterfaces.SWD)
    jlk.connect(chip_name='STM32F103CB', speed=4000)
    print(jlk.target_connected())
    print(jlk.version)

def main():
    init_jlink('JLinkARM_v4.92.0.0.dll')
    init_jlink('JLinkARM_v7.60.8.0.dll')

if __name__ == '__main__':
    main()


