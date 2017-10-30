#include <adios.h>
#include <mpi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "adios_tt.h"

using std::cerr;
using std::endl;

template <typename n_t>
unsigned long define_array(int i, unsigned int n_elem, uint64_t gh)
{
    int rank = 0;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);

    // define an array
    std::ostringstream oss;
    oss << "dataset_" << rank << "/array_" << i;
    std::string elem_path = oss.str() + "/number_of_elements";
    std::string data_path = oss.str() + "/data";

    adios_define_var(gh, elem_path.c_str(), "",
      adios_integer, "", "", "");

#if defined(ADIOS_ISSUE_1)
    adios_define_var(gh, data_path.c_str(), "", adios_tt<n_t>::type(),
       elem_path.c_str(), "", "");
#else
    adios_define_var(gh, data_path.c_str(), "", adios_tt<n_t>::type(),
       elem_path.c_str(), elem_path.c_str(), "0");
#endif

    return sizeof(int) + n_elem*sizeof(n_t);
}

template <typename n_t>
void write_array(int i, unsigned int n_elem, uint64_t fh)
{
    int rank = 0;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);

    // write the array
    std::ostringstream oss;
    oss << "dataset_" << rank << "/array_" << i;

    std::string elem_path = oss.str() + "/number_of_elements";
    std::string data_path = oss.str() + "/data";

    n_t *data = static_cast<n_t*>(malloc(n_elem*sizeof(n_t)));
    for (int i = 0; i < n_elem; ++i)
        data[i] = n_t(rank*n_elem + i);

    adios_write(fh, elem_path.c_str(), &n_elem);
    adios_write(fh, data_path.c_str(), data);

    // print the array
    cerr << "put " << n_elem << " " << adios_tt<n_t>::name() << endl;
    cerr << +data[0];
    for (int i = 1; i < n_elem; ++i)
        cerr  << (i % 32 == 0 ? "\n" : ", ") << +data[i];
    cerr << endl;

    free(data);
}

int main(int argc, char **argv)
{
    int rank = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);

    if (argc < 5)
    {
        cerr << "ERROR: put [file] [method] [array len] [n steps]" << endl;
        return -1;
    }

    const char *file = argv[1];
    const char *method = argv[2];
    int n_elem = atoi(argv[3]);
    int n_steps = atoi(argv[4]);

    // initialize adios
    adios_init_noxml(comm);

    int64_t gh = 0;
    int64_t fh = 0;
    adios_set_max_buffer_size(500);
    adios_declare_group(&gh, "test", "",
        static_cast<ADIOS_STATISTICS_FLAG>(adios_flag_yes));

    adios_select_method(gh, method, "", "");

    // define
    uint64_t buff_size = 0;
#if defined(ADIOS_ISSUE_3)
    buff_size += define_array<char>(0, n_elem, gh);
    buff_size += define_array<unsigned char>(1, n_elem, gh);
#endif
    buff_size += define_array<int>(2, n_elem, gh);
    buff_size += define_array<unsigned int>(3, n_elem, gh);
    buff_size += define_array<long>(4, n_elem, gh);
    buff_size += define_array<unsigned long>(5, n_elem, gh);
    buff_size += define_array<float>(6, n_elem, gh);
    buff_size += define_array<double>(7, n_elem, gh);

    for (int s = 0; s < n_steps; ++s)
    {
        // open file
        adios_open(&fh, "test", file, s == 0 ? "w" : "a", comm);

        // set size
        adios_group_size(fh, buff_size, &buff_size);

        // write
#if defined(ADIOS_ISSUE_3)
        write_array<char>(0, n_elem, fh);
        write_array<unsigned char>(1, n_elem, fh);
#endif
        write_array<int>(2, n_elem, fh);
        write_array<unsigned int>(3, n_elem, fh);
        write_array<long>(4, n_elem, fh);
        write_array<unsigned long>(5, n_elem, fh);
        write_array<float>(6, n_elem, fh);
        write_array<double>(7, n_elem, fh);

        // close the file
        adios_close(fh);

        cerr << "put finished step " << s << endl;
    }

    adios_finalize(rank);

    MPI_Finalize();

    return 0;
}
