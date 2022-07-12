/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021-2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// I/O log file
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <cstdio> // std::remove(fpath)
#include <chrono>


namespace traccc:: bench_s {

    class timer {
    private:
        std::chrono::time_point<std::chrono::system_clock> start_;


    public:
        timer() {
            start_ = std::chrono::system_clock::now();
        }
        
        // Resets chrono and gets time since last reset / creation (seconds)
        double reset() {
            auto stop = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed = stop - start_;
            start_ = std::chrono::system_clock::now();
            return elapsed.count();
        }

        // /*time*/ auto start_clusterization_cpu =
        //     std::chrono::system_clock::now();

        // auto measurements_per_event = ca(cells_per_event);

        // /*time*/ auto end_clusterization_cpu = std::chrono::system_clock::now();
        // /*time*/ std::chrono::duration<double> time_clusterization_cpu =
        //     end_clusterization_cpu - start_clusterization_cpu;
        // /*time*/ omp_clusterization += time_clusterization_cpu.count();
    };

    class logger {

    private:
        std::ofstream out_log_file, out_log_file_chk;
        bool open_success = false;
        std::string full_file_path = "<no_path>";
        static const uint label_max_len =  22;

        std::string padTo(std::string str, const size_t num, const char paddingChar = ' ') {
            if(num > str.size()) str.insert(str.size(), num - str.size(), paddingChar);
            return str;
        }

        static const bool REPLACE_OLD_LOG = true;

    public:

        inline bool file_exists_test0(const std::string& name) {
            std::ifstream f(name.c_str());
            return f.good();
        }

        bool init_log_file(std::string fname) {
            std::string wdir_tmp = std::filesystem::current_path();
            std::string wdir = wdir_tmp + "/";
            std::string output_file_path = wdir + std::string(fname);
            full_file_path = output_file_path;
            std::string output_file_path_chk = output_file_path + ".chk";

            if (REPLACE_OLD_LOG) {
                if (file_exists_test0(output_file_path))     std::remove(output_file_path.c_str());
                if (file_exists_test0(output_file_path_chk)) std::remove(output_file_path_chk.c_str());
            }

            if ( file_exists_test0(output_file_path) ) {
                log("\n\n\n\n\nFILE ALREADY EXISTS, SKIPPING TEST");
                log("NAME = " + fname + "\n");
                log("FULL PATH = " + output_file_path + "\n\n\n\n\n");
                open_success = false;
                return false;
            }

            log("\n\n\n================ BENCHMARK SEQ RUN ================");
            log("Bench log file: " + output_file_path);
            log("Bench chk file: " + output_file_path+".chk");
            log("\n\n");

            out_log_file.open(output_file_path);
            out_log_file_chk.open(output_file_path_chk);
            open_success = true;
            return true;
        }

        // Constructor
        logger(std::string fname) {
            init_log_file(fname);
        }

        // Print on terminal
        void log(std::string str) {
            std::cout << str << std::endl;
        }
        void logs(std::string str) {
            std::cout << str << std::flush;
        }

        // Writes on file log
        // void flog(std::string value) {
        //     out_log_file << value;
        // }
        // void flogn(std::string value) {
        //     out_log_file << value << "\n";
        // }

        // Écrire une valeur
        // void flog(double value, std::string chk_name) {
        //     if ( ! open_success ) {
        //         log("File not open: " + full_file_path);
        //         return;
        //     }
        //     out_log_file << value;
        //     out_log_file_chk << padTo(chk_name, label_max_len, ' ') << ":" << value << "  ";
        // }
        // void flogn(double value, std::string chk_name) {
        //     if ( ! open_success ) {
        //         log("File not open: " + full_file_path);
        //         return;
        //     }
        //     out_log_file     << chk_name << " " << value << "\n";
        //     out_log_file_chk << padTo(chk_name, label_max_len, ' ') << ": " << value << "\n";
        // }


        void flogn_ext(std::string value, std::string label = "", bool newline = false) {
            if ( ! open_success ) {
                log("File not open: " + full_file_path);
                return;
            }
            // Has a label, first prints label
            if (label.compare("") != 0) {
                // log file => "label value"
                out_log_file     << label << " ";
                // check file => "label    : value"
                out_log_file_chk << padTo(label, label_max_len, ' ') << ": ";
            }
            out_log_file     << value;
            out_log_file_chk << value;

            if (newline) {
                out_log_file     << "\n";
                out_log_file_chk << "\n";
            }
        }

        // Écrire simplement une valeur
        // void flogn(std::string value) {
        //     flogn_ext(value, "", true);
        //     if ( ! open_success ) {
        //         log("File not open: " + full_file_path);
        //         return;
        //     }
        //     out_log_file     << value << "\n";
        //     out_log_file_chk << value << "\n";
        // }

        void flogn(std::string value) { flogn_ext(value, "", true); }
        void flogn(double value)      { flogn_ext(std::to_string(value), "", true); }

        void flogn(std::string value, std::string label) { flogn_ext(value, label, true); }
        void flogn(double value, std::string label)      { flogn_ext(std::to_string(value), label, true); }


        void flog(double value, std::string chk_name) {
            if ( ! open_success ) {
                log("File not open: " + full_file_path);
                return;
            }
            out_log_file << value;
            out_log_file_chk << padTo(chk_name, label_max_len, ' ') << ":" << value << "  ";
        }



        // void flog_chk(std::string value) {
        //     out_log_file_chk << value;
        // }
        // void flogn_chk(std::string value) {
        //     out_log_file_chk << value << "\n";
        // }



        void close() {
            out_log_file.close();
            out_log_file_chk.close();
        }
    };
} // close namespace traccc::bench_timer
// namespace bt = traccc::bench_timer;