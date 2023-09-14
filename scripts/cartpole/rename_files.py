import os
import itertools

# get all the filenames in the results folder
HOME_PATH = os.getenv('HOME')
dir_names = next(os.walk(f"{HOME_PATH}/ray_results"))[1]


# sort all filenames in alphabetical order

sorted_dirs = sorted(dir_names)

# create another list containing the new names of each directory. Note that the list should be the same length.

SEEDS =  [1,10,100]
ENVS = ['ns3-v0']
WORKERS = [2,4,8,16,32,63]
    
new_names = []
for params in itertools.product(ENVS, WORKERS, SEEDS):
    new_names.append(f"{params[0]}_{params[1]}_{params[2]}")

nsdirs = [x for x in sorted_dirs if 'ns3' in x]
nsnewnames = [x for x in new_names if 'ns3' in x]
# execute a mv command for each folder in the list 

if len(nsnewnames) == len(nsdirs):
    for i in range(len(nsdirs)):
        os.system(f"mv {HOME_PATH}/ray_results/{nsdirs[i]} {HOME_PATH}/ray_results/{nsnewnames[i]}")
