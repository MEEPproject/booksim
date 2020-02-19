#!/usr/bin/python3
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import sys
import os, errno
import json
import re
from abc import ABCMeta, abstractmethod
from math import log
import pylab
import inspect

def lineno():
    """Returns the current line number in our program."""
    return inspect.currentframe().f_back.f_lineno


def main():

    print("Openning JSON:\t\t\t{}".format(sys.argv[1]))
    with open(sys.argv[1],'r') as f:
        data = json.load(f)

        if "output" not in data["plots"]:
            data["plots"]["output"] = sys.argv[1].replace(".json",".pdf")

        p = Plotter.factory(data)

        p.GenerateStyle()

        p.CreatePlot()

        p.GenerateOutput()

        print("Generated output:\t", data["plots"]["output"])



### TODO: move location of this function?
def format_label(label):
    if "r\'" in label or "r\"" in label:
        return eval(label)
    else:
        return label


class BookSimReader(object):
    def __init__(self, filename):
        self.filename = filename
        self.datatable = self.create_datatable(filename)
        self.classes = self.datatable['class'].max() + 1
        #self.initial_time = self.datatable.time[0]

    def create_datatable(self, filename):
        print("Processing file:\t{}".format(filename))
        dt = pd.read_csv(filename, sep=',')
        return dt

    def read_time(self):
        return self.datatable.loc[self.datatable['class'] == 0]["time"] - self.initial_time

    #### TODO: Check this cody, it is very ugly
    def read_combined_stat(self, stat_name, class_id):
        if "eval(" in stat_name:
            stats = list()
            temp_eval = stat_name.replace('eval','[')
            pattern = re.compile(r'[a-zA-Z_]+')
            m = re.findall(pattern,temp_eval)
            temp_eval += ' for '
            stats_no = len(m)
            for index, item in enumerate(m):
                temp_eval = temp_eval.replace(item,'stat' + str(index))
                temp_eval += 'stat' + str(index)
                if index < stats_no -1:
                    temp_eval += ','
                stats.append(item)
            temp_eval += ' in '
            
            if stats_no > 1:
                temp_eval += 'zip('
                for index, stat in enumerate(stats):
                    temp_eval += 'self.read_stat(stats[' + str(index) + '],class_id)'
                    if index < stats_no -1:
                        temp_eval += ','
                temp_eval += ')'
            else:
                temp_eval += 'self.read_stat(stats[0],class_id)'
            temp_eval += ']'

            temp_eval = temp_eval.replace('$','').replace('{','').replace('}','')
            y_list = eval(temp_eval)
        else:
            y_list = self.read_stat(stat_name,class_id)

        return y_list
        
    def read_stat(self, stat, class_id):
        # if class_id is -1 it returns the sum of the stat for all the classes
        if stat in self.datatable:
            if class_id == -1:
                #print("stat ", stat)
                if 'his_' in stat:
                    hist = list()
                    for c in range(self.classes):
                        hist.append([eval(x) for x in self.datatable.loc[self.datatable['class'] == c][stat].tolist()])
                    for index, hist_c in enumerate(hist):
                        if index == 0:
                            total = hist_c
                            #print(total)
                        else:
                            for index2, hist_c_load in enumerate(hist_c):
                                total[index2] = [x+y for x,y in zip(total[index2],hist_c_load)]
                    return total
                else:
                    return self.datatable.groupby('load')[stat].sum().tolist()
            else:
                return self.datatable.loc[self.datatable['class'] == class_id][stat].tolist()
                #print("read_stats: ", self.datatable.loc[self.datatable['class'] == class_id][stat])
                #return self.datatable.loc[self.datatable['class'] == class_id][stat]
        else:
            print("Warning: stat ", stat, " doesn't exist in csv table of: ", self.filename)

    def read_offered_load(self, inj_load_uses_flits=False):
        load = []
        if inj_load_uses_flits == False:
            for c in range(self.classes):
                if c == 0:
                    load = self.datatable[self.datatable['class'] == c].load * round(self.datatable[self.datatable['class'] == c].avg_sent_packet_size)
                    #load = self.datatable[self.datatable['class'] == c].load * round(self.datatable[self.datatable['class'] == c].avg_accepted_packet_size)
                else:
                    temp_load = self.datatable[self.datatable['class'] == c].load * round(self.datatable[self.datatable['class'] == c].avg_sent_packet_size)
                    #temp_load = self.datatable[self.datatable['class'] == c].load * round(self.datatable[self.datatable['class'] == c].avg_accepted_packet_size)
                    load = [x+y for x,y in zip(load.tolist(),temp_load.tolist())]
        else:
            for c in range(self.classes):
                if c == 0:
                    load = self.datatable[self.datatable['class'] == c].load
                else:
                    temp_load = self.datatable[self.datatable['class'] == c].load
                    load = [x+y for x,y in zip(load.tolist(),temp_load.tolist())]
        if isinstance(load, list):
            return [x*100 for x in load]
        else:
            return [x*100 for x in load.tolist()]

    def read_stat_ponderate_classes(self,stat,inj_rate_uses_flits=False):
        if stat == 'load':
            return self.read_offered_load(inj_rate_uses_flits)

        
        y_values = list()
        multipliers = list()
        for c in range(self.classes):
            y_values.append(self.read_combined_stat(stat,c))
            # Packet ponderation
            if 'plat' in stat or 'nlat' in stat:
                multipliers.append(self.read_combined_stat('avg_sent_packets',c))
            if 'bypassed_flits' in stat or 'flat' in stat:
                multipliers.append(self.read_combined_stat('avg_sent_flits',c))

        if 'avg_sent_flits' in stat or 'avg_accepted_flits' in stat:
            stat_values = [0.0] * len(y_values[0])
            for values in y_values:
                stat_values = [x+y for x,y in zip(values,stat_values)]
            return stat_values


        stat_values = [0.0] * len(y_values[0])
        # stat times sent packets/flits
        for index, values in enumerate(y_values):
            stat_values = [x*y + z for x,y,z in zip(values, multipliers[index], stat_values)]
        
        total = [0.0] * len(multipliers[0])
        for mult in multipliers:
            total = [x+y for x,y in zip(mult, total)]

        stat_values = [x/y for x,y in zip(stat_values,total)]

        if isinstance(stat_values, list):
            return stat_values
        else:
            return stat_values.tolist()


# FIXME: On progress
class Plotter:
    __metaclass__ = ABCMeta
    def __init__(self, data):
        # FIXME: process common stuff here (filename, type, yaxis, xaxis...)
        self.data = data
        self.xaxis_stat = data["plots"]["x-axis"]
        self.yaxis_stat = data["plots"]["y-axis"]
    
        if "figsize" in data["plots"]:
            figsize = eval(data["plots"]["figsize"])
        else:
            figsize = (8,6)

        if 'simulation_dir' in data['plots']:
            new_simnames = list()
            for sim in data['plots']['simulation_files']:
                if type(sim) is str:
                    new_simnames.append(data['plots']['simulation_dir'] + sim)
                elif type(sim) is list:
                    ss_list = list()
                    for s in sim:
                        ss_list.append(data['plots']['simulation_dir'] + s)
                    new_simnames.append(ss_list)
                    
            print(new_simnames)
            data['plots']['simulation_files'] = new_simnames

        self.markevery = None
        if 'markevery' in data['plots']:
            self.markevery = int(data['plots']['markevery'])
        
        if "markersize" in self.data["plots"]:
            self.markersize = int(self.data["plots"]["markersize"])
        else:
            self.markersize = 7


        self.fig, self.ax = plt.subplots(figsize=figsize)

        self.colormap = self.CreateColormap()

    def factory(data):
        if data['plots']['type'] == 'custom': return CustomPlotter(data)
        if data['plots']['type'] == 'custom-statlist': return CustomStatListPlotter(data)
        if data['plots']['type'] == 'custom-multiclass': return CustomMulticlassPlotter(data)
        if data['plots']['type'] == 'bar-multiclass' : return BarMulticlassPlotter(data)
        if data['plots']['type'] == 'histogram': return HistogramPlotter(data)
        if data['plots']['type'] == 'horizontal-histogram': return HorizontalHistogramPlotter(data)
        if data['plots']['type'] == 'throughput': return ThroughputPlotter(data)
        if data['plots']['type'] == 'zero-latency': return ZeroLatencyPlotter(data)
        if data['plots']['type'] == 'throughput-baseline': return ThroughputBaselinePlotter(data)
        if data['plots']['type'] == 'legend': return LegendPlotter(data)
    factory = staticmethod(factory)

    def CreateColormap(self):
        colormap = 'viridis'
        ## FIXME: Is there a way to simplify the following two lines?
        if 'colormap' in self.data['plots']:
            colormap = self.data['plots']['colormap']

        cmap = plt.get_cmap(colormap)
        total_colors = len(cmap.colors)
        # Find latest colormap index
        no_colors = 1
        for color in self.data['plots']['colors']:
            if 'colormap' in color:
                color_index = int(color.replace('colormap[','').replace(']',''))
                no_colors = color_index+1 if color_index+1 > no_colors else no_colors
        color_offset = int(total_colors/(no_colors))
        return list(reversed(cmap.colors))[0:-1:color_offset]
    
    @abstractmethod
    def GenerateStyle(self):
        pass
    
    @abstractmethod
    def ObtainSeries(self):
        pass
        
    def CreatePlot(self):
        # FIXME: general stuff like figsave
        y_list_baseline = list()
        for indx, f in enumerate(self.data["plots"]["simulation_files"]):
            br = BookSimReader(f)
            
            x_list, y_list = self.ObtainSeries(br)
            if indx == 0:
                y_list_baseline = y_list
            #print('y_list: ', y_list)

            ### FIXME: I don't like this code here. create self.color as list.
            ###        Here we only have to index that list
            colormap = self.CreateColormap()

            tmp = ""
            index_list = y_list if len(y_list) < len(y_list_baseline) else y_list_baseline
            for p_index, y in enumerate(index_list):
                tmp += "x = {} | y_base = {} | y = {} | y/y_base = {}\n".format(
                        str(x_list[p_index]),
                        str(y_list_baseline[p_index]),
                        str(y_list[p_index]),
                        str(y_list[p_index]/y_list_baseline[p_index])
                    )
            print("Points of ", f, " (relative to the baseline):\n", tmp)

            self.ax.plot(x_list, y_list, label=self.data["plots"]["legend"][indx],
                    color=eval(self.data["plots"]["colors"][indx]),
                    linestyle=self.linestyle[indx], marker=self.markers[indx],
                    mec='black', markersize=self.markersize, markeredgewidth=0.5 , lw=1,
                    markevery=self.markevery)

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        ncols = len(self.data['plots']['legend'])
        if "number_columns" in self.data["plots"]:
            ncols = int(self.data["plots"]["number_columns"])
        self.ax.legend(loc=0, ncol=ncols)
        if "disable_legend" in self.data["plots"]:
            if self.data["plots"]["disable_legend"] == "1":
                self.ax.legend().remove()
        self.ax.set_xlabel(self.data["plots"]["x-label"])
        self.ax.set_ylabel(self.data["plots"]["y-label"])
        self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        self.ax.set_xlim(xmin= float(self.data["plots"]["x-min"]), xmax=float(self.data["plots"]["x-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        #self.ax.minorticks_on()
        self.ax.grid(b=True, which='major',color='silver', linewidth=0.2, linestyle="-")
        #self.ax.grid(b=True, which='minor',color='silver', linewidth=0.1, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        
        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        box = self.ax.get_position()
        if "legend_outside" in self.data["plots"]:
            if self.data["plots"]["legend_outside"] == "1":
                self.ax.set_position([box.x0, box.y0, box.width*0.8, box.height])
                self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5),
                        ncol=ncols)

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight',pad_inches=0)
        #self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight')
    

class CustomPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markersize" in self.data["plots"]:
            self.markersize = int(self.data["plots"]["markersize"])
        else:
            self.markersize = 7

    def ObtainSeries(self, booksim_reader):
        yaxis_stat = self.yaxis_stat
        xaxis_stat = self.xaxis_stat

        y_list = booksim_reader.read_combined_stat(yaxis_stat, -1)

        # FIXME: Add this in read_stat. If the stat is load, then can to read_offered_load().
        if xaxis_stat == "load":
            x_list = booksim_reader.read_offered_load()
        else:
            x_list = booksim_reader.read_stat(xaxis_stat,-1)

        return x_list, y_list

class CustomStatListPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.
        self.stat_index = 0

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        yaxis_stat = self.yaxis_stat[self.stat_index]
        self.stat_index += 1
        xaxis_stat = self.xaxis_stat

        y_list = booksim_reader.read_combined_stat(yaxis_stat, -1)

        # FIXME: Add this in read_stat. If the stat is load, then can to read_offered_load().
        if xaxis_stat == "load":
            x_list = booksim_reader.read_offered_load()
        else:
            x_list = booksim_reader.read_stat(xaxis_stat,-1)

        return x_list, y_list

class CustomMulticlassPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        yaxis_stat = self.yaxis_stat
        xaxis_stat = self.xaxis_stat

        y_list = booksim_reader.read_stat_ponderate_classes(yaxis_stat)

        x_list = booksim_reader.read_stat_ponderate_classes(xaxis_stat)

        return x_list, y_list

class BarMulticlassPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]
        
        if "markersize" in self.data["plots"]:
            self.markersize = int(self.data["plots"]["markersize"])
        else:
            self.markersize = 7

    def ObtainSeries(self, booksim_reader):
        yaxis_stat = self.yaxis_stat
        xaxis_stat = self.xaxis_stat

        y_list = booksim_reader.read_stat_ponderate_classes(yaxis_stat)

        x_list = booksim_reader.read_stat_ponderate_classes(xaxis_stat)

        return x_list, y_list
    
    def CreatePlot(self):
        # FIXME: general stuff like figsave
        total_width=0.9
        width = total_width/len(self.data["plots"]["simulation_files"])
        for indx, f in enumerate(self.data["plots"]["simulation_files"]):
            br = BookSimReader(f)
            
            x_list, y_list = self.ObtainSeries(br)

            ### FIXME: I don't like this code here. create self.color as list.
            ###        Here we only have to index that list
            colormap = self.CreateColormap()

           # self.ax.plot(x_list, y_list, label=self.data["plots"]["legend"][indx],
           #         color=eval(self.data["plots"]["colors"][indx]),
           #         linestyle=self.linestyle[indx], marker=self.markers[indx], mec='black', markersize=7, markeredgewidth=0.5 , lw=1)
            
            ### FIXME: Use user defined x value to read histogram
            #data = eval(y_list[eval(self.data["plots"]["x-val"])])
            data = list()
            data_index = list()
            for val in self.data["plots"]["x-val"]:
                for index,item in enumerate(x_list):
                    if abs(val - item) < val/100: 
                        data.append(y_list[index])
                        data_index.append(val)
            
            bar_height = 1
            name = format_label(self.data["plots"]["legend"][indx])
            #self.ax.barh([bar_height*x for x in range(len(bin_data))], bin_data, height=bar_height, left=lefts,
            self.ax.bar([bar_height*(x+width*indx)+width/2.0 for x in range(len(data))], data, width=width*bar_height,
                    label=name,
                    color=eval(self.data["plots"]["colors"][indx]),
                    edgecolor='black', linewidth=0.3)
            #self.ax.set_yscale('log')
            self.ax.set_xticks([x*bar_height+ total_width/2.0 for x in range(len(data))])
            self.ax.set_xticklabels(self.data['plots']['x-val'])
    
    def GenerateOutput(self):
        self.ax.legend(loc=0)
        if "disable_legend" in self.data["plots"]:
            if self.data["plots"]["disable_legend"] == "1":
                self.ax.legend().remove()
        self.ax.set_xlabel(self.data["plots"]["x-label"])
        self.ax.set_ylabel(self.data["plots"]["y-label"])
        self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        #self.ax.minorticks_on()
        self.ax.grid(b=True, which='major',color='silver', linewidth=0.2, linestyle="-")
        #self.ax.grid(b=True, which='minor',color='silver', linewidth=0.1, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)
        
        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        box = self.ax.get_position()
        if "legend_outside" in self.data["plots"]:
            if self.data["plots"]["legend_outside"] == "1":
                self.ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
                self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5))

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight',pad_inches=0)

class HistogramPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        yaxis_stat = self.yaxis_stat
        xaxis_stat = self.xaxis_stat

        y_list = booksim_reader.read_stat(yaxis_stat,-1)

        x_list = booksim_reader.read_stat(xaxis_stat,-1)

        return x_list, y_list
    
    def CreatePlot(self):
        # FIXME: general stuff like figsave
        offset = 0.0
        # TODO: find offset
        for indx, f in enumerate(self.data["plots"]["simulation_files"]):
            br = BookSimReader(f)
            
            x_list, y_list = self.ObtainSeries(br)

            ### FIXME: Use user defined x value to read histogram
            bin_data_temp = eval(y_list[eval(self.data["plots"]["x-val"])])
            bin_data = list()
            for x in bin_data_temp:
                if x > 0:
                    bin_data.append(log(x,10))
                else:
                    bin_data.append(0)
            #lefts = [i-j for i,j in zip([indx]*len(bin_data),[0.5*x for x in bin_data])]
            offset = max(max(bin_data), offset)
        
        x_ticks = list()
        x_tick_labels = list()
        for indx, f in enumerate(self.data["plots"]["simulation_files"]):
            br = BookSimReader(f)
            
            x_list, y_list = self.ObtainSeries(br)

            ### FIXME: I don't like this code here. create self.color as list.
            ###        Here we only have to index that list
            colormap = self.CreateColormap()

            ### FIXME: Use user defined x value to read histogram
            bin_data_temp = eval(y_list[eval(self.data["plots"]["x-val"])])
            bin_data = list()
            for x in bin_data_temp:
                if x > 0:
                    bin_data.append(log(x,10))
                else:
                    bin_data.append(0)
            #lefts = [i-j for i,j in zip([indx]*len(bin_data),[0.5*x for x in bin_data])]
            lefts = [offset*indx - y for y in [0.5 * x for x in bin_data]]
            x_ticks.append(offset*indx)
            x_tick_labels.append(self.data["plots"]["legend"][indx])
            bar_height = eval(self.data["plots"]["y-height"])
            name = format_label(self.data["plots"]["legend"][indx])
            self.ax.barh([bar_height*x for x in range(len(bin_data))], bin_data, height=bar_height, left=lefts,
                    label=name,
                    color=eval(self.data["plots"]["colors"][indx]),
                    edgecolor='black', linewidth=0.3)
            self.ax.set_xticks(x_ticks)
            self.ax.set_xticklabels(x_tick_labels, fontsize=7, rotation=90)

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        self.ax.legend(loc=0)
        if "legend_outside" in self.data["plots"]:
            if self.data["plots"]["legend_outside"] == "1":
                self.ax.legend(loc="center left",bbox_to_anchor=(1, 0.5))
        if "disable_legend" in self.data["plots"]:
            if self.data["plots"]["disable_legend"] == "1":
                self.ax.legend().remove()
        self.ax.set_xlabel(format_label(self.data["plots"]["x-label"]))
        self.ax.set_ylabel(format_label(self.data["plots"]["y-label"]))
        self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        #self.ax.set_xlim(xmin= float(self.data["plots"]["x-min"]), xmax=float(self.data["plots"]["x-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        self.ax.minorticks_on()
        #self.ax.grid(b=True, which='major',color='gray', linewidth=0.05, linestyle="-")
        #self.ax.grid(b=True, which='minor',color='gray', linewidth=0.01, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)

        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        box = self.ax.get_position()
        self.ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
        self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5))

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight')

## FIXME: Does this class have some relation with HistogramPlotter?
class HorizontalHistogramPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        yaxis_stat = self.yaxis_stat
        xaxis_stat = self.xaxis_stat

        y_list = booksim_reader.read_stat(yaxis_stat,-1)

        if xaxis_stat == "load":
            inj_load_uses_flits = True
            if 'inj_data_uses_flits' in self.data['plots']:
                if int(self.data["plots"]["inj_data_uses_flits"]) > 0:
                    inj_load_uses_flits = True
                else:
                    inj_load_uses_flits = False
            x_list = booksim_reader.read_offered_load(inj_load_uses_flits)
        else:
            x_list = booksim_reader.read_stat(xaxis_stat,-1)


        return x_list, y_list
    
    def CreatePlot(self):
        # FIXME: general stuff like figsave
        """
        offset = 0.0
        # TODO: find offset
        for indx, f in enumerate(self.data["plots"]["simulation_files"]):
            br = BookSimReader(f)

            x_list, y_list = self.ObtainSeries(br)

            ### FIXME: Use user defined x value to read histogram
            bin_data_temp = eval(y_list[eval(self.data["plots"]["x-val"])])
            bin_data = list()
            for x in bin_data_temp:
                if x > 0:
                    bin_data.append(log(x,10))
                else:
                    bin_data.append(0)
            #lefts = [i-j for i,j in zip([indx]*len(bin_data),[0.5*x for x in bin_data])]
            offset = max(max(bin_data), offset)
        """

        x_ticks = list()
        x_tick_labels = list()
        width = 1.0/float(len(self.data["plots"]["simulation_files"]))
        for indx, f in enumerate(self.data["plots"]["simulation_files"]):
            br = BookSimReader(f)

            x_list, y_list = self.ObtainSeries(br)

            ### FIXME: I don't like this code here. create self.color as list.
            ###        Here we only have to index that list
            colormap = self.CreateColormap()

            ### FIXME: Use user defined x value to read histogram
            x_pos = 0
            x_val = eval(self.data["plots"]["x-val"])
            
            for pos, x in enumerate(x_list):
                if x_val > x:
                    x_pos = pos
                else:
                    break

            bin_data_temp = y_list[x_pos]
            bin_data = list()
            bin_data = bin_data_temp
            """
            for x in bin_data_temp:
                if x > 0:
                    bin_data.append(log(x,10))
                else:
                    bin_data.append(0)
            """
            #lefts = [i-j for i,j in zip([indx]*len(bin_data),[0.5*x for x in bin_data])]
            #lefts = [offset*indx - y for y in [0.5 * x for x in bin_data]]
            #x_ticks.append(offset*indx)
            #x_tick_labels.append(self.data["plots"]["legend"][indx])
            bar_height = eval(self.data["plots"]["y-height"])
            name = format_label(self.data["plots"]["legend"][indx])
            #self.ax.barh([bar_height*x for x in range(len(bin_data))], bin_data, height=bar_height, left=lefts,
            # Find 99% percentile:
            total_samples = sum(bin_data)
            max_samples = max(bin_data)
            accumulated_samples = 0
            percentile = 0
            for index, x in enumerate(bin_data):
                accumulated_samples += x
                if accumulated_samples / float(total_samples) >= 0.999:
                    percentile = bar_height*index
                    break
            self.ax.bar([bar_height*(x+width*indx) for x in range(len(bin_data))], bin_data, width=width*bar_height,
                    label=name,
                    color=eval(self.data["plots"]["colors"][indx]),
                    edgecolor='black', linewidth=0.3)
            self.ax.axvline(x=percentile, color='k', linestyle=':')
            print("percentile", percentile)
            self.ax.annotate('99.9%', (percentile*1.1,max_samples/4.0))

            self.ax.set_yscale('log')
            #self.ax.set_xticks(x_ticks)
            #self.ax.set_xticklabels(x_tick_labels, fontsize=7, rotation=90)

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        ncols = len(self.data['plots']['legend'])
        if "number_columns" in self.data["plots"]:
            ncols = int(self.data["plots"]["number_columns"])
        self.ax.legend(loc=0, ncol=ncols)
        if "legend_outside" in self.data["plots"]:
            if self.data["plots"]["legend_outside"] == "1":
                self.ax.legend(loc="center left",bbox_to_anchor=(1, 0.5))
        if "disable_legend" in self.data["plots"]:
            if self.data["plots"]["disable_legend"] == "1":
                self.ax.legend().remove()
        self.ax.set_xlabel(format_label(self.data["plots"]["x-label"]))
        self.ax.set_ylabel(format_label(self.data["plots"]["y-label"]))
        #self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        if 'y-max' in self.data['plots'] and 'y-min' in self.data['plots']:
            self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        self.ax.set_xlim(xmin= float(self.data["plots"]["x-min"]), xmax=float(self.data["plots"]["x-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        self.ax.minorticks_on()
        self.ax.grid(b=True, which='major',color='gray', linewidth=0.05, linestyle=":")
        #self.ax.grid(b=True, which='minor',color='gray', linewidth=0.01, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)

        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        #box = self.ax.get_position()
        #self.ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
        #self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5))

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight',pad_inches=0)

class ThroughputPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        #yaxis_stat = self.yaxis_stat
        #xaxis_stat = self.xaxis_stat
        yaxis_stat = "avg_accepted_flits"
        xaxis_stat = "load"

        y_list = booksim_reader.read_stat_ponderate_classes(yaxis_stat)
        x_list = booksim_reader.read_stat_ponderate_classes(xaxis_stat)

        return x_list, y_list
    
    def CreatePlot(self):
        
            
        colormap = self.CreateColormap()

        cfg_names = self.data["plots"]["configurations"]
        cfg_number = len(cfg_names)
        assert cfg_number == len(self.data["plots"]["simulation_files"])
            
        x_ticks = list()
        x_tick_labels = list()

        width = 1.0/cfg_number
        for cfg_indx, config in enumerate(self.data["plots"]["simulation_files"]):
            x_ticks = list()
            x_tick_labels = list()

            throughput = list()
            for indx, f in enumerate(config):
                if "EMPTY_COLUMN" not in f:
                    br = BookSimReader(f)
                    x_list, y_list = self.ObtainSeries(br)
                    # TODO: Take last element of y_list
                    throughput.append(y_list[-1])
                    x_ticks.append(indx) # TODO: one tick per config comparison
                    x_tick_labels.append(self.data["plots"]["legend"][indx])
                else:
                    throughput.append(0.0)
                
                #x_ticks.append(indx) # TODO: one tick per config comparison
                #x_tick_labels.append(self.data["plots"]["legend"][indx])
        
            
            ind = [x + cfg_indx*width for x in range(len(throughput))]

            self.ax.bar(ind, throughput, width=width, label=cfg_names[cfg_indx],
                    color=eval(self.data["plots"]["colors"][cfg_indx]),
                    edgecolor='black', linewidth=0.3)
            

            #name.append(self.data["plots"]["legend"][indx])


            # TODO: Group data somehow by configurations: (eg: SMART (Baseline) vs SMART++)
            # Could I introduce fake simulations in the simulation_files list to separate configurations by groups?
        
        #width = 0.9/len(self.data["plots"]["configurations"])
        #for indx, f in enumerate(self.data["plots"]["configurations"]):
            
            
        #tmp = np.arange(len(x_ticks)) 
        #self.ax.set_xticks(tmp + width/2)
        self.ax.set_xticks([x + width/2 for x in x_ticks])
        self.ax.set_xticklabels(x_tick_labels, rotation='vertical')

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        self.ax.legend(loc=4) #, ncol=len(self.data["plots"]["configurations"])) # 'loc=4 equivalent to lower right'
        #if "legend_outside" in self.data["plots"]:
        #    if self.data["plots"]["legend_outside"] == "1":
        #        self.ax.legend(loc="center left",bbox_to_anchor=(1, 0.5))
        #if "disable_legend" in self.data["plots"]:
        #    if self.data["plots"]["disable_legend"] == "1":
        #        self.ax.legend().remove()
        self.ax.set_xlabel(format_label(self.data["plots"]["x-label"]))
        self.ax.set_ylabel(format_label(self.data["plots"]["y-label"]))
        self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        #self.ax.set_xlim(xmin= float(self.data["plots"]["x-min"]), xmax=float(self.data["plots"]["x-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        #self.ax.minorticks_on()
        self.ax.grid(b=True, which='major', axis='y', color='gray', linewidth=0.05, linestyle="-")
        #self.ax.grid(b=True, which='minor',color='gray', linewidth=0.01, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)

        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        box = self.ax.get_position()
        #self.ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
        #self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5))

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight')


class ThroughputBaselinePlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        #yaxis_stat = self.yaxis_stat
        #xaxis_stat = self.xaxis_stat
        yaxis_stat = "avg_accepted_flits"
        xaxis_stat = "load"

        y_list = booksim_reader.read_stat_ponderate_classes(yaxis_stat)
        x_list = booksim_reader.read_stat_ponderate_classes(xaxis_stat)

        return x_list, y_list
    
    def CreatePlot(self):
        
            
        colormap = self.CreateColormap()

        cfg_names = self.data["plots"]["configurations"]
        cfg_number = len(cfg_names)
        assert cfg_number == len(self.data["plots"]["simulation_files"])
            
        x_ticks = list()
        x_tick_labels = list()

        #width = 1.0/cfg_number
        width = 1.0
        i = 0
        prev_i = 0
        for cfg_indx, config in enumerate(self.data["plots"]["simulation_files"]):
            #x_ticks = list()
            #x_tick_labels = list()

            throughput = list()
            prev_i += i+1
            i = 0
            for indx, f in enumerate(config):
                if "EMPTY_COLUMN" not in f:
                    br = BookSimReader(f)
                    x_list, y_list = self.ObtainSeries(br)
                    # TODO: Take last element of y_list
                    throughput.append(y_list[-1])
                    x_ticks.append(i+prev_i) # TODO: one tick per config comparison
                    #x_tick_labels.append(self.data["plots"]["legend"][indx])
                else:
                    throughput.append(0.0)
                
                i += 1
                #x_ticks.append(indx) # TODO: one tick per config comparison
                #x_tick_labels.append(self.data["plots"]["legend"][indx])
        
            
            #ind = [(i+x) + cfg_indx*width for x in range(len(throughput))]
            print("config: {}, i: {}, prev_i: {}".format(config,i, prev_i))
            ind = [(prev_i+x) for x in range(len(throughput))]

            self.ax.bar(ind, throughput, width=width, label=cfg_names[cfg_indx],
                    color=eval(self.data["plots"]["colors"][cfg_indx]),
                    edgecolor='black', linewidth=0.3)
            

            #name.append(self.data["plots"]["legend"][indx])


            # TODO: Group data somehow by configurations: (eg: SMART (Baseline) vs SMART++)
            # Could I introduce fake simulations in the simulation_files list to separate configurations by groups?
        
        #width = 0.9/len(self.data["plots"]["configurations"])
        #for indx, f in enumerate(self.data["plots"]["configurations"]):
            
            
        #tmp = np.arange(len(x_ticks)) 
        #self.ax.set_xticks(tmp + width/2)
        self.ax.set_xticks(x_ticks)
        #self.ax.set_xticklabels(x_tick_labels, rotation='vertical')
        self.ax.set_xticklabels(self.data["plots"]["legend"], rotation='vertical')

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        self.ax.legend(loc=4) #, ncol=len(self.data["plots"]["configurations"])) # 'loc=4 equivalent to lower right'
        #if "legend_outside" in self.data["plots"]:
        #    if self.data["plots"]["legend_outside"] == "1":
        #        self.ax.legend(loc="center left",bbox_to_anchor=(1, 0.5))
        #if "disable_legend" in self.data["plots"]:
        #    if self.data["plots"]["disable_legend"] == "1":
        #        self.ax.legend().remove()
        self.ax.set_xlabel(format_label(self.data["plots"]["x-label"]))
        self.ax.set_ylabel(format_label(self.data["plots"]["y-label"]))
        self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        #self.ax.set_xlim(xmin= float(self.data["plots"]["x-min"]), xmax=float(self.data["plots"]["x-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        #self.ax.minorticks_on()
        self.ax.grid(b=True, which='major', axis='y', color='gray', linewidth=0.05, linestyle="-")
        #self.ax.grid(b=True, which='minor',color='gray', linewidth=0.01, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)

        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        box = self.ax.get_position()
        #self.ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
        #self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5))

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight')

class ZeroLatencyPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.

    def GenerateStyle(self):
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

    def ObtainSeries(self, booksim_reader):
        #yaxis_stat = self.yaxis_stat
        #xaxis_stat = self.xaxis_stat
        yaxis_stat = "avg_plat"
        xaxis_stat = "load"

        y_list = booksim_reader.read_stat_ponderate_classes(yaxis_stat)
        x_list = booksim_reader.read_stat_ponderate_classes(xaxis_stat)

        return x_list, y_list
    
    def CreatePlot(self):
        
            
        colormap = self.CreateColormap()

        cfg_names = self.data["plots"]["configurations"]
        cfg_number = len(cfg_names)
        assert cfg_number == len(self.data["plots"]["simulation_files"])
            
        x_ticks = list()
        x_tick_labels = list()

        width = 1.0/cfg_number
        for cfg_indx, config in enumerate(self.data["plots"]["simulation_files"]):
            x_ticks = list()
            x_tick_labels = list()

            throughput = list()
            for indx, f in enumerate(config):
                if "EMPTY_COLUMN" not in f:
                    br = BookSimReader(f)
                    x_list, y_list = self.ObtainSeries(br)
                    # TODO: Take last element of y_list
                    throughput.append(y_list[0])
                    x_ticks.append(indx) # TODO: one tick per config comparison
                    x_tick_labels.append(self.data["plots"]["legend"][indx])
                else:
                    throughput.append(0.0)
                
                #x_ticks.append(indx) # TODO: one tick per config comparison
                #x_tick_labels.append(self.data["plots"]["legend"][indx])
        
            
            ind = [x + cfg_indx*width for x in range(len(throughput))]

            self.ax.bar(ind, throughput, width=width, label=cfg_names[cfg_indx],
                    color=eval(self.data["plots"]["colors"][cfg_indx]),
                    edgecolor='black', linewidth=0.3)
            

            #name.append(self.data["plots"]["legend"][indx])


            # TODO: Group data somehow by configurations: (eg: SMART (Baseline) vs SMART++)
            # Could I introduce fake simulations in the simulation_files list to separate configurations by groups?
        
        #width = 0.9/len(self.data["plots"]["configurations"])
        #for indx, f in enumerate(self.data["plots"]["configurations"]):
            
            
        #tmp = np.arange(len(x_ticks)) 
        #self.ax.set_xticks(tmp + width/2)
        self.ax.set_xticks([x + width/2 for x in x_ticks])
        self.ax.set_xticklabels(x_tick_labels, rotation='horizontal')

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        self.ax.legend(loc=0) #, ncol=len(self.data["plots"]["configurations"])) # 'loc=4 equivalent to lower right'
        #if "legend_outside" in self.data["plots"]:
        #    if self.data["plots"]["legend_outside"] == "1":
        #        self.ax.legend(loc="center left",bbox_to_anchor=(1, 0.5))
        #if "disable_legend" in self.data["plots"]:
        #    if self.data["plots"]["disable_legend"] == "1":
        #        self.ax.legend().remove()
        self.ax.set_xlabel(format_label(self.data["plots"]["x-label"]))
        self.ax.set_ylabel(format_label(self.data["plots"]["y-label"]))
        self.ax.set_ylim(ymin= float(self.data["plots"]["y-min"]), ymax=float(self.data["plots"]["y-max"]))
        #self.ax.set_xlim(xmin= float(self.data["plots"]["x-min"]), xmax=float(self.data["plots"]["x-max"]))
        if "title" in self.data["plots"]:
            self.ax.set_title(self.data["plots"]["title"])
        #self.ax.minorticks_on()
        self.ax.grid(b=True, which='major', axis='y', color='gray', linewidth=0.05, linestyle="-")
        #self.ax.grid(b=True, which='minor',color='gray', linewidth=0.01, linestyle=":")
       
        # FIXME: Check if output path exists
	#	 Current implementation only supports linux filesystems
        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)

        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        
        box = self.ax.get_position()
        #self.ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
        #self.ax.legend(loc='center left', bbox_to_anchor=(1,0.5))

        self.fig.savefig(self.data["plots"]["output"], bbox_inches='tight')

class LegendPlotter(Plotter):
    def __init__(self, data):
        super().__init__(data)
        # FIXME: initialize particular members of the child class.
        print('__init__ not implemented')

    def GenerateStyle(self):
        print('GenerateStyle not implemented')
        if "linestyle" in self.data["plots"]:
            self.linestyle = self.data["plots"]["linestyle"]
        else:
            self.linestyle = ["-" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markers" in self.data["plots"]:
            self.markers = self.data["plots"]["markers"]
        else:
            self.markers = ["" for x in range(len(self.data["plots"]["simulation_files"]))]

        if "markersize" in self.data["plots"]:
            self.markersize = int(self.data["plots"]["markersize"])
        else:
            self.markersize = 7

    def ObtainSeries(self, booksim_reader):
        print('ObtainSeries not implemented')
        y_list = list()
        x_list = list()
        return x_list, y_list

    def CreatePlot(self):
        print('CreatePlot not implemented')

    ### FIXME: This code is a shiiit
    def GenerateOutput(self):
        fig = pylab.figure()
        for indx, f in enumerate(self.data["plots"]["legend"]):

            colormap = self.CreateColormap()

            fig.gca().plot(range(10), range(10), label=self.data["plots"]["legend"][indx],
                    color=eval(self.data["plots"]["colors"][indx]),
                    linestyle=self.linestyle[indx], marker=self.markers[indx],
                    mec='black', markersize=self.markersize, markeredgewidth=0.5 , lw=1)
        legend_fig = pylab.figure()
        columns = len(self.data['plots']['legend'])
        if 'legend_columns' in self.data['plots']:
            columns = int(self.data['plots']['legend_columns'])
        if 'legend_title' in self.data['plots']:
            print("HELLOOO")
            legend = pylab.figlegend(*fig.gca().get_legend_handles_labels(),
                                     title=self.data['plots']['legend_title'],
                                     loc = 'center',
                                     ncol=columns)
        else:
            print("NOOOOOOOOOOOO")
            legend = pylab.figlegend(*fig.gca().get_legend_handles_labels(),
                                     loc = 'center',
                                     ncol=columns)


        output_dir = ""
        for x in self.data["plots"]["output"].split("/")[0:-1]:
            output_dir += x + "/"
        print('output_dir: ', output_dir)
        if output_dir != "":
            try:
                os.makedirs(output_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise

        legend_fig.savefig(self.data["plots"]["output"], bbox_inches='tight',
                           pad_inches=0)
        #legend_fig.canvas.draw()
        #rend = legend_fig.canvas.get_render()
        #legend_fig.savefig(self.data["plots"]["output"],
        #                bbox_inches=legend.get_window_extent(rend).transformed(legend_fig.dpi_scale_trans.inverted())
        #        )

### TODO: Remove this, we don't have "time" in standalone booksim simulations?
######################################
def plat_ponderated(group):
    stat = group["avg_plat"]
    multiplier = group["avg_sent_packets"]

    return (stat*multiplier).sum() / multiplier.sum()

def nlat_ponderated(group):
    stat = group["avg_nlat"]
    multiplier = group["avg_sent_packets"]

    return (stat*multiplier).sum() / multiplier.sum()

def bypassed_flits_ponderated(group):
    stat = group["bypassed_flits"]
    multiplier = group["avg_sent_flits"]

    return (stat*multiplier).sum() / multiplier.sum()
######################################

if __name__ == '__main__':
    #main_old()
    main()
