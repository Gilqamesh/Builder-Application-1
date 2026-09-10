from pathlib import Path
import os, subprocess, sys
workspace=Path(os.environ['BUILDER_WORKSPACE_ROOT'])
env=os.environ|{'CCACHE_DISABLE':'1','BUILDER_WORKSPACE_ROOT':str(workspace),'BUILDER_ARTIFACT_ROOT':os.environ.get('BUILDER_ARTIFACT_ROOT',str(workspace/'artifacts'))}
cli=[str(workspace/'cli'),'m03gubnevc9vfwh0ka3iz3mjuf_builder_eval']
checks=[
 ('records','(list '+ ' '.join('(record? ('+name+'))' for name in ['binding','build_config','compiler','filesystem','graph','phase','process','shared_library','signal_handler','runtime','json','google_test','raylib','module_command','language','kernel'])+')','['+' '.join(['true']*16)+']'),
 ('phases','((get (build_config) "default_phase_order"))','[source interface library binary]'),
 ('libraries','((get (build_config) "library_types"))','{"shared": shared}'),
 ('compiler','((get (filesystem) "exists") ((get (compiler) "cxx_path")))','true'),
 ('process-success','((get (process) "succeeds") (list "/bin/true"))','true'),
 ('process-failure','((get (process) "succeeds") (list "/bin/false"))','false'),
 ('freestanding-compiler','((get (filesystem) "exists") (x86_64_elf))','true'),
]
for name,expression,expected in checks:
 r=subprocess.run(cli+[expression],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 if r.returncode or r.stdout.splitlines()[-1]!=expected:
  print('FAIL',name,'\n'+'\n'.join(r.stdout.splitlines()[-8:]),flush=True);sys.exit(1)
 print('PASS',name,flush=True)
for name in ['cmake','dot','download','feh','gzip','module_graph','sha256sum','svg','tar','wget','builder_read','builder_print']:
 expression='('+name+(' false)' if name=='cmake' else ')')
 r=subprocess.run(cli+[expression],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 if r.returncode!=1 or 'expected' not in r.stdout.splitlines()[-1]:
  print('FAIL',name,'\n'+'\n'.join(r.stdout.splitlines()[-8:]),flush=True);sys.exit(1)
 print('PASS',name,'argument validation',flush=True)
