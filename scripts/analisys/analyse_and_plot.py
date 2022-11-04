import matplotlib.pyplot as plt
import pandas as pd
import json
import os
from pathlib import Path
import matplotlib.lines as mlines
import numpy as np

plt.style.use(['science', 'muted', 'grid'])

RESULTS_MAP = {"trimmed": "numRcvTrimmedHeaderSigNdp:last",
           "goodput": "mohThroughputNDP:last",
           "inst": "instThroughputNDP:mean",
           "fct": "fctRecordv3:last"
}

LATENCIES = ['0ms', '0.01ms', '0.1ms', '1ms']
RESULTS = ["trimmed", "goodput", "inst", "fct"]
RESULTS_PATH = str(Path.home()) + "/RLlibIntegration/ndp_simple_sims/case1"

def get_vectors(data, module_vector_pairs):
    data_vectors = data[list(data.keys())[0]]['vectors']
    ret = {}
    stats = set()
    for vector in data_vectors:
        for pair in module_vector_pairs:
            stats.add(vector['name'])
            if (pair[0] in vector['module']) and (pair[1] in vector['name']):
                ret.update({pair[0]:{'module': vector['module'], 'name': vector['name'] ,'time': vector['time'], 'value': vector['value']}})

    return ret, stats

if __name__ == "__main__":
    
    
    for latency in LATENCIES:
    
        for result in RESULTS:
            initial_window_results = {}
        
            FOLDER = RESULTS_PATH + "/results" + f"_{latency}"

            directory = os.fsencode(FOLDER)
            
            for file in os.listdir(directory):
                filename = os.fsdecode(file)
                window_sizes = ['256', '512', '1024', '2048']
                if filename.endswith('.sca') and any(x in filename for x in window_sizes):
                    window_size = filename.split('-')[1]
                    file_format = filename.split('.')[-1]
                    file_no_format = filename.split('.')[0]
                    print(f'{FOLDER}/{file_no_format}.sca')	
                    # Use scavetool to extract results we are interested in
                    os.system(f'scavetool export -f "module(\\"**.server*\\") AND name(\\"{RESULTS_MAP[result]}\\")" -F JSON -o {FOLDER}/{latency}_{result}_{window_size}.json {FOLDER}/{file_no_format}.sca')

                    with open(f"{FOLDER}/{latency}_{result}_{window_size}.json", 'r') as fp:
                        results = json.load(fp)

                        results = results[list(results.keys())[0]]['scalars']
                        initial_window_results[int(window_size)] = {}
                        
                        for res in results:
                            if res['name'] == RESULTS_MAP[result]:
                                initial_window_results[int(window_size)].update({res['module']: res['value']})
            
            df_index = list(initial_window_results.keys())
            df_columns = list(initial_window_results.values())
            df = pd.DataFrame(df_columns, index=df_index).sort_index()
            df.index.name = "initial_window"

            df.to_csv(f"{latency}_{result}.csv")

        LATENCIES_PLOT = [['fct', 'goodput'], ['trimmed', 'inst']]
    
        fig, axs = plt.subplots(2,2, figsize=(14,5))

        # plt.setp(axs, xlabel='Initial Window', ylabel=ylabel)
        # red_line = mlines.Line2D([],[], color='red', linewidth=1, label='F1')
        # blue_line = mlines.Line2D([],[], color='blue', linewidth=1, label='F2')
        # green_line = mlines.Line2D([],[], color='green', linewidth=1, label='F3, F4, F5')

        for i in range(2):
            for j in range(2):
                result_to_plot = LATENCIES_PLOT[i][j]

                if result_to_plot == 'trimmed':
                    title = "Trimmed Packets / Flow Size"
                    ylabel = "X"
                    denominator= 20000

                elif result_to_plot == 'goodput':
                    ylabel = "Gbps"
                    title = "Goodput"
                    denominator= 1000000000

                elif result_to_plot == 'inst':
                    title = 'Inst. Throughput Mean'
                    ylabel = 'Gbps'
                    denominator= 1000000000

                elif result_to_plot == 'fct':
                    title = "FCT"
                    ylabel = "s"
                    denominator= 1


                file_name = str(Path.home())+f"/RLlibIntegration/{latency}_{LATENCIES_PLOT[i][j]}.csv"
                results_df = (pd.read_csv(file_name, index_col=[0])/denominator)
                results_df = results_df.rename(columns={"ndpcongestionnetwork.server1.app[0]":"F1", "ndpcongestionnetwork.server2.app[0]":"F2", 
                                                        "ndpcongestionnetwork.server3.app[0]":"F3", "ndpcongestionnetwork.server3.app[1]":"F4",
                                                        "ndpcongestionnetwork.server3.app[2]":"F5", "ndpcongestionnetwork.server3.app[3]":"F6" 
                                                        } ) 
  
                labels = results_df.index
                x = np.arange(len(labels))  # the label locations
                width = 0.1  # the width of the bars

                rects1 = axs[i][j].bar(x - width*5/2, results_df['F1'].values, width, label='F1')
                rects2 = axs[i][j].bar(x - width*3/2, results_df['F2'].values, width, label='F2')
                rects3 = axs[i][j].bar(x - width/2, results_df['F3'].values, width, label='F3')
                rects4 = axs[i][j].bar(x + width/2, results_df['F4'].values, width, label='F4')
                rects5 = axs[i][j].bar(x + width*3/2, results_df['F5'].values, width, label='F5')
                rects6 = axs[i][j].bar(x + width*5/2, results_df['F6'].values, width, label='F6')
                
                axs[i][j].set_xticks(x, labels)
                axs[i][j].legend()
                axs[i][j].autoscale(tight=True)
                axs[i][j].set(ylabel=ylabel, xlabel='IW')

                axs[i][j].title.set_text(title)


                # axs[i][j].set_xscale('log', base=2)
                # axs[i][j].legend()
            

        plt.suptitle("Varying Initial Window on different BDP")
        # plt.legend()
        
        #x_ticks = [1,10,20,30,40,50]
        #plt.xticks(x_ticks)
        # plt.subplots_adjust(bottom=0.2, left=0.14)
        plt.tight_layout()
        plt.savefig(f'case1/InitialWindowBDP_{latency}.pdf', dpi=300)
        plt.savefig(f'case1/InitialWindowBDP_{latency}.jpg', dpi=300)
        plt.clf()
        plt.cla()

    
