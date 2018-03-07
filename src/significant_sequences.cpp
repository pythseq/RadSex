#include "significant_sequences.h"

void significant_sequences(Parameters& parameters) {

    /* The significant_sequences function parses through a file generated by process_reads and outputs sequences significantly associated with sex.
     * Association with sex is determined using a Chi-squared test, and p-value is corrected with Bonferroni correction.
     * - Found in M males with min_males <= M <= max_males
     * - Found in F females with min_females <= F <= max_females
     */

    std::unordered_map<std::string, bool> popmap = load_popmap(parameters);

    uint total_males = 0, total_females = 0;
    for (auto i: popmap) if (i.second) ++total_males; else ++total_females;

    std::string par = "input_file_path";
    std::ifstream input_file;
    input_file.open(parameters.get_value_from_name<std::string>(par));

    par = "min_cov";
    int min_cov = parameters.get_value_from_name<int>(par) - 1; // -1 allows comparison with > instead of >=

    if (input_file) {

        par = "output_file_path";
        std::ofstream output_file;
        output_file.open(parameters.get_value_from_name<std::string>(par));

        // First line is the header. The header is parsed to get the sex of each field in the table.
        std::vector<std::string> line;
        std::string temp = "";
        std::getline(input_file, temp);
        output_file << temp << "\n"; // Copy the header line to the subset output file
        line = split(temp, "\t");

        // Map with column number --> index of sex_count (0 = male, 1 = female, 2 = no sex)
        std::unordered_map<uint, uint> sex_columns;

        // Detection of individuals is based on the popmap, so individuals without sex should still be in the popmap
        for (uint i=0; i<line.size(); ++i) {
            if (popmap.find(line[i]) != popmap.end()) {
                if (popmap[line[i]]) {
                    sex_columns[i] = 0; // Male --> column 0
                } else {
                    sex_columns[i] = 1; // Female --> column 1
                }
            } else {
                sex_columns[i] = 2; // First and second columns (id and sequence) are counted as no sex
            }
        }

        // Define variables used to read the file
        char buffer[65536];
        std::string temp_line;
        uint k = 0, field_n = 0, seq_count = 0;
        int sex_count[3] = {0, 0, 0}; // Index: 0 = male, 1 = female, 2 = no sex information
        double chi_squared = 0, p = 0;

        std::map<std::string, double> candidate_sequences;

        do {

            // Read a chunk of size given by the buffer
            input_file.read(buffer, sizeof(buffer));
            k = input_file.gcount();

            for (uint i=0; i<k; ++i) {

                // Read the buffer character by character
                switch(buffer[i]) {

                case '\r':
                    break;
                case '\t':  // New field
                    if (sex_columns[field_n] != 2 and std::stoi(temp) > min_cov) ++sex_count[sex_columns[field_n]];  // Increment the appropriate counter
                    temp = "";
                    temp_line += buffer[i];
                    ++field_n;
                    break;
                case '\n':  // New line (also a new field)
                    if (sex_columns[field_n] != 2 and std::stoi(temp) > min_cov) ++sex_count[sex_columns[field_n]];  // Increment the appropriate counter
                    if (sex_count[0] + sex_count[1] > 0) {
                        ++seq_count;
                        chi_squared = get_chi_squared(sex_count[0], sex_count[1], total_males, total_females);
                        p = get_chi_squared_p(chi_squared);
                        if (p < 0.05) { // First pass: we filter sequences with at least one male or one female and non-corrected p < 0.05
                            candidate_sequences[temp_line] = p;
                        }
                    }
                    // Reset variables
                    temp = "";
                    temp_line = "";
                    field_n = 0;
                    sex_count[0] = 0;
                    sex_count[1] = 0;
                    break;
                default:
                    temp += buffer[i];
                    temp_line += buffer[i];
                    break;
                }
            }

        } while(input_file);

        double significance_threshold = 0.05 / seq_count; // Bonferroni correction: divide threshold by number of tests

        // Second pass: filter with bonferroni
        for (auto sequence: candidate_sequences) {
            if (sequence.second < significance_threshold) {
                output_file << sequence.first << "\n";
            }
        }

        output_file.close();
        input_file.close();
    }
}

double get_chi_squared_p(double chi_squared) {

    /* p is given by 1 - P(chi_squared, df) where P is the Cumulative Distribution Function of the Chi-squared distribution.
     * P is also the regularized gamma function. Here we use samtool's implementation of the regularized gamma function by Hen Li.
     * Source: https://en.wikipedia.org/wiki/Chi-squared_distribution#Cumulative_distribution_function
     * DF is always 1 in our case
     */

    return 1 - kf_gammap(0.5, chi_squared/2);
}


double get_chi_squared(uint n_males, uint n_females, uint total_males, uint total_females) {

    /* Chi squared is computed from the number of males and females with the sequence, as well as total number of males and females in the population.
     * Yates correction is applied and the shortcut formula for 2x2 table is used.
     * Source: https://en.wikipedia.org/wiki/Yates%27s_correction_for_continuity
     */

    uint N = total_males + total_females;
    uint Ns = total_males, Nf = total_females;
    uint Na = n_males + n_females, Nb = total_males + total_females - n_males - n_females;

    int temp = (n_males * total_females) - (n_females * total_males);
    temp = std::abs(temp);
    double temp2 = std::max(0.0, double(temp) - N/2);
    temp2 *= temp2;

    return N * temp2 / Ns / Nf / Na / Nb;
}
