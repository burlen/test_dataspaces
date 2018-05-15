#include <adios.h>
#include <mpi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "adios_tt.h"

using std::cerr;
using std::endl;

// MPI info
MPI_Comm g_comm = MPI_COMM_WORLD;
int g_rank = 0;
int g_n_ranks = 1;

#define ERROR(msg)                                  \
{std::cerr << "ERROR! [" << g_rank << "]["          \
    << __FILE__ << ":" << __LINE__ <<  "]" << endl  \
    << "" msg << endl;}

// --------------------------------------------------------------------------
template <typename n_t>
int define_array_adios(int64_t gh,
    int mesh_id, int array_id, unsigned int n_elem, uint64_t &buff_size)
{
    // tell ADIOS how we define the data
    std::ostringstream oss;
    oss << "dataset_" << mesh_id << "/array_" << array_id;

    std::string elem_path = oss.str() + "/number_of_elements";
    if (adios_define_var(gh, elem_path.c_str(), "",
        adios_integer, "", "", ""))
        return -1;

    std::string data_path = oss.str() + "/data";
    if (adios_define_var(gh, data_path.c_str(), "", adios_tt<n_t>::type(),
        elem_path.c_str(), elem_path.c_str(), "0"))
        return -1;

    // return the number of bytes to hold the data
    buff_size += sizeof(int) + n_elem*sizeof(n_t);

    return 0;
}


// --------------------------------------------------------------------------
int define_group_adios(const char *name,
    const char *method, int n_datasets_per, unsigned int n_elem,
    uint64_t &buff_size)
{
    buff_size = 0;

    // initialize adios
    adios_init_noxml(g_comm);

    adios_set_max_buffer_size(500);

    int64_t gh = 0;
    if (adios_declare_group(&gh, name, "",
        static_cast<ADIOS_STATISTICS_FLAG>(adios_flag_yes)))
        return -1;

    if (adios_select_method(gh, method, "", ""))
        return -1;

    // define variables and per step buffer size
    adios_define_var(gh, "n_datasets_per_writer", "", adios_integer, "", "", "");

    adios_define_var(gh, "n_writers", "", adios_integer, "", "", "");

    buff_size += 2*sizeof(int);

    for (int i = 0; i < n_datasets_per; ++i)
    {
        int dataset_id = n_datasets_per*g_rank + i;
        if (define_array_adios<double>(gh, dataset_id, 7, n_elem, buff_size))
            return -1;
    }

    return 0;
}

// --------------------------------------------------------------------------
template <typename n_t>
n_t *initialize_array(unsigned int n_elem)
{
    // allocate and initialize the array
    n_t *data = static_cast<n_t*>(malloc(n_elem*sizeof(n_t)));
    for (int i = 0; i < n_elem; ++i)
        data[i] = n_t(g_rank*n_elem + i);
    return data;
}

// --------------------------------------------------------------------------
template <typename n_t>
int write_array_adios(uint64_t fh, int dataset_id,
    int array_id, unsigned int n_elem, n_t *data)
{
    // write the array
    std::ostringstream oss;
    oss << "dataset_" << dataset_id << "/array_" << array_id;

    std::string elem_path = oss.str() + "/number_of_elements";
    if (adios_write(fh, elem_path.c_str(), &n_elem))
    {
        ERROR("failed to write " << elem_path)
        return -1;
    }

    std::string data_path = oss.str() + "/data";
    if (adios_write(fh, data_path.c_str(), data))
    {
        ERROR("Failed to write " << data_path)
        return -1;
    }

    return 0;
}

// --------------------------------------------------------------------------
int write_datasets_adios(uint64_t fh, int n_datasets_per, unsigned int n_elem)
{
    // write the datasets
    if (adios_write(fh, "n_datasets_per_writer", &n_datasets_per) ||
        adios_write(fh, "n_writers", &g_n_ranks))
    {
        ERROR("Failed to write dataset metadata")
        return -1;
    }

    for (int i = 0; i < n_datasets_per; ++i)
    {
        double *data = initialize_array<double>(n_elem);

        int dataset_id = n_datasets_per*rank + i;
        if (write_array_daios<double>(fh, dataset_id, 7, n_elem, data))
        {
            ERROR("Failed to write array")
            return -1;
        }

        free(data);
    }

    return 0;
}

// --------------------------------------------------------------------------
template <typename n_t>
void print_array(int dataset_id, int array_id, unsigned int n_elem)
{
    // print the array
    cerr << g_rank << " put " << dataset_id << " " << n_elem << " " << adios_tt<n_t>::name() << endl;
    cerr << +data[0];
    for (int i = 1; i < n_elem; ++i)
        cerr  << (i % 32 == 0 ? "\n" : ", ") << +data[i];
    cerr << endl;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    // initialize MPI stuff
    g_comm = MPI_COMM_WORLD;
    MPI_Comm_rank(g_comm, &g_rank);
    MPI_Comm_size(g_comm, &g_n_ranks);

    // process the comand line
    if (argc < 5)
    {
        cerr << "ERROR: put [file] [method] [array len] [n steps]" << endl;
        return -1;
    }
    const char *file = argv[1];
    const char *method = argv[2];
    int n_elem = atoi(argv[3]);
    int n_datasets_per = atoi(argv[4]);
    int n_steps = atoi(argv[5]);

    // describe the data layout to ADIOS, and compute per step buffer size
    uint64_t buff_size = 0;
    if (define_group_adios("data_group", method, n_datasets_per, n_elem, buff_size))
    {
        ERROR("Failed to define ADIOS group")
        return -1;
    }

    // write the time steps one by one
    for (int s = 0; s < n_steps; ++s)
    {
        // open file in append mode
        int64_t fh = 0;
        if (adios_open(&fh, "data_group", file, s == 0 ? "w" : "a", g_comm))
        {
            ERROR("Failed to open file " << file)
            return -1;
        }

        // set buffer size
        adios_group_size(fh, buff_size, &buff_size);

        // write the datasets
        if (write_datasets_adios(h, n_datasets_per, n_elem))
        {
            ERROR("Failed to write datasets")
            return -1;
        }

        // close the file
        adios_close(fh);

        cerr << rank << " put finished step " << s << endl;
    }

    adios_finalize(g_rank);

    MPI_Finalize();

    return 0;
}
