/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010, Lawrence Livermore National Security, LLC.  
// Produced at the Lawrence Livermore National Laboratory  
// Written by Todd Gamblin, tgamblin@llnl.gov.
// LLNL-CODE-417602
// All rights reserved.  
// 
// This file is part of Libra. For details, see http://github.com/tgamblin/libra.
// Please also read the LICENSE file for further information.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the disclaimer below.
//  * Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the disclaimer (as noted below) in the documentation and/or other materials
//    provided with the distribution.
//  * Neither the name of the LLNS/LLNL nor the names of its contributors may be used to endorse
//    or promote products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// LAWRENCE LIVERMORE NATIONAL SECURITY, LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef EFFORT_PARAMS_H
#define EFFORT_PARAMS_H

#include <ostream>
#include <vector>
#include <set>
#include "effort_key.h"
#include "env_config.h"
#include "Metric.h"

/// TODO: make it so char*'s in here don't leak.

namespace effort {

  /// This truct encapsulates effort module parameters 
  /// and their default values.
  struct effort_params {
    int rows_per_process;     /// # of rows consolidated to each compressor process
    bool verify;              /// Whether or not to dump exact data.
    int pass_limit;           /// Limit on number of EZW passes output (compression level)
    long long scale;          /// Scaling factor for double-precision numbers input to EZW coder.
    bool sequential;          /// Whether EZW bit-ordering is per sequential algorithm.  Very slow!
    const char *encoding;     /// Encoding to use.  Options are "rle", "arithmetic", "huffman", "none"

    const char *metrics;      /// Comma-separated list of all metrics to monitor.  Possible values are 
                              /// PAPI metric names or "time".  This is a string.  Access metrics as 'Metric' objects
                              /// through get_metrics() below.

    bool chop_libc;           /// Whether to chop libc_start_main calls
    const char *regions;      /// Controls how to delineate effort regions in the code.  Can be effort, comm, or both.
    long long sampling;       /// Sampling rate for progress steps.  Defaults to 1.  If set higher, only rolls over 
                              /// progress every so many actual timesteps.

    bool topo;                /// alternately outputs topology-ordered compressed data.
    bool dump_keys;           /// Dump all effort keys to a file in MPI_Finalize.  Default is false.

    bool ampl;                /// AMPL mode -- uses AMPL for sampling and outputs sampled trace.
    double confidence;        /// AMPL confidence
    double error;             /// AMPL error
    bool normalized_error;    /// Whether error is absolute or normalized bt/w 0 and 1.  Default false.
    int windows_per_update;   /// AMPL windows per update.
    bool ampl_stats;          /// Whether AMPL should write stats for eventsin its log.
    bool ampl_trace;          /// Whether AMPL should write traces
    int ampl_strata;          /// Max strata to produce for AMPL auto-stratification.  Default is 1 (no stratification)
    int ampl_sig_level;       /// Transform Level for signatures used in clustering. Defaults to -1.
    const char *ampl_guide;   /// identifier for region to guide sampling


    /// Constructor with default values of all parameters.
    effort_params() 
      : rows_per_process(32), 
        verify(0), 
        pass_limit(5), 
        scale(1 << 10), 
        sequential(0), 
        encoding("huffman"), 
        metrics("time"),
        chop_libc(false),
        regions("effort"),
        sampling(1),
        topo(false),
        dump_keys(false),
        ampl(false),
        confidence(.90),
        error(.08),
        normalized_error(false),
        windows_per_update(32),
        ampl_stats(false),
        ampl_trace(true),
        ampl_strata(1),
        ampl_sig_level(-1),
        ampl_guide(""),
        have_time(false),
        parsed(false)
    { /* constructor just inits things. */ }


    /// Returns a config_desc array suitable for passing to env_get_configuration()
    config_desc *get_config_arguments();
    
    /// parses metrics into metric_names vector.
    void parse_metrics();
    void parse_keys();
    void parse();

    /// Returns name mapping for a particular metric id.  METRIC_TIME
    const std::vector<Metric>& get_metrics() {
      if (!parsed) parse();
      return all_metrics;
    }

    size_t m_to_i(Metric m) {
      return metric_to_index[m];
    }

    bool has_metric(Metric m) {
      return metric_to_index.count(m);
    }

    size_t num_metrics() {
      if (!parsed) parse();
      return all_metrics.size() + (have_time ? 1 : 0);
    }
    
    /// Whether user wants us to track time.
    bool keep_time() {
      if (!parsed) parse();
      return have_time;
    }

    const std::set<effort_key> guide_keys() {
      if (!parsed) parse();
      return guide_list;
    }

  private:
    std::vector<Metric> all_metrics;             /// Parsed metric names, in the order they apeared in this->metrics
    std::map<Metric, size_t> metric_to_index;    /// Map from metric to index in array.

    std::set<effort_key> guide_list;          /// Effort keys to guide ampl sampling.

    bool have_time;               /// memo-ized record of whether 
    bool parsed;                  /// whether metrics were parsed yet.
  };

  std::ostream& operator<<(std::ostream& out, effort_params& params);

} //namespace


#endif //EFFORT_PARAMS_H
