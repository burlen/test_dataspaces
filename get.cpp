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

MPI_Comm comm = MPI_COMM_WORLD;

struct AdiosFile
{
  AdiosFile() : File(nullptr), Method(static_cast<ADIOS_READ_METHOD>(-1)) {}

  AdiosFile(ADIOS_FILE *file, ADIOS_READ_METHOD method)
    : File(file), Method(method) {}

  operator ADIOS_FILE*(){ return this->File; }
  operator const ADIOS_FILE*() const { return this->File; }
  operator ADIOS_READ_METHOD(){ return this->Method; }

  ADIOS_FILE *File;
  ADIOS_READ_METHOD Method;
};

template <typename val_t>
int adiosInq(ADIOS_FILE *fp, ADIOS_SELECTION *sel,
  const std::string &path, val_t &val)
{
  if (adios_schedule_read(fp, sel, path.c_str(), 0, 1, &val) ||
    adios_perform_reads(fp, 1))
    {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    cerr << "Failed to read " << path << "on rank " << rank << endl;
    return -1;
    }
  return 0;
}

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
int read_array(AdiosFile *fp, int writer_id, int dataset_id, int array_id)
{
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cerr << rank << " reading dataset_" << dataset_id << "/array_"
      << array_id << " from " << writer_id << endl;

    // read array
    ADIOS_SELECTION *sel = nullptr;
    if (fp->Method != ADIOS_READ_METHOD_BP)
      sel = adios_selection_writeblock(writer_id);

    std::ostringstream oss;
    oss << "dataset_" << dataset_id << "/array_" << array_id;

    std::string elem_path = oss.str() + "/number_of_elements";
    int n_elem = 0;
    if (adiosInq(fp->File, sel, elem_path, n_elem))
      return -1;

    n_t *data = static_cast<n_t*>(malloc(n_elem*sizeof(n_t)));
    std::string data_path = oss.str() + "/data";

    adios_schedule_read(fp->File, sel, data_path.c_str(), 0, 1, data);

    if (adios_perform_reads(fp->File, 1))
      return -1;

    if (sel)
      adios_selection_delete(sel);

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
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(comm, &rank);

    int n_ranks = 1;
    MPI_Comm_size(comm, &n_ranks);

    if (argc < 3)
    {
        cerr << "ERROR: get [file] [method]" << endl;
        return -1;
    }

    const char *fileName = argv[1];
    const char *method_str = argv[2];
    int n_steps = 0;

    // initialize adios
    ADIOS_READ_METHOD method = get_read_method(method_str);
    adios_read_init_method(method, comm, "verbose=2");

    // open the file ADIOS_LOCKMODE_ALL
    ADIOS_FILE *fp = adios_read_open(fileName, method, comm,
      ADIOS_LOCKMODE_CURRENT, -1.0f);

    if (!fp)
    {
        cerr << rank << " failed to open " << fileName << endl;
        return -1;
    }

    AdiosFile *file = new AdiosFile(fp, method);

    int ierr = 0;
    while (adios_errno == 0) //err_end_of_stream)
    {
        int n_datasets_per = 0;
        if (adiosInq(fp, "n_datasets_per_writer", n_datasets_per))
            return -1;

        int n_writers = 0;
        if (adiosInq(fp, "n_writers", n_writers))
            return -1;

        // partiton the same number of datasets to each rank
        int n_datasets = n_writers*n_datasets_per;

        int n_per_rank = n_datasets/n_ranks;
        int n_left_over = n_datasets%n_ranks;

        int n_local = n_per_rank +
           (rank < n_left_over ? 1 : 0);

        int start_id = rank*n_per_rank +
          (rank < n_left_over ? rank : n_left_over);

        if (n_local < 1)
        {
            cerr << rank << " has nothing to read" << endl;
        }
        else
        {
            for (int i = 0; i < n_local; ++i)
            {
                int dataset_id = start_id + i;
                int writer_id = dataset_id/n_datasets_per;
                read_array<double>(file, writer_id, dataset_id, 7);
            }
        }

        adios_release_step(fp);
        ierr = adios_advance_step(fp, 0, method == ADIOS_READ_METHOD_DATASPACES ? -1.0f : 0.0f);
    }

    adios_read_close(fp);
    adios_read_finalize_method(method);

    delete file;

    MPI_Finalize();

    return 0;
}
