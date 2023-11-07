import os
import itertools

# get all the filenames in the results folder
HOME_PATH = os.getenv('HOME')
dir_names = next(os.walk(f"{HOME_PATH}/ray_results"))[1]
print(dir_names)

# sort all filenames in alphabetical order

sorted_dirs = sorted(dir_names)

# create another list containing the new names of each directory. Note that the list should be the same length.

SEEDS =  [1,10,100]
ENVS = ["ns3-v0","OmnetGymApiEnv", "CartPole-v1"]
WORKERS = [2,4,8,16,32,63]
    
new_names = []
for params in itertools.product(ENVS, WORKERS, SEEDS):
    new_names.append(f"{params[0]}_{params[1]}_{params[2]}")

# execute a mv command for each folder in the list 
if len(new_names) == len(sorted_dirs):
    for i in range(sorted_dirs):
        os.system(f"mv {HOME_PATH}/ray_results/{sorted_dirs[i]} {HOME_PATH}/ray_results/{new_names[i]}")