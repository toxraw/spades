//***************************************************************************
//* Copyright (c) 2015 Saint Petersburg State University
//* Copyright (c) 2011-2014 Saint Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#include "utils/standard_base.hpp"
#include "utils/stl_utils.hpp"
#include "utils/logger/log_writers.hpp"

#include "pipeline/graphio.hpp"
#include "pipeline/graph_pack.hpp"
#include "pipeline/config_struct.hpp"

#include "assembly_graph/core/graph.hpp"
#include "modules/alignment/sequence_mapper.hpp"

#include "io/reads/io_helper.hpp"
#include "common/assembly_graph/core/coverage.hpp"

#include "io/reads/wrapper_collection.hpp"
#include "assembly_graph/stats/picture_dump.hpp"
#include "io/reads/multifile_reader.hpp"

#include <iostream>
#include <fstream>

void create_console_logger() {
    logging::logger *log = logging::create_logger("", logging::L_INFO);
    log->add_writer(std::make_shared<logging::console_writer>());
    logging::attach_logger(log);
}

namespace debruijn_graph {

typedef debruijn_graph::BasicSequenceMapper<debruijn_graph::Graph, Index> MapperClass;

class IdealAligner {
private:    
    const conj_graph_pack &gp_;
    const string &output_file_;
    std::shared_ptr<MapperClass> mapper_;

public:
    IdealAligner(const conj_graph_pack &gp, 
                 const string &output_file,
                 std::shared_ptr<MapperClass> mapper):
      gp_(gp), output_file_(output_file), mapper_(mapper){}


    bool IsCanonical(EdgeId e) const {
        return e <= gp_.g.conjugate(e);
    }

    EdgeId Canonical(EdgeId e) const {
        return IsCanonical(e) ? e : gp_.g.conjugate(e);
    }
    
    void AlignRead(const io::SingleRead &read){
        Sequence seq(read.sequence());
        INFO("Read " << read.name() <<". Current Read")
        auto current_mapping = mapper_->MapRead(read);
        ReadPathFinder<debruijn_graph::Graph> readmapper(gp_.g);
        auto read_mapping = readmapper.FindReadPath(current_mapping);
        if (current_mapping.empty()){
            INFO("Read " << read.name() <<" wasn't aligned");
        }
        if (read_mapping.size() > 0){
            std::string pathStr = "";
            std::string curpathStr = "";
            std::string edgesStr1 = "";
            std::string edgesStr2 = "";
            std::string pathlenStr = "";
            std::string edgelenStr = "";
            std::string str = "";
            // EdgeId last_edge= EdgeId();
            // omnigraph::MappingRange last_range;
            // for (const auto &edge_mapping: current_mapping) {
            //     EdgeId edgeid = edge_mapping.first;
            //     omnigraph::MappingRange range= edge_mapping.second;
            //     last_edge = edgeid;
            //     last_range = range;
            //     pathStr += std::to_string(edgeid.int_id()) + " (" + std::to_string(range.mapped_range.start_pos) + "," + std::to_string(range.mapped_range.end_pos) + ") ["
            //                    + std::to_string(range.initial_range.start_pos) + "," + std::to_string(range.initial_range.end_pos) + "], ";
            //     //pathStr += std::to_string(edgeid.int_id()) + ",";
            //     pathlenStr += std::to_string(range.mapped_range.end_pos - range.mapped_range.start_pos) + ",";
            //     edgelenStr += std::to_string(gp_.g.length(edgeid)) + ",";
            //     string tmp = gp_.g.EdgeNucls(edgeid).str();
            //     string to_add = tmp.substr(range.mapped_range.start_pos, range.mapped_range.end_pos - range.mapped_range.start_pos);
            //     str += to_add;
            // }
            // string tmp = gp_.g.EdgeNucls(last_edge).str();
            // str += tmp.substr(last_range.mapped_range.end_pos, gp_.g.k());
            for (const auto &edge_mapping: current_mapping) {
                EdgeId edgeid = edge_mapping.first;
                omnigraph::MappingRange range= edge_mapping.second;
                curpathStr += std::to_string(edgeid.int_id()) + " (" + std::to_string(range.mapped_range.start_pos) + "," + std::to_string(range.mapped_range.end_pos) + ") ["
                                + std::to_string(range.initial_range.start_pos) + "," + std::to_string(range.initial_range.end_pos) + "], ";
                edgesStr1 += std::to_string(edgeid.int_id()) + ",";
            }

            for (const auto &edge_mapping: read_mapping) {
                EdgeId edgeid = edge_mapping;
                edgesStr2 += std::to_string(edgeid.int_id()) + ",";
            }
            int j = 0;
            INFO(curpathStr);
            INFO(edgesStr1);
            INFO(edgesStr2);
            for (int i = 0; i < read_mapping.size(); ++ i) {
                EdgeId edgeid = read_mapping[i];
                INFO("edgeid=" << edgeid.int_id() << " " << i << "; " << " cur_edge=" << gp_.g.int_id(current_mapping[j].first) << " " << j)
                omnigraph::MappingRange range= current_mapping[j].second;
                size_t mapping_start = range.mapped_range.start_pos;
                size_t mapping_end = range.mapped_range.end_pos + gp_.g.k();
                size_t initial_start = range.initial_range.start_pos;
                size_t initial_end = range.initial_range.end_pos + gp_.g.k();
                if (i > 0){
                    mapping_start = 0;
                    if (current_mapping[j].first != edgeid) {
                        initial_start = current_mapping[j].second.initial_range.end_pos + 1;
                    }
                }
                if (i < read_mapping.size() - 1) {
                    mapping_end = gp_.g.length(edgeid);
                    if (current_mapping[j].first != edgeid) {
                        // INFO("read_mapping " << read_mapping.size() << " " << gp_.g.int_id(edgeid) << " " << gp_.g.int_id(current_mapping[j].first));
                        // INFO("cur_map " << current_mapping.size() << " " << j + 1);
                        initial_end = current_mapping[j].second.initial_range.start_pos - 1;
                    } else {
                        initial_end = range.initial_range.end_pos;
                    }
                } else{
                    INFO("Readname=" << read.name() << " mapping_end" << mapping_end << " edge_len=" << gp_.g.length(edgeid));
                }
                pathStr += std::to_string(edgeid.int_id()) + " (" + std::to_string(mapping_start) + "," + std::to_string(mapping_end) + ") ["
                                + std::to_string(initial_start) + "," + std::to_string(initial_end) + "], ";
                pathlenStr += std::to_string(mapping_end - mapping_start) + ",";
                edgelenStr += std::to_string(gp_.g.length(edgeid)) + ",";
                string tmp = gp_.g.EdgeNucls(edgeid).str();
                //INFO("Edgeid=" << edgeid.int_id() << " size=" << tmp.size() << " mapping_start=" << mapping_start << " mapping_end=" << mapping_end);
                string to_add = tmp.substr(mapping_start, mapping_end - mapping_start);
                str += to_add;
                if (current_mapping[j].first == edgeid){
                    j++;
                }
            }
            INFO("Path: " << pathStr);
            INFO("MappedPath: " << curpathStr);
            INFO("Read " << read.name() << " length=" << seq.size() << "; path_len=" << current_mapping.size()  << "; aligned: " << pathStr);
            //INFO("Seq subs: " << subStr);
#pragma omp critical
        {
            ofstream myfile;
            myfile.open(output_file_ + ".tsv", std::ofstream::out | std::ofstream::app);
            myfile << read.name() << "\t" << seq.size() << "\t" << current_mapping.size() << "\t" << pathStr << "\t" << pathlenStr << "\t" << edgelenStr << "\t" << str << "\n";
            myfile.close();
        }
        }  
    }  
};


void Launch(size_t K, const string &saves_path, const string &reads_fasta, const string &output_file) {
    conj_graph_pack gp(K, "tmp3", 0);
    //graphio::ScanGraphPack(saves_path, gp);
    std::shared_ptr<MapperClass> mapper(new MapperClass(gp.g, gp.index, gp.kmer_mapper));
    gp.kmer_mapper.Attach();
    debruijn_graph::graphio::ScanGraphPack(saves_path, gp);
    INFO("Loaded graph with " << gp.g.size() << " vertices");

    io::ReadStreamList<io::SingleRead> streams;
    streams.push_back(make_shared<io::FixingWrapper>(make_shared<io::FileReadStream>(reads_fasta)));
    
    io::SingleStreamPtr sstream = io::MultifileWrap(streams); 
    std::vector<io::SingleRead> wrappedreads; 
    while (!sstream->eof()) {
        io::SingleRead read;
        *sstream >> read;
        wrappedreads.push_back(std::move(read));
    }
    INFO("Loaded reads from " << reads_fasta);
    
    IdealAligner aligner(gp, output_file, mapper); 
    INFO("IdealAligner created");

#pragma omp parallel num_threads(1)
#pragma omp for
        for (size_t i =0 ; i < wrappedreads.size(); ++i) {
            aligner.AlignRead(wrappedreads[i]);
        }
}
}

int main(int argc, char **argv) {
    omp_set_num_threads(1);
    if (argc < 5) {
        cout << "Usage: idealreads_aligner <K>"
             << " <saves path> <long reads file (fasta)> <ouput-prefix>" << endl;
        exit(1);
    }
    create_console_logger();
    size_t K = std::stoll(argv[1]);
    string saves_path = argv[2];
    INFO("Load graph from " << saves_path);
    string longreads_file = argv[3];
    INFO("Load long reads from " << longreads_file);
    string output_file = argv[4];
    debruijn_graph::Launch(K, saves_path, longreads_file, output_file);
    return 0;
}
