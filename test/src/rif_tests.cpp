/* rif_tests Performs benchmarks on the constructed R-Index-F --- must serialize both r-index-f and LF_table
    Copyright (C) 2021 Nathaniel Brown
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/ .
*/
/*!
   \file rif_tests.cpp
   \brief rif_tests Benchmark tests on the R-Index-F
   \author Nathaniel Brown
   \author Massimiliano Rossi
   \date 02/11/2021
*/

#include <iostream>
#include <fstream> 

#define VERBOSE

#include <common.hpp>
#include <r_index_f.hpp>
#include <malloc_count.h>

// Check inversion is correct (compared to explicit table) - FAIL FOR ACGT_MAP
void test_invert(r_index_f<> rif, LF_table table) 
{
    ulint steps = 0;
    interval_pos pos(0,0);
    char c_rif;
    while((c_rif = rif.get_char(pos)) > TERMINATOR) 
    {
        auto[table_k, table_d] = table.LF(pos.run, pos.offset);
        pos = rif.LF(pos);

        assert(pos.run == table_k);
        assert(pos.offset == table_d);
        ++steps;
    }
}

void test_prior_LF(r_index_f<> rif, LF_table table)
{
    std::string pattern = "CGATATCGCACAGATC"; // Occurs in example, should implement dynamic test
    interval_pos curr = rif.get_table().end();
    for (size_t i = 0; i < pattern.size(); i++)
    {
        char c = pattern[i];
        ulint k = curr.run;
        ulint d = curr.offset;
        while(table.get(k).character != c)
        {
            d = table.get(--k).length - 1;
        }

        auto[k_prime, d_prime] = table.LF(k, d);
        curr = rif.get_table().LF_prior(curr, c);
        
        assert(k_prime == curr.run);
        assert(d_prime == curr.offset);
    }
}

void test_next_LF(r_index_f<> rif, LF_table table)
{
    std::string pattern = "CGATATCGCACAGATC"; // Occurs in example, should implement dynamic test
    interval_pos curr = rif.get_table().begin();
    for (size_t i = 0; i < pattern.size(); i++)
    {
        char c = pattern[i];
        ulint k = curr.run;
        ulint d = curr.offset;
        while(table.get(k).character != c)
        {
            k++;
            d = 0;
        }

        auto[k_prime, d_prime] = table.LF(k, d);
        curr = rif.get_table().LF_next(curr, c);
        
        assert(k_prime == curr.run);
        assert(d_prime == curr.offset);
    }
}

// Test pos to idx
void test_idx_samples(r_index_f<> rif)
{
    interval_pos curr = interval_pos(0,0);
    for (int i = 0; i < rif.size(); i++)
    {
        curr = rif.get_table().reduced_pos(curr);
        assert(i == rif.interval_to_idx(curr));

        curr++;
    }
}

int main(int argc, char *const argv[])
{
    Args args;
    parseArgs(argc, argv, args);

    std::cerr << "Loading the R-Index-F from B-Table" << std::endl;
    verbose("Loading the R-Index-F from B-Table");

    r_index_f<> rif;
    std::string filename_rif = args.filename + rif.get_file_extension();

    ifstream fs_rif(filename_rif);
    rif.load(fs_rif);
    fs_rif.close();

    ifstream patterns(args.filename + ".patterns");
    std::string p;
    double tot = 0;
    size_t i = 0;
    while (std::getline(patterns, p)) {
        std::chrono::high_resolution_clock::time_point t_start = std::chrono::high_resolution_clock::now();
        auto c = rif.count(p);
        std::chrono::high_resolution_clock::time_point t_end = std::chrono::high_resolution_clock::now();
        double t = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
        std::cout << p << "\t" << c << "\t" << t << std::endl;
        tot += t;
        i++;
    }
    nullstream o_st;
    auto bytes_size = rif.serialize(o_st);
    std::cerr << "mean query time (ns): " << tot / i << std::endl;
    std::cerr << "bps: " << double(bytes_size * 8) / rif.size() << std::endl;
    verbose("mean query time (ns): ", tot / i);

    return 0;
}