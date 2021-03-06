#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdio>
#include "utils.h"
#include "stats.h"

// Create output file for the process reads analysis
void output_process_reads(std::string& output_file_path, std::vector<std::string>& individuals, std::unordered_map<std::string, std::unordered_map<std::string, uint16_t>>& results, uint min_cov);

// Create output file for the sex_distribution analysis
void output_sex_distribution_matrix(std::string& output_file_path, sd_table& results, uint n_males, uint n_females);

// Create output file for the sex_distribution analysis
void output_sex_distribution(std::string& output_file_path, sd_table& results, uint n_males, uint n_females);

// Create output file for the group_loci analysis
void output_group_loci(std::string& output_file_path, std::unordered_map<std::string, std::vector<Locus>>& results, std::vector<std::string>& header);

// Create output file for the mapping analysis
void output_mapping(std::string& output_file_path, std::vector<MappedSequence> sequences);
