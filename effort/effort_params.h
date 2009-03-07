#ifndef EFFORT_PARAMS_H
#define EFFORT_PARAMS_H

#include <ostream>
#include "effort_key.h"
#include "env_config.h"

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
                              /// PAPI metric names or "time".  This is a string.  Access metric names 
                              /// conveniently through metric_name(int) below.
    bool chop_libc;           /// Whether to chop libc_start_main calls
    const char *regions;      /// Controls how to delineate effort regions in the code.  Can be effort, comm, or both.
    long long sampling;       /// Sampling rate for progress steps.  Defaults to 1.  If set higher, only rolls over 
                              /// progress every so many actual timesteps.
    
    /// Constructor with default values of all parameters.
    effort_params() 
      : rows_per_process(32), 
        verify(0), 
        pass_limit(5), 
        scale(1 << 10), 
        sequential(0), 
        encoding("huffman"), 
        metrics(METRIC_TIME),
        chop_libc(false),
        regions("effort"),
        sampling(1),
        keep_time(true);
    { /* constructor just inits things. */ }


    /// Returns a config_desc array suitable for passing to env_get_configuration()
    config_desc *get_config_arguments();
    
    /// parses metrics into metric_names vector.
    void parse_metrics();

    /// Returns name mapping for a particular metric id.  METRIC_TIME
    const string& metric_name(int id);

    /// Total number of metrics including time.
    size_t num_metrics();
    
    /// Whether user wants us to track time.
    bool keep_time();

  private:
    /// parsed out version of metrics.  Time is always -1, others are the order they appeared in
    /// the metrics string, starting with 0.
    vector<string> metric_names;
    bool keep_time;
  };

  std::ostream& operator<<(std::ostream& out, const effort_params& params);

} //namespace


#endif //EFFORT_PARAMS_H
