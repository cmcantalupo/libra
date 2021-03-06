#################################################################################################
# Copyright (c) 2010, Lawrence Livermore National Security, LLC.  
# Produced at the Lawrence Livermore National Laboratory  
# Written by Todd Gamblin, tgamblin@llnl.gov.
# LLNL-CODE-417602
# All rights reserved.  
# 
# This file is part of Libra. For details, see http://github.com/tgamblin/libra.
# Please also read the LICENSE file for further information.
# 
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
# 
#  * Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the disclaimer below.
#  * Redistributions in binary form must reproduce the above copyright notice, this list of
#    conditions and the disclaimer (as noted below) in the documentation and/or other materials
#    provided with the distribution.
#  * Neither the name of the LLNS/LLNL nor the names of its contributors may be used to endorse
#    or promote products derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# LAWRENCE LIVERMORE NATIONAL SECURITY, LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#################################################################################################
#
# Keeps vtkLoadFunctions for effort regions cached by
# their file names.  Also provides algorithms for building
# other components out of effort data.
#
from vtkeffort import *
from pyeffort import *
import effort_tree, dendrogram, sys, os, re

wrapperfile = re.compile("^.*_wrapper.C$")

def trimCallpath(callpath):
    i=0
    for frame in callpath:
        if frame.file() and not wrapperfile.match(frame.file()):
            break
        i+= 1
    foo = callpath.slice(i)
    return foo

class EffortRegion:
    def __init__(self):
        # Set up a hash of all the data by metric for this region.
        self._metrics = {}
        self._start = self._end = self._type = None

    def addData(self, data):
        if self._metrics:
            assert self._start == trimCallpath(data.getStart())
            assert self._end   == trimCallpath(data.getEnd())
            assert self._type  == data.getType()
        else:
            self._start = trimCallpath(data.getStart())
            self._end   = trimCallpath(data.getEnd())
            self._type  = data.getType()
        
        self._metrics[data.getMetric()] = data

    def key(self):
        return (self._start, self._end, self._type)

    def start(self):
        return self._start

    def end(self):
        return self._end

    def type(self):
        return self._type
    
    def dataFor(self, metric):
        return self._metrics[metric]

    def hasMetric(self, metric):
        return metric in self._metrics.keys()

    def metrics(self):
        for m in self._metrics: yield m

    def firstMetric(self):
        if not self._metrics or "time" in self._metrics.keys():
            return "time"
        else:
            return self._metrics.keys()[0]

    def __getattr__(self, attr):
        if attr in self._metrics:
            return self._metrics[attr]
        else:
            raise AttributeError, self.__class__.__name__ + \
                  " has no attribute named " + attr


#
# Wrapper to display data in a GUI from a particular frame in a callpath.
#
class FrameViewWrapper:
    def __init__(self, frame=None):
        self._frame = frame
    
    @classmethod
    def fromIndex(cls, callpath, index):
        if index in xrange(0, len(callpath)):
            return cls(callpath[index])
        else:
            return cls(None)

    def prettyFile(self):
        if not self._frame or not self._frame.file():
            return None
        return "%s:%d" % (self._frame.file(), self._frame.line())

    def file(self):
        if not self._frame: return None
        else: return self._frame.file()

    def line(self):
        """Returns line in file where frame came from, or 0 if frame is not valid."""
        if not self._frame: return 0
        else: return self._frame.line()

    def fun(self):
        if not self._frame: return None
        else: return self._frame.fun()

    def prettyModule(self, long=False):
        if not self._frame or not self._frame.module(): 
            return None

        module = self._frame.module()
        if not long:
            module = re.sub("^.*[^/]/", "", module)
        return "%s(0x%x)" % (module, self._frame.offset())

    def prettyLocation(self):
        if not self._frame or not self._frame.file():
            return "unknown"
        return "%s(%s)" % (self.fun(), self.prettyFile())

    def name(self):
        if not self._frame:
            return "none"
        elif not self._frame.fun():
            return self.prettyModule()
        else:
            return self._frame.fun()


class DB:
    def __init__(self, approx = -1, passes = 0):
        # Map of effort regions keyed by tuples of (start cp, end cp, type)
        self._regions = []
        self.approximationLevel = approx
        self.passLimit = passes
        self.totalTime = 0                 # total time in nanoseconds

    #
    # Loads a directory full of effort files into the regions map.
    # EffortRegions are keyed by start callpath, end callpath, and
    # effort type.
    #
    def loadDirectory(self, dir):
        regions = {}

        for file in os.listdir(dir):
            if file.startswith("effort"):
                parts = file.split("-")

                # make sure file's name is valid.
                if not len(parts) == 4:
                    continue

                data = EffortData(file)   # TODO: need to prepend dir here.

                self.steps = data.steps()
                self.processes = data.processes()
                self.vprocs = data.rows()

                data.setApproximationLevel(self.approximationLevel)
                data.setPassLimit(self.passLimit)

                key = (data.getStart(), data.getEnd(), data.getType())
                if not key in regions:
                    region = EffortRegion()
                    regions[key] = region
                    
                regions[key].addData(data)

            elif file == "times":
                f = open(file)
                self.totalTime = float(f.next().split()[1]) * 1e9 # sec -> ns
                f.close()

        # Dump map into list when done.
        self._regions = regions.values()
                

    def setApproximationLevel(self, level):
        self.approximationLevel = level

    def __len__(self):
        return len(self._regions)

    def __getitem__(self, key):
        return self._regions[key]

    def __contains__(self, item):
        return item in self._regions

    def __iter__(self):
        for r in self._regions:
            yield r

