#pragma once

#include <functional>
#include <vecmem/memory/host_memory_resource.hpp>

#include "edm/cell.hpp"
#include "edm/cluster.hpp"
#include "edm/measurement.hpp"
#include "edm/spacepoint.hpp"
#include "geometry/geometry.hpp"
#include "io/csv.hpp"
#include "io/demonstrator_edm.hpp"
#include "io/utils.hpp"

namespace traccc {

inline traccc::geometry read_geometry(const std::string &detector_file) {
    // Read the surface transforms
    std::string io_detector_file = data_directory() + detector_file;
    traccc::surface_reader sreader(
        io_detector_file, {"geometry_id", "cx", "cy", "cz", "rot_xu", "rot_xv",
                           "rot_xw", "rot_zu", "rot_zv", "rot_zw"});
    return traccc::read_surfaces(sreader);
}

inline traccc::host_cell_container read_cells_from_event(
    size_t event, const std::string &cells_dir,
    traccc::geometry surface_transforms,
    vecmem::host_memory_resource &resource) {
    // Read the cells from the relevant event file
    std::string io_cells_file =
        data_directory() + cells_dir + get_event_filename(event, "-cells.csv");
    traccc::cell_reader creader(
        io_cells_file,
        {"geometry_id", "hit_id", "cannel0", "channel1", "activation", "time"});
    return traccc::read_cells(creader, resource, &surface_transforms);
}

inline traccc::host_spacepoint_container read_spacepoints_from_event(
    size_t event, const std::string &hits_dir,
    traccc::geometry surface_transforms,
    vecmem::host_memory_resource &resource) {
    // Read the cells from the relevant event file
    std::string io_hits_file =
        data_directory() + hits_dir + get_event_filename(event, "-hits.csv");
    traccc::fatras_hit_reader hreader(
        io_hits_file,
        {"particle_id", "geometry_id", "tx", "ty", "tz", "tt", "tpx", "tpy",
         "tpz", "te", "deltapx", "deltapy", "deltapz", "deltae", "index"});
    return traccc::read_hits(hreader, resource, &surface_transforms);
}

inline traccc::demonstrator_input read(size_t events,
                                       const std::string &detector_file,
                                       const std::string &cell_directory,
                                       vecmem::host_memory_resource &resource) {
    using namespace std::placeholders;
    auto geometry = read_geometry(detector_file);
    auto readFn = std::bind(read_cells_from_event, _1, cell_directory, geometry,
                            resource);
    traccc::demonstrator_input input_data(events, &resource);

#if defined(_OPENMP)
#pragma omp parallel for
#endif
    for (size_t event = 0; event < events; ++event) {
        traccc::host_cell_container cells_per_event = readFn(event);

#if defined(_OPENMP)
#pragma omp critical
#endif
        { input_data[event] = cells_per_event; }
    }
    return input_data;
}

}  // namespace traccc
