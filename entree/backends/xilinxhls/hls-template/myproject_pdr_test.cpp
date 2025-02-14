// Copyright 2022 Novel, Emerging Computing System Technologies Laboratory 
//                (NECSTLab), Politecnico di Milano

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
//    rfnoc-hls-neuralnet: Vivado HLS code for neural-net building blocks
//
//    Copyright (C) 2017 EJ Kreinar
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "firmware/myproject.h"
#include "firmware/parameters.h"
#include "firmware/BDT.h"

#define CHECKPOINT 5000

template<typename packet_t, int in_count, int out_count>
void axis_crossbar(
	hls::stream<packet_t> in[in_count],
	hls::stream<packet_t> out[out_count]
) {
	for (unsigned int i = 0; i < in_count; i++) {
		while (!in[i].empty()) {
			packet_t pkt;
			in[i] >> pkt;

			if (in_count > 1) {
				// N:1?
				unsigned int dest = 0;

				if (out_count > 1) {
					// N:M
					dest = pkt.dest;
				}

				if (dest > out_count) {
					std::cout << "Unable to route axis packet from stream " << i << " to stream " << dest << std::endl;
					continue;
				}

				out[dest] << pkt;
			} else {
				// 1:N
				for (unsigned int j = 0; j < out_count; j++) {
					out[j] << pkt;
				}
			}
		}
	}
}

template<typename pkt_t, typename data_t>
data_t tee(hls::stream<pkt_t> &in, hls::stream<pkt_t> &out) {
	pkt_t pkt;

	in >> pkt;
	out << pkt;

	return pkt.data;
}

int main(int argc, char **argv)
{
  //load input data from text file
  std::ifstream fin("tb_data/tb_input_features.dat");

#ifdef RTL_SIM
  std::string RESULTS_LOG = "tb_data/cosim_results.log";
  std::string VERBOSE_LOG = "tb_data/cosim_tree_results.log";
#else
  std::string RESULTS_LOG = "tb_data/csim_results.log";
  std::string VERBOSE_LOG = "tb_data/csim_tree_results.log";
#endif
  std::ofstream fout(RESULTS_LOG);
  std::ofstream ftrees(VERBOSE_LOG);

  std::string iline;
  std::string pline;
  int e = 0;

  if (fin.is_open()) {
    static hls::stream<input_arr_s_t> sample_stream[1];
    static hls::stream<bank_command_s_t> bank_command_stream[1];
    static hls::stream<input_arr_s_t> bank_stream[bank_count];
    static hls::stream<input_arr_s_t> tree_stream[n_trees * n_classes];
    static hls::stream<tree_score_s_t> aux_score_stream[n_trees * n_classes];
    static hls::stream<tree_score_s_t> score_stream[n_trees * n_classes];
    static hls::stream<tree_score_s_t> in_class_stream[n_classes];
    static hls::stream<class_score_s_t> out_class_stream[n_classes];

    int curr_id = max_parallel_samples - 1;

    while ( std::getline(fin,iline) ) {
      if (e % CHECKPOINT == 0) std::cout << "Processing input " << e << std::endl;
      e++;
      char* cstr=const_cast<char*>(iline.c_str());
      char* current;
      std::vector<float> in;
      current=strtok(cstr," ");
      while(current!=NULL) {
        in.push_back(atof(current));
        current=strtok(NULL," ");
      }

      input_arr_s_t in_pkt;
      //hls-fpga-machine-learning insert data
      curr_id = (curr_id + 1) % max_parallel_samples;
      in_pkt.id = curr_id;

      //hls-fpga-machine-learning insert top-level-function
      
      for(int  i = 0; i < n_trees; i++){
          for(int j = 0; j < BDT::fn_classes(n_classes); j++){
            ftrees << tree_scores[i * BDT::fn_classes(n_classes) + j] << " ";
          }
      }
      ftrees << std::endl;
    }

    for (int i = 0; i < BDT::fn_classes(n_classes); i++) {
    	tree_score_s_t pkt;
		  pkt.last = true;
    	in_class_stream[i] << pkt;
    }

    //hls-fpga-machine-learning insert final-round

    while(
      //hls-fpga-machine-learning insert stream-check
    ) {

      for(int i = 0; i < BDT::fn_classes(n_classes); i++){
        class_score_s_t score;
        if (!out_class_stream[i].empty()) {
          out_class_stream[i] >> score;
          fout << score.data << " ";
        } else {
          fout << "NaN ";
        }
      }
      fout << std::endl;
    }

    fin.close();
  } else {
    std::cout << "CRITICAL: Unable to open input file." << std::endl;
	  return -1;
  }

  fout.close();
  ftrees.close();
  std::cout << "INFO: Saved inference results to file: " << RESULTS_LOG << std::endl;

  return 0;
}
