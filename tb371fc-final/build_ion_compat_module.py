#!/usr/bin/env python3
"""Build v676.68 OGL + VNDK30-compatible ION bridge, preserving stock SQE.

Only create the ZIP. Does not touch tablet partitions or installed modules.
"""
from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
from hashlib import sha256

here=Path(__file__).resolve().parent
upstream=Path.home()/'Downloads/OGL & VK + SQE v676.68_M^KSU.zip'
installer=Path.home()/'kernel-lab/tb371fc/evidence/gpu-upgrade-20261010/Turnip-26.2.3-MAGISK-KSU.zip'
output=here/'TB371FC-Adreno-v676.68-ION-Compat-APatch.zip'
assert sha256(upstream.read_bytes()).hexdigest()=='a31250debcae47595d5ca4c54ce222b9a8c3ae966cc46a0f40d825fa50c75002'

core=('libadreno_utils.so','libadreno_app_profiles.so','libgsl.so',
      'libllvm-glnext.so','libllvm-qcom.so','libllvm-qgl.so')
egl=('eglSubDriverAndroid.so','libEGL_adreno.so','libGLESv1_CM_adreno.so',
     'libGLESv2_adreno.so','libq3dtools_adreno.so','libq3dtools_esx.so',
     'libVkLayer_ADRENO_qprofiler.so')
names=[]
for arch in ('lib','lib64'):
    prefix='system/vendor/'+arch+'/'
    names += [prefix+name for name in core]
    names += [prefix+'egl/'+name for name in egl]
names += ['system/vendor/lib/hw/vulkan.adreno.so']

prop='''id=tb371fc-adreno-ogl-67668
name=TB371FC Adreno 650 OpenGL v676.68 (VNDK30 ION compatibility)
version=676.68-ION-compat-20261010
versionCode=67669
author=Peter upstream / TB371FC test integration
description=Experimental matched Adreno650 EGL/GLES 32/64 + ion-fallback libdmabufheap; 32-bit Qualcomm VK. Preserve A650 SQE and 64-bit Mesa Turnip.
'''

with ZipFile(upstream) as src, ZipFile(installer) as wrapper, ZipFile(output,'w',ZIP_DEFLATED,compresslevel=7) as result:
    for name in names:
        result.writestr(name,src.read(name))
    for arch,bit in (('lib',32),('lib64',64)):
        shim=here/'abi-bridge'/f'libdmabufheap-bridge-{bit}.so'
        assert shim.is_file()
        result.writestr('system/vendor/'+arch+'/libdmabufheap.so',shim.read_bytes())
    for name in ('META-INF/com/google/android/update-binary','META-INF/com/google/android/updater-script'):
        result.writestr(name,wrapper.read(name))
    result.writestr('module.prop',prop)
    result.write(here/'customize.sh','customize.sh')

with ZipFile(output) as z:
    assert not any(x in z.namelist() for x in (
        'system.prop','service.sh','system/vendor/lib64/hw/vulkan.adreno.so',
        'system/vendor/firmware/a650_sqe.fw'))
    assert len(z.namelist())==len(names)+2+4
print(output)
print('SHA256:',sha256(output.read_bytes()).hexdigest())
print('Payload libraries:',len(names)+2)
