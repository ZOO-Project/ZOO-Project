import sys
import pandas as pd


paths=sys.path
pathsDataframe=pd.DataFrame({'paths':paths})
pathsDataframe.to_csv('./qgis_sys_paths.csv',index=False)

import os
import json
env=dict(os.environ)
rem=['SECURITYSESSIONID', 'LaunchInstanceID', 'TMPDIR']
_=[env.pop(r, None) for r in rem]with open('./qgis_env.json', 'w') as f:
	json.dump(env,f,ensure_ascii=False,indent=4)