#include <adios.h>
#include <adios_read.h>
#include <mpi.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "adios_tt.h"

using std::cerr;
using std::endl;

template <typename val_t>
int adiosInq(ADIOS_FILE *fp, const std::string &path, val_t &val)
{
  ADIOS_VARINFO *vinfo = adios_inq_var(fp, path.c_str());
  if (!vinfo)
    {
    cerr << "ADIOS stream is missing \"" << path << "\"" << endl;
    return -1;
    }
  val = *static_cast<val_t*>(vinfo->value);
  adios_free_varinfo(vinfo);
  return 0;
}

int adiosInqBlock(ADIOS_FILE *fp, const std::string &path)
{
  ADIOS_VARINFO *vinfo = adios_inq_var(fp, path.c_str());
  if (!vinfo)
    {
    cerr << "ADIOS stream is missing \"" << path << "\"" << endl;
    return -1;
    }
  adios_inq_var_blockinfo(fp, vinfo);


  cerr << path << " has nsteps = " << vinfo->nsteps << " nblocks = "  << vinfo->nblocks[0] << endl;

  for (int i = 0; i < vinfo->nblocks[0]; ++i)
    {
    cerr << "start = " << vinfo->blockinfo[0].start[0]
      << " count = " << vinfo->blockinfo[0].count[0]
      << " process_id = " << vinfo->blockinfo[0].process_id
      << " time_index = " <<  vinfo->blockinfo[0].time_index << endl;
    }
  adios_free_varinfo(vinfo);
  return 0;
}

ADIOS_READ_METHOD get_read_method(const char *method)
{
    if (strcmp(method, "BP") == 0)
        return ADIOS_READ_METHOD_BP;
    if (strcmp(method, "DATASPACES") == 0)
        return ADIOS_READ_METHOD_DATASPACES;
    if (strcmp(method, "FLEXPATH") == 0)
        return ADIOS_READ_METHOD_FLEXPATH;
    return static_cast<ADIOS_READ_METHOD>(-1);
}

template<typename n_t>
int read_array(int dataset_id, int array_id, ADIOS_FILE *fp)
{
    int rank = 0;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);

    // read array
    std::ostringstream oss;
    oss << "dataset_" << dataset_id << "/array_" << array_id;

    std::string elem_path = oss.str() + "/number_of_elements";
    int n_elem = 0;
    if (adiosInq(fp, elem_path, n_elem))
      return -1;

    n_t *data = static_cast<n_t*>(malloc(n_elem*sizeof(n_t)));
    std::string data_path = oss.str() + "/data";

    //adiosInqBlock(fp, data_path);

    ADIOS_SELECTION *sel = adios_selection_writeblock(rank);
    adios_schedule_read(fp, sel, data_path.c_str(), 0, 1, data);

    int ierr = adios_perform_reads(fp, 1);

    adios_selection_delete(sel);

    if (ierr)
        return -1;

    // print the array
    cerr << "get "<< dataset_id << " "  << n_elem << " " << adios_tt<n_t>::name() << endl;
    cerr << +data[0];
    for (int i = 1; i < n_elem; ++i)
        cerr  << (i % 32 == 0 ? "\n" : ", ") << +data[i];
    cerr << endl;

    return 0;
}



int main(int argc, char **argv)
{
    int rank = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);

    if (argc < 3)
    {
        cerr << "ERROR: get [file] [method]" << endl;
        return -1;
    }

    const char *file = argv[1];
    const char *method_str = argv[2];
    int n_steps = 0;

    // initialize adios
    ADIOS_READ_METHOD method = get_read_method(method_str);
    adios_read_init_method(method, comm, "verbose=2");

    // open the file ADIOS_LOCKMODE_ALL
    ADIOS_FILE *fp = adios_read_open(file, method, comm,
      ADIOS_LOCKMODE_CURRENT, -1.0f);

    if (!fp)
    {
        cerr << "failed to open " << file << endl;
        return -1;
    }

    int ierr = 0;
    while (adios_errno == 0) //err_end_of_stream)
    {
        int n_datasets = 0;
        if (adiosInq(fp, "n_datasets", n_datasets))
            return -1;

        for (int i = 0; i < n_datasets; ++i)
        {
            int dataset_id = rank*n_datasets + i;
            // read array
#if defined(ADIOS_ISSUE_3)
            read_array<char>(dataset_id, 0, fp);
            read_array<unsigned char>(dataset_id, 1, fp);
#endif
            read_array<int>(dataset_id, 2, fp);
            read_array<unsigned int>(dataset_id, 3, fp);
            read_array<long>(dataset_id, 4, fp);
            read_array<unsigned long>(dataset_id, 5, fp);
            read_array<float>(dataset_id, 6, fp);
            read_array<double>(dataset_id, 7, fp);
        }

        adios_release_step(fp);
        ierr = adios_advance_step(fp, 0, method == ADIOS_READ_METHOD_DATASPACES ? -1.0f : 0.0f);
    }

    adios_read_close(fp);
    adios_read_finalize_method(method);

    MPI_Finalize();

    return 0;
}
