#ifndef GM2_FIELD_CORE_INCLUDE_FIXED_PROBE_SEQUENCER_HH_
#define GM2_FIELD_CORE_INCLUDE_FIXED_PROBE_SEQUENCER_HH_

/*============================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   event_manager_trg_seq.hh

  about:  Implements an event builder that sequences each channel of
          a set of multiplexers according to a json config file
	  which defines the trigger sequence.

\*============================================================================*/


//--- std includes -----------------------------------------------------------//
#include <string>
#include <map>
#include <vector>
#include <cassert>

//--- other includes ---------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <zmq.hpp>
#include "fid.h"

//--- project includes -------------------------------------------------------//
#include "event_manager_base.hh"
#include "dio_mux_controller.hh"
#include "dio_trigger_board.hh"
#include "shim_structs.hh"

namespace gm2 {

class FixedProbeSequencer: public daq::EventManagerBase {

public:

  //ctor
  FixedProbeSequencer(int num_probes);
  FixedProbeSequencer(std::string conf_file, int num_probes);

  //dtor
  ~FixedProbeSequencer();

  // Simple initialization.
  int Init();

  // Launches any threads needed and start collecting data.
  int BeginOfRun();

  // Rejoins threads and stops data collection.
  int EndOfRun();

  int ResizeEventData(daq::event_data &data);

  // Issue a software trigger to take another sequence.
  inline int IssueTrigger() {
    got_software_trg_ = true;
  }

  // Returns the oldest stored event.
  inline const nmr_data_vector GetCurrentEvent() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (!run_queue_.empty()) {

      return run_queue_.front();

    } else {

      nmr_data_vector tmp;
      tmp.Resize(num_probes_);
      return tmp;
    }
  };

  // Removes the oldest event from the front of the queue.
  inline void PopCurrentEvent() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (!run_queue_.empty()) {
      run_queue_.pop();
    }
    if (run_queue_.empty()) {
      has_event_ = false;
    }
  };

private:

  // This is used the logging facility.
  const std::string name_ = "FixedProbeSequencer";

  int max_event_time_;
  int num_probes_;
  std::atomic<bool> got_software_trg_;
  std::atomic<bool> got_start_trg_;
  std::atomic<bool> got_round_data_;
  std::atomic<bool> builder_has_finished_;
  std::atomic<bool> sequence_in_progress_;
  std::atomic<bool> mux_round_configured_;
  std::atomic<bool> analyze_fids_online_;
  std::atomic<bool> use_fast_fids_class_;
  std::string trg_seq_file_;
  std::string mux_conf_file_;
  std::string fid_conf_file_;

  int nmr_trg_mask_;
  int mux_switch_time_;
  daq::DioTriggerBoard *nmr_pulser_trg_;
  std::vector<daq::DioMuxController *> mux_boards_;
  std::map<std::string, int> mux_idx_map_;
  std::map<std::string, int> sis_idx_map_;
  std::map<std::string, std::pair<std::string, int>> data_in_;
  std::map<std::pair<std::string, int>, std::pair<std::string, int>> data_out_;
  std::vector<std::vector<std::pair<std::string, int>>> trg_seq_;

  std::queue<nmr_data_vector> run_queue_;
  std::mutex queue_mutex_;
  std::thread trigger_thread_;
  std::thread builder_thread_;
  std::thread starter_thread_;

  // Collects from data workers, i.e., direct from the waveform digitizers.
  void RunLoop();

  // Listens for sequence start signals.
  void StarterLoop();

  // Runs through the trigger sequence by configuring multiplexers for
  // several rounds and interacting with the BuilderLoop to ensure data
  // capture.
  void TriggerLoop();

  // Builds the event by selecting the proper data from each round.
  void BuilderLoop();
};

} // ::daq

#endif

